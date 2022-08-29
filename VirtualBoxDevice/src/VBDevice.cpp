// ReSharper disable CppEqualOperandsInBinaryExpression
#define LOG_GROUP LOG_GROUP_MISC

#include <VBox/vmm/pdmdev.h>
#include <VBox/version.h>
#include <VBox/err.h>
#include <VBox/log.h>

#include <VBox/com/assert.h>
#include <VBox/com/defs.h>
#include <VBox/com/string.h>
#include <VBox/com/Guid.h>
#include <VBox/com/VirtualBox.h>

#include <iprt/assert.h>

/**
 * Playground device per function (sub-device) data.
 */
struct SoftGpuDeviceFunction final
{
    /** The function number. */
    uint8_t         iFun;
    /** Device function name. */
    char            szName[31];
    /** MMIO region \#0 name. */
    char            szMmio0[32];
    /** MMIO region \#2 name. */
    char            szMmio2[32];
    /** The MMIO region \#0 handle. */
    IOMMMIOHANDLE   hMmio0;
    /** The MMIO region \#2 handle. */
    IOMMMIOHANDLE   hMmio2;
    /** Backing storage. */
    uint8_t         abBacking[4096];
};

/**
 * Playground device instance data.
 */
struct SoftGpuDevice final
{
    /** PCI device functions. */
    SoftGpuDeviceFunction pciFunction;
};

#define SOFT_GPU_SSM_VERSION 1

static DECLCALLBACK(int) softGpuConstruct(PPDMDEVINS deviceInstance, int instance, PCFGMNODE cfg)
{
    /*
     * Check that the device instance and device helper structures are compatible.
     * THIS IS ALWAYS THE FIRST STATEMENT IN A CONSTRUCTOR!
     */
    PDMDEV_CHECK_VERSIONS_RETURN(deviceInstance); /* This must come first. */
    Assert(instance == 0); RT_NOREF(instance);

    /*
     * Initialize the instance data so that the destructor won't mess up.
     */
    SoftGpuDevice* device = PDMDEVINS_2_DATA(deviceInstance, SoftGpuDevice*);

    /*
     * Validate and read the configuration.
     */
    PDMDEV_VALIDATE_CONFIG_RETURN(deviceInstance, "Whatever1|NumFunctions|BigBAR0MB|BigBAR0GB|BigBAR2MB|BigBAR2GB", "");

    const PCPDMDEVHLPR3 pdmDeviceApi = deviceInstance->pHlpR3;

    AssertCompile(RT_ELEMENTS(deviceInstance->apPciDevs) >= 1);

    uint8_t numFunctions;

    int rc = pdmDeviceApi->pfnCFGMQueryU8Def(cfg, "NumFunctions", &numFunctions, 1);

    if(RT_FAILURE(rc))
    {
        return PDMDEV_SET_ERROR(deviceInstance, rc, N_("Configuration error: Failed to query integer value \"NumFunctions\""));
    }

    if(numFunctions != 1)
    {
        return PDMDEV_SET_ERROR(deviceInstance, rc, N_("Configuration error: Invalid \"NumFunctions\" value (must be 1)"));
    }

    uint16_t bigBAR0GB;
    rc = pdmDeviceApi->pfnCFGMQueryU16Def(cfg, "BigBAR0GB", &bigBAR0GB, 2);  /* Default to nothing. */


    if(RT_FAILURE(rc))
    {
        return PDMDEV_SET_ERROR(deviceInstance, rc, N_("Configuration error: Failed to query integer value \"BigBAR0GB\""));
    }

    if(bigBAR0GB > 512)
    {
        return PDMDEV_SET_ERROR(deviceInstance, rc, N_("Configuration error: Invalid \"BigBAR0GB\" value (must be 512 or less)"));
    }

    RTGCPHYS firstBAR;

    if(bigBAR0GB)
    {
        firstBAR = bigBAR0GB * _1G64;
    }
    else
    {
        uint16_t bigBAR0MB;
        rc = pdmDeviceApi->pfnCFGMQueryU16Def(cfg, "BigBAR0MB", &bigBAR0MB, 256);  /* 8 MB default. */

        if(RT_FAILURE(rc))
        {
            return PDMDEV_SET_ERROR(deviceInstance, rc, N_("Configuration error: Failed to query integer value \"BigBAR0MB\""));
        }

        if(bigBAR0MB < 1 || bigBAR0MB > 4095)
        {
            return PDMDEV_SET_ERROR(deviceInstance, rc, N_("Configuration error: Invalid \"BigBAR0MB\" value (must be between 1 and 4095)"));
        }

        firstBAR = static_cast<RTGCPHYS>(bigBAR0MB) * _1M;
    }

    uint16_t uBigBAR2GB;
    rc = pdmDeviceApi->pfnCFGMQueryU16Def(cfg, "BigBAR2GB", &uBigBAR2GB, 0);  /* Default to nothing. */

    if(RT_FAILURE(rc))
    {
        return PDMDEV_SET_ERROR(deviceInstance, rc, N_("Configuration error: Failed to query integer value \"BigBAR2GB\""));
    }

    if(uBigBAR2GB > 512)
    {
        return PDMDEV_SET_ERROR(deviceInstance, rc, N_("Configuration error: Invalid \"BigBAR2GB\" value (must be 512 or less)"));
    }

    RTGCPHYS secondBAR;
    if(uBigBAR2GB)
    {
        secondBAR = uBigBAR2GB * _1G64;
    }
    else
    {
        uint16_t bigBAR2MB;
        rc = pdmDeviceApi->pfnCFGMQueryU16Def(cfg, "BigBAR2MB", &bigBAR2MB, 16); /* 16 MB default. */

        if(RT_FAILURE(rc))
        {
            return PDMDEV_SET_ERROR(deviceInstance, rc, N_("Configuration error: Failed to query integer value \"BigBAR2MB\""));
        }

        if(bigBAR2MB < 1 || bigBAR2MB > 4095)
        {
            return PDMDEV_SET_ERROR(deviceInstance, rc, N_("Configuration error: Invalid \"BigBAR2MB\" value (must be between 1 and 4095)"));
        }

        secondBAR = static_cast<RTGCPHYS>(bigBAR2MB) * _1M;
    }

    /*
     * PCI device setup.
     */
    PPDMPCIDEV pciDevice = deviceInstance->apPciDevs[0];
    SoftGpuDeviceFunction* pciFunction = &device->pciFunction;
    RTStrPrintf(pciFunction->szName, sizeof(pciFunction->szName), "soft_gpu%u", 0);
    pciFunction->iFun = 0;

    PDMPCIDEV_ASSERT_VALID(devInstance, pciDevice);

    PDMPciDevSetVendorId(pciDevice, 0xFFFD);
    PDMPciDevSetDeviceId(pciDevice, 0x0001);
    PDMPciDevSetClassBase(pciDevice, 0x03);  /* display controller device */
    PDMPciDevSetClassSub(pciDevice, 0x00);  /* VGA compatible controller device */
    PDMPciDevSetHeaderType(pciDevice, 0x00);  /* normal, multifunction device */

    rc = PDMDevHlpPCIRegisterEx(deviceInstance, pciDevice, 0 /*fFlags*/, 0, 0, device->pciFunction.szName);
    AssertLogRelRCReturn(rc, rc);

    /* First region. */
    RTStrPrintf(pciFunction->szMmio0, sizeof(pciFunction->szMmio0), "PG-F%d-BAR0", 0);

    rc = PDMDevHlpMmioCreate(deviceInstance, firstBAR, pciDevice, 0 /*iPciRegion*/,
        nullptr, nullptr, pciFunction,
        IOMMMIO_FLAGS_READ_DWORD | IOMMMIO_FLAGS_WRITE_DWORD_ZEROED, pciFunction->szMmio0, &pciFunction->hMmio0);
    AssertLogRelRCReturn(rc, rc);

    rc = PDMDevHlpPCIIORegionRegisterMmioEx(deviceInstance, pciDevice, 0, firstBAR,
        static_cast<PCIADDRESSSPACE>(PCI_ADDRESS_SPACE_MEM | PCI_ADDRESS_SPACE_BAR64 | PCI_ADDRESS_SPACE_MEM_PREFETCH),
        pciFunction->hMmio0, nullptr);
    AssertLogRelRCReturn(rc, rc);


    /* Second region. */
    RTStrPrintf(pciFunction->szMmio2, sizeof(pciFunction->szMmio2), "PG-F%d-BAR2", 0);
    rc = PDMDevHlpMmioCreate(deviceInstance, secondBAR, pciDevice, 2 << 16 /*iPciRegion*/,
        nullptr, nullptr, pciFunction,
        IOMMMIO_FLAGS_READ_DWORD | IOMMMIO_FLAGS_WRITE_DWORD_ZEROED, pciFunction->szMmio2, &pciFunction->hMmio2);
    AssertLogRelRCReturn(rc, rc);

    rc = PDMDevHlpPCIIORegionRegisterMmioEx(deviceInstance, pciDevice, 2, secondBAR,
        static_cast<PCIADDRESSSPACE>(PCI_ADDRESS_SPACE_MEM | PCI_ADDRESS_SPACE_BAR32),
        pciFunction->hMmio2, nullptr);
    AssertLogRelRCReturn(rc, rc);

    /*
     * Save state handling.
     */
    rc = PDMDevHlpSSMRegister(deviceInstance, SOFT_GPU_SSM_VERSION, sizeof(*device), nullptr, nullptr);
    if(RT_FAILURE(rc))
    {
        return rc;
    }

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) softGpuDestruct(PPDMDEVINS deviceInstance)
{
    /*
     * Check the versions here as well since the destructor is *always* called.
     * THIS IS ALWAYS THE FIRST STATEMENT IN A DESTRUCTOR!
     */
    PDMDEV_CHECK_VERSIONS_RETURN_QUIET(deviceInstance);

    return VINF_SUCCESS;
}

/**
 * The device registration structure.
 */
static const PDMDEVREG g_DeviceSoftGpu =
{
    /* .u32Version = */             PDM_DEVREG_VERSION,
    /* .uReserved0 = */             0,
    /* .szName = */                 "softgpu",
    /* .fFlags = */                 PDM_DEVREG_FLAGS_DEFAULT_BITS | PDM_DEVREG_FLAGS_NEW_STYLE,
    /* .fClass = */                 PDM_DEVREG_CLASS_MISC,
    /* .cMaxInstances = */          1,
    /* .uSharedVersion = */         42,
    /* .cbInstanceShared = */       sizeof(SoftGpuDevice),
    /* .cbInstanceCC = */           0,
    /* .cbInstanceRC = */           0,
    /* .cMaxPciDevices = */         1,
    /* .cMaxMsixVectors = */        0,
    /* .pszDescription = */         "SoftGpu Device.",
#if defined(IN_RING3)
    /* .pszRCMod = */               "",
    /* .pszR0Mod = */               "",
    /* .pfnConstruct = */           softGpuConstruct,
    /* .pfnDestruct = */            softGpuDestruct,
    /* .pfnRelocate = */            NULL,
    /* .pfnMemSetup = */            NULL,
    /* .pfnPowerOn = */             NULL,
    /* .pfnReset = */               NULL,
    /* .pfnSuspend = */             NULL,
    /* .pfnResume = */              NULL,
    /* .pfnAttach = */              NULL,
    /* .pfnDetach = */              NULL,
    /* .pfnQueryInterface = */      NULL,
    /* .pfnInitComplete = */        NULL,
    /* .pfnPowerOff = */            NULL,
    /* .pfnSoftReset = */           NULL,
    /* .pfnReserved0 = */           NULL,
    /* .pfnReserved1 = */           NULL,
    /* .pfnReserved2 = */           NULL,
    /* .pfnReserved3 = */           NULL,
    /* .pfnReserved4 = */           NULL,
    /* .pfnReserved5 = */           NULL,
    /* .pfnReserved6 = */           NULL,
    /* .pfnReserved7 = */           NULL,
#elif defined(IN_RING0)
    /* .pfnEarlyConstruct = */      NULL,
    /* .pfnConstruct = */           NULL,
    /* .pfnDestruct = */            NULL,
    /* .pfnFinalDestruct = */       NULL,
    /* .pfnRequest = */             NULL,
    /* .pfnReserved0 = */           NULL,
    /* .pfnReserved1 = */           NULL,
    /* .pfnReserved2 = */           NULL,
    /* .pfnReserved3 = */           NULL,
    /* .pfnReserved4 = */           NULL,
    /* .pfnReserved5 = */           NULL,
    /* .pfnReserved6 = */           NULL,
    /* .pfnReserved7 = */           NULL,
#elif defined(IN_RC)
    /* .pfnConstruct = */           NULL,
    /* .pfnReserved0 = */           NULL,
    /* .pfnReserved1 = */           NULL,
    /* .pfnReserved2 = */           NULL,
    /* .pfnReserved3 = */           NULL,
    /* .pfnReserved4 = */           NULL,
    /* .pfnReserved5 = */           NULL,
    /* .pfnReserved6 = */           NULL,
    /* .pfnReserved7 = */           NULL,
#else
# error "Not in IN_RING3, IN_RING0 or IN_RC!"
#endif
    /* .u32VersionEnd = */          PDM_DEVREG_VERSION
};

extern "C" DECLEXPORT(int) VBoxDevicesRegister(PPDMDEVREGCB pCallbacks, uint32_t u32Version)
{
    LogFlow(("VBoxSoftGpuEmulator::VBoxDevicesRegister: u32Version=%#x pCallbacks->u32Version=%#x\n", u32Version, pCallbacks->u32Version));

    AssertLogRelMsgReturn(u32Version >= VBOX_VERSION, ("VirtualBox version %#x, expected %#x or higher\n", u32Version, VBOX_VERSION), VERR_VERSION_MISMATCH);
    AssertLogRelMsgReturn(pCallbacks->u32Version == PDM_DEVREG_CB_VERSION, ("Callbacks version %#x, expected %#x or higher\n", pCallbacks->u32Version, PDM_DEVREG_CB_VERSION), VERR_VERSION_MISMATCH);

    return pCallbacks->pfnRegister(pCallbacks, &g_DeviceSoftGpu);
}
