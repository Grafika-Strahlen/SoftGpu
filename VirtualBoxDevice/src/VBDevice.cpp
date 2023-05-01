// ReSharper disable CppEqualOperandsInBinaryExpression
#define LOG_GROUP LOG_GROUP_MISC

#define RT_STRICT
#include <VBox/com/assert.h>
#include <VBox/com/defs.h>
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/version.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <VBox/msi.h>

#include <VBox/com/string.h>
#include <VBox/com/VirtualBox.h>

#include <iprt/ctype.h>
#include <iprt/assert.h>

#include <Processor.hpp>
#include <DebugManager.hpp>
#include <Console.hpp>
#include "ConLogger.hpp"
#include "VBoxDeviceFunction.hpp"
#include "PciConfigHandler.hpp"

DebugManager GlobalDebug;

#undef PDMPCIDEV_ASSERT_VALID

#define PDMPCIDEV_ASSERT_VALID(a_pDevIns, a_pPciDev) \
    do { \
        uintptr_t const offPciDevInTable = reinterpret_cast<uintptr_t>(a_pPciDev) - reinterpret_cast<uintptr_t>(a_pDevIns->apPciDevs[0]); \
        uint32_t  const cbPciDevTmp      = a_pDevIns->cbPciDev; \
        ASMCompilerBarrier(); \
        AssertMsg(   offPciDevInTable < static_cast<uintptr_t>(a_pDevIns->cPciDevs * cbPciDevTmp) \
                  && cbPciDevTmp >= RT_UOFFSETOF(PDMPCIDEV, abConfig) + 256 \
                  && offPciDevInTable % cbPciDevTmp == 0, \
                  ("pPciDev=%p apPciDevs[0]=%p offPciDevInTable=%p cPciDevs=%#x cbPciDev=%#x\n", \
                   (a_pPciDev), a_pDevIns->apPciDevs[0], offPciDevInTable, a_pDevIns->cPciDevs, cbPciDevTmp)); \
        AssertPtr((a_pPciDev)); \
        AssertMsg((a_pPciDev)->u32Magic == PDMPCIDEV_MAGIC, ("%#x\n", (a_pPciDev)->u32Magic)); \
    } while (false)

[[maybe_unused]] static DECLCALLBACK(void*) softGpuPortQueryInterface(PPDMIBASE pInterface, const char* const iid)
{
    (void) pInterface;
    (void) iid;

    return nullptr;
}

static DECLCALLBACK(VBOXSTRICTRC) softGpuMMIORead(PPDMDEVINS pDevIns, void* pvUser, RTGCPHYS off, void* pv, unsigned cb)
{
    SoftGpuDeviceFunction* pFun = static_cast<SoftGpuDeviceFunction*>(pvUser);
    NOREF(pDevIns);

#ifdef LOG_ENABLED
    // unsigned const cbLog = cb;
    // const RTGCPHYS       offLog = off;
#endif
    // uint8_t* pbDst = static_cast<uint8_t*>(pv);
    // while(cb-- > 0)
    // {
    //     *pbDst = pFun->abBacking[off % std::size(pFun->abBacking)];
    //     pbDst++;
    //     off++;
    // }
    
    // ConLogLn("SoftGpu/[{}]: READ off=0x{XP0} cb={}: 0x{XP0}\n", pFun->FunctionId, off, cb, cb, pv);

    pFun->processor.PciMemRead(off, static_cast<u16>(cb), reinterpret_cast<u32*>(pv));

    return VINF_SUCCESS;
}

static DECLCALLBACK(VBOXSTRICTRC) softGpuMMIOWrite(PPDMDEVINS pDevIns, void* pvUser, RTGCPHYS off, void const* pv, unsigned cb)
{
    SoftGpuDeviceFunction* pFun = static_cast<SoftGpuDeviceFunction*>(pvUser);
    NOREF(pDevIns);
    // ConLogLn("SoftGpu/[{}]: WRITE off=0x{XP0} cb={}: 0x{XP0}\n", pFun->FunctionId, off, cb, cb, pv);

    // uint8_t const* pbSrc = static_cast<uint8_t const*>(pv);
    // while(cb-- > 0)
    // {
    //     pFun->abBacking[off % std::size(pFun->abBacking)] = *pbSrc;
    //     pbSrc++;
    //     off++;
    // }

    pFun->processor.PciMemWrite(off, static_cast<u16>(cb), reinterpret_cast<const u32*>(pv));

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) softGpuMMIOMapUnmap(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion, RTGCPHYS GCPhysAddress, RTGCPHYS cb, PCIADDRESSSPACE enmType)
{
    (void) pPciDev;

    const SoftGpuDevice* device = PDMDEVINS_2_DATA(pDevIns, SoftGpuDevice*);
    const SoftGpuDeviceFunction& pciFunction = device->pciFunction;

    if(GCPhysAddress != NIL_RTGCPHYS)
    {
        ConLogLn(u8"SoftGpu/[{}]: Mapping MMIO Region {}: Address = 0x{XP0}, Size = 0x{XP0}, BAR Flags: 0x{XP0}", pciFunction.FunctionId, iRegion, GCPhysAddress, cb, static_cast<u8>(enmType));
    }
    else
    {
        ConLogLn(u8"SoftGpu/[{}]: Unmapping MMIO Region {}: Address = 0x{XP0}, Size = 0x{XP0}, BAR Flags: 0x{XP0}", pciFunction.FunctionId, iRegion, GCPhysAddress, cb, static_cast<u8>(enmType));
    }

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) softGpuConstruct(PPDMDEVINS deviceInstance, int instance, PCFGMNODE cfg)
{
    /*
     * Check that the device instance and device helper structures are compatible.
     * THIS IS ALWAYS THE FIRST STATEMENT IN A CONSTRUCTOR!
     */
    PDMDEV_CHECK_VERSIONS_RETURN(deviceInstance); /* This must come first. */
    Assert(instance == 0); RT_NOREF(instance);

    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: u32Version={XP}, iInstance={}", deviceInstance->u32Version, deviceInstance->iInstance);
    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Function Address: {}", reinterpret_cast<void*>(softGpuConstruct));

    /*
     * Initialize the instance data so that the destructor won't mess up.
     */
    SoftGpuDevice* device = PDMDEVINS_2_DATA(deviceInstance, SoftGpuDevice*);

    /*
     * Validate and read the configuration.
     */
    PDMDEV_VALIDATE_CONFIG_RETURN(deviceInstance, "ConfigBAR0MB|FrameBufferBAR1GB", "");

    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Validated config.");

    const PCPDMDEVHLPR3 pdmDeviceApi = deviceInstance->pHlpR3;

    AssertCompile(RT_ELEMENTS(deviceInstance->apPciDevs) >= 1);

    uint16_t configBAR0MB;
    int rc = pdmDeviceApi->pfnCFGMQueryU16Def(cfg, "ConfigBAR0MB", &configBAR0MB, 16);  /* Default to 16 MiB. */

    if(RT_FAILURE(rc))
    {
        return PDMDEV_SET_ERROR(deviceInstance, rc, N_("Configuration error: Failed to query integer value \"ConfigBAR0MB\""));
    }

    if(configBAR0MB < 16 && configBAR0MB > 0)
    {
        return PDMDEV_SET_ERROR(deviceInstance, rc, N_("Configuration error: Invalid \"ConfigBAR0MB\" value (must be at least 16)"));
    }

    if(configBAR0MB > 512)
    {
        return PDMDEV_SET_ERROR(deviceInstance, rc, N_("Configuration error: Invalid \"ConfigBAR0MB\" value (must be 512 or less)"));
    }

    RTGCPHYS firstBAR = 16ull * _1M;

    if(configBAR0MB)
    {
        firstBAR = static_cast<RTGCPHYS>(configBAR0MB) * _1M;
    }

    uint16_t frameBufferBAR1GB;
    rc = pdmDeviceApi->pfnCFGMQueryU16Def(cfg, "FrameBufferBAR1GB", &frameBufferBAR1GB, 2);  /* Default to 2 GiB. */

    if(RT_FAILURE(rc))
    {
        return PDMDEV_SET_ERROR(deviceInstance, rc, N_("Configuration error: Failed to query integer value \"FrameBufferBAR1GB\""));
    }

    if(frameBufferBAR1GB < 2 && frameBufferBAR1GB > 0)
    {
        return PDMDEV_SET_ERROR(deviceInstance, rc, N_("Configuration error: Invalid \"FrameBufferBAR1GB\" value (must be at least 2)"));
    }

    if(frameBufferBAR1GB > 16)
    {
        return PDMDEV_SET_ERROR(deviceInstance, rc, N_("Configuration error: Invalid \"FrameBufferBAR1GB\" value (must be 16 or less)"));
    }

    RTGCPHYS secondBAR = 256ull * _1M;
    // if(frameBufferBAR1GB)
    // {
    //     secondBAR = static_cast<RTGCPHYS>(frameBufferBAR1GB) * _1G64;
    // }

    ConLogLn(u8"VBoxSoftGpuEmulator::softGpuConstruct: BAR0 Size: 0x{XP0}", firstBAR);
    ConLogLn(u8"VBoxSoftGpuEmulator::softGpuConstruct: BAR1 Size: 0x{XP0}", secondBAR);

    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Read config.");

    /*
     * PCI device setup.
     */
    PPDMPCIDEV pciDevice = deviceInstance->apPciDevs[0];
    SoftGpuDeviceFunction* pciFunction = &device->pciFunction;
    RTStrPrintf(pciFunction->FunctionName, sizeof(pciFunction->FunctionName), "soft_gpu%u", 0);
    pciFunction->FunctionId = 0;
    ::new(&pciFunction->processor) Processor;

    PDMPCIDEV_ASSERT_VALID(deviceInstance, pciDevice);
    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Asserted pciDevice as valid.");

    if constexpr(false)
    {
        ConLogLn("Setting PCI config info through VBox functions.");
        PDMPciDevSetVendorId(pciDevice, 0xFFFD);
        PDMPciDevSetDeviceId(pciDevice, 0x0001);
        PDMPciDevSetClassBase(pciDevice, 0x03);  /* display controller device */
        PDMPciDevSetClassSub(pciDevice, 0x00);  /* VGA compatible controller device */
        PDMPciDevSetHeaderType(pciDevice, 0x00);  /* normal, single function device */
    }
    else
    {
        static_assert(sizeof(PciController) == sizeof(pciDevice->abConfig), "SoftGpu PCI config space does not match size of the VBox backing storage.");

        ConLogLn("Setting PCI config info by copying the PCIController.");

        (void) ::std::memcpy(pciDevice->abConfig, &pciFunction->processor.GetPciController(), sizeof(pciDevice->abConfig));

        ConLogLn("VBox PCI Vendor ID: {X}", PDMPciDevGetVendorId(pciDevice));
        ConLogLn("SoftGpu PCI Vendor ID: {X}", pciFunction->processor.PciConfigRead(0, 2));
    }

    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Set PCI info.");

    // pciFunction->IBase.pfnQueryInterface = nullptr;
    //
    // pciFunction->IDisplayPort.pfnUpdateDisplay = nullptr;
    // pciFunction->IDisplayPort.pfnUpdateDisplayAll = nullptr;
    // pciFunction->IDisplayPort.pfnQueryVideoMode = nullptr;
    // pciFunction->IDisplayPort.pfnSetRefreshRate = nullptr;
    // pciFunction->IDisplayPort.pfnTakeScreenshot = nullptr;
    // pciFunction->IDisplayPort.pfnFreeScreenshot = nullptr;
    // pciFunction->IDisplayPort.pfnDisplayBlt = nullptr;
    // pciFunction->IDisplayPort.pfnUpdateDisplayRect = nullptr;
    // pciFunction->IDisplayPort.pfnCopyRect = nullptr;
    // pciFunction->IDisplayPort.pfnSetRenderVRAM = nullptr;
    //
    // pciFunction->IDisplayPort.pfnSetViewport = nullptr;
    // pciFunction->IDisplayPort.pfnReportMonitorPositions = nullptr;
    //
    // pciFunction->IDisplayPort.pfnSendModeHint = nullptr;
    // pciFunction->IDisplayPort.pfnReportHostCursorCapabilities = nullptr;
    // pciFunction->IDisplayPort.pfnReportHostCursorPosition = nullptr;

    rc = PDMDevHlpPCIRegisterEx(deviceInstance, pciDevice, 0 /*fFlags*/, PDMPCIDEVREG_DEV_NO_FIRST_UNUSED, 0, device->pciFunction.FunctionName);
    AssertLogRelRCReturn(rc, rc);

    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Registered PCI device.");

    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Registering config interceptor.");
    // Register a interceptor for PCI config reads and writes.
    rc = PDMDevHlpPCIInterceptConfigAccesses(deviceInstance, pciDevice, SoftGpuConfigRead, SoftGpuConfigWrite);
    AssertLogRelRCReturn(rc, rc);
    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Registered config interceptor.");

    /* BAR0 region. */
    RTStrPrintf(pciFunction->MmioBar0Name, sizeof(pciFunction->MmioBar0Name), "SG-F%d-BAR0", 0);

    rc = PDMDevHlpMmioCreate(deviceInstance, firstBAR, pciDevice, 0 /*iPciRegion*/,
        softGpuMMIOWrite, softGpuMMIORead, pciFunction,
        IOMMMIO_FLAGS_READ_DWORD | IOMMMIO_FLAGS_WRITE_DWORD_ZEROED | IOMMMIO_FLAGS_ABS, pciFunction->MmioBar0Name, &pciFunction->hMmioBar0);
    AssertLogRelRCReturn(rc, rc);
    
    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Created BAR0 region.");
    
    rc = PDMDevHlpPCIIORegionRegisterMmioEx(deviceInstance, pciDevice, 0, firstBAR,
        static_cast<PCIADDRESSSPACE>(PCI_ADDRESS_SPACE_MEM | PCI_ADDRESS_SPACE_BAR32),
        pciFunction->hMmioBar0, softGpuMMIOMapUnmap);
    AssertLogRelRCReturn(rc, rc);
    
    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Registered MMIO function for BAR0 region.");

    /* BAR1 region. */
    RTStrPrintf(pciFunction->MmioBar1Name, sizeof(pciFunction->MmioBar1Name), "SG-F%d-BAR1", 0);

    rc = PDMDevHlpMmioCreate(deviceInstance, secondBAR, pciDevice, 1 << 16 /*iPciRegion*/,
        softGpuMMIOWrite, softGpuMMIORead, pciFunction,
        IOMMMIO_FLAGS_READ_DWORD | IOMMMIO_FLAGS_WRITE_DWORD_ZEROED | IOMMMIO_FLAGS_ABS, pciFunction->MmioBar1Name, &pciFunction->hMmioBar1);
    AssertLogRelRCReturn(rc, rc);
    
    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Created BAR1 region.");

    rc = PDMDevHlpPCIIORegionRegisterMmioEx(deviceInstance, pciDevice, 1, secondBAR,
        static_cast<PCIADDRESSSPACE>(PCI_ADDRESS_SPACE_MEM | PCI_ADDRESS_SPACE_BAR64 | PCI_ADDRESS_SPACE_MEM_PREFETCH),
        pciFunction->hMmioBar1, softGpuMMIOMapUnmap);
    AssertLogRelRCReturn(rc, rc);
    
    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Registered MMIO function BAR1 region.");

    /* Expansion ROM region. */
    RTStrPrintf(pciFunction->MmioExpansionRomName, sizeof(pciFunction->MmioExpansionRomName), "SG-F%d-ROM", 0);

    rc = PDMDevHlpMmioCreate(deviceInstance, 32ull * 1024, pciDevice, 6 << 16 /*iPciRegion*/,
        softGpuMMIOWrite, softGpuMMIORead, pciFunction,
        IOMMMIO_FLAGS_WRITE_PASSTHRU | IOMMMIO_FLAGS_READ_PASSTHRU | IOMMMIO_FLAGS_ABS, pciFunction->MmioExpansionRomName, &pciFunction->hMmioExpansionRom);
    AssertLogRelRCReturn(rc, rc);
    
    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Created Expansion ROM region.");
    
    rc = PDMDevHlpPCIIORegionRegisterMmioEx(deviceInstance, pciDevice, 6, 32ull * 1024,
        static_cast<PCIADDRESSSPACE>(PCI_ADDRESS_SPACE_MEM | PCI_ADDRESS_SPACE_MEM_PREFETCH),
        pciFunction->hMmioExpansionRom, softGpuMMIOMapUnmap);
    AssertLogRelRCReturn(rc, rc);
    
    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Registered MMIO function Expansion ROM region.");

    /*
     * Save state handling.
     */
    rc = PDMDevHlpSSMRegister(deviceInstance, SOFT_GPU_SSM_VERSION, sizeof(*device), nullptr, nullptr);
    AssertLogRelRCReturn(rc, rc);

    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Registered saves state handler.");
    
    // rc = PDMDevHlpDriverAttach(deviceInstance, 0, &pciFunction->IBase, &pciFunction->pDrvBase, "Display Port");

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

    ConLogLn(u8"VBoxSoftGpuEmulator::softGpuDestruct: deviceInstance->u32Version={X} deviceInstance->iInstance={}", deviceInstance->u32Version, deviceInstance->iInstance);

    SoftGpuDevice* device = PDMDEVINS_2_DATA(deviceInstance, SoftGpuDevice*);
    SoftGpuDeviceFunction& pciFunction = device->pciFunction;

    pciFunction.processor.~Processor();

    return VINF_SUCCESS;
}

/**
 * The device registration structure.
 */
static constexpr PDMDEVREG g_DeviceSoftGpu =
{
    /* .u32Version = */             PDM_DEVREG_VERSION,
    /* .uReserved0 = */             0,
    /* .szName = */                 "softgpu",
    /* .fFlags = */                 PDM_DEVREG_FLAGS_DEFAULT_BITS | PDM_DEVREG_FLAGS_NEW_STYLE,
    /* .fClass = */                 PDM_DEVREG_CLASS_GRAPHICS,
    /* .cMaxInstances = */          1,
    /* .uSharedVersion = */         1,
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
    /* .pfnRelocate = */            nullptr,
    /* .pfnMemSetup = */            nullptr,
    /* .pfnPowerOn = */             nullptr,
    /* .pfnReset = */               nullptr,
    /* .pfnSuspend = */             nullptr,
    /* .pfnResume = */              nullptr,
    /* .pfnAttach = */              nullptr,
    /* .pfnDetach = */              nullptr,
    /* .pfnQueryInterface = */      nullptr,
    /* .pfnInitComplete = */        nullptr,
    /* .pfnPowerOff = */            nullptr,
    /* .pfnSoftReset = */           nullptr,
    /* .pfnReserved0 = */           nullptr,
    /* .pfnReserved1 = */           nullptr,
    /* .pfnReserved2 = */           nullptr,
    /* .pfnReserved3 = */           nullptr,
    /* .pfnReserved4 = */           nullptr,
    /* .pfnReserved5 = */           nullptr,
    /* .pfnReserved6 = */           nullptr,
    /* .pfnReserved7 = */           nullptr,
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

static bool pdmR3IsValidName(const char* pszName)
{
    char ch;
    while((ch = *pszName) != '\0'
        && (RT_C_IS_ALNUM(ch)
            || ch == '-'
            || ch == '_'))
        pszName++;
    return ch == '\0';
}

/** The maximum device instance (total) size, ring-0/raw-mode capable devices. */
#define PDM_MAX_DEVICE_INSTANCE_SIZE    _4M
/** The maximum device instance (total) size, ring-3 only devices. */
#define PDM_MAX_DEVICE_INSTANCE_SIZE_R3 _8M

[[maybe_unused]] static DECLCALLBACK(int) preRegisterTest(PPDMDEVREGCB pCallbacks, PCPDMDEVREG pReg)
{
    (void) pCallbacks;

    /*
     * Validate the registration structure.
     */
    Assert(pReg);
    AssertLogRelMsgReturn(pReg->u32Version == PDM_DEVREG_VERSION,
        ("Unknown struct version %#x!\n", pReg->u32Version),
        VERR_PDM_UNKNOWN_DEVREG_VERSION);

    AssertLogRelMsgReturn(pReg->szName[0]
        && strlen(pReg->szName) < sizeof(pReg->szName)
        && pdmR3IsValidName(pReg->szName),
        ("Invalid name '%.*s'\n", sizeof(pReg->szName), pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertLogRelMsgReturn(!(pReg->fFlags & PDM_DEVREG_FLAGS_RC)
        || (pReg->pszRCMod[0]
            && strlen(pReg->pszRCMod) < RT_SIZEOFMEMB(PDMDEVICECREATEREQ, szModName)),
        ("Invalid GC module name '%s' - (Device %s)\n", pReg->pszRCMod, pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertLogRelMsgReturn(!(pReg->fFlags & PDM_DEVREG_FLAGS_R0)
        || (pReg->pszR0Mod[0]
            && strlen(pReg->pszR0Mod) < RT_SIZEOFMEMB(PDMDEVICECREATEREQ, szModName)),
        ("Invalid R0 module name '%s' - (Device %s)\n", pReg->pszR0Mod, pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertLogRelMsgReturn((pReg->fFlags & PDM_DEVREG_FLAGS_HOST_BITS_MASK) == PDM_DEVREG_FLAGS_HOST_BITS_DEFAULT,
        ("Invalid host bits flags! fFlags=%#x (Device %s)\n", pReg->fFlags, pReg->szName),
        VERR_PDM_INVALID_DEVICE_HOST_BITS);
    AssertLogRelMsgReturn((pReg->fFlags & PDM_DEVREG_FLAGS_GUEST_BITS_MASK),
        ("Invalid guest bits flags! fFlags=%#x (Device %s)\n", pReg->fFlags, pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertLogRelMsgReturn(pReg->fClass,
        ("No class! (Device %s)\n", pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertLogRelMsgReturn(pReg->cMaxInstances > 0,
        ("Max instances %u! (Device %s)\n", pReg->cMaxInstances, pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);
    uint32_t const cbMaxInstance = pReg->fFlags & (PDM_DEVREG_FLAGS_RC | PDM_DEVREG_FLAGS_R0)
        ? PDM_MAX_DEVICE_INSTANCE_SIZE : PDM_MAX_DEVICE_INSTANCE_SIZE_R3;
    AssertLogRelMsgReturn(pReg->cbInstanceShared <= cbMaxInstance,
        ("Instance size %u bytes! (Max %u; Device %s)\n", pReg->cbInstanceShared, cbMaxInstance, pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertLogRelMsgReturn(pReg->cbInstanceCC <= cbMaxInstance,
        ("Instance size %d bytes! (Max %u; Device %s)\n", pReg->cbInstanceCC, cbMaxInstance, pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertLogRelMsgReturn(pReg->pfnConstruct,
        ("No constructor! (Device %s)\n", pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertLogRelMsgReturn((pReg->fFlags & PDM_DEVREG_FLAGS_GUEST_BITS_MASK) == PDM_DEVREG_FLAGS_GUEST_BITS_DEFAULT,
        ("PDM: Rejected device '%s' because it didn't match the guest bits.\n", pReg->szName),
        VERR_PDM_INVALID_DEVICE_GUEST_BITS);
    AssertLogRelMsg(pReg->u32VersionEnd == PDM_DEVREG_VERSION,
        ("u32VersionEnd=%#x, expected %#x. (szName=%s)\n",
            pReg->u32VersionEnd, PDM_DEVREG_VERSION, pReg->szName));
    AssertLogRelMsgReturn(pReg->cMaxPciDevices <= 8, ("%#x (szName=%s)\n", pReg->cMaxPciDevices, pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertLogRelMsgReturn(pReg->cMaxMsixVectors <= VBOX_MSIX_MAX_ENTRIES,
        ("%#x (szName=%s)\n", pReg->cMaxMsixVectors, pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertLogRelMsgReturn(pReg->fFlags & PDM_DEVREG_FLAGS_NEW_STYLE /* the flag is required now */,
        ("PDM_DEVREG_FLAGS_NEW_STYLE not set for szName=%s!\n", pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);

    return 0;
}

extern "C" DECLEXPORT(int) VBoxDevicesRegister(PPDMDEVREGCB pCallbacks, const uint32_t u32Version)
{
    LogRelFlow(("VBoxSoftGpuEmulator::VBoxDevicesRegister: u32Version=%#x pCallbacks->u32Version=%#x\n", u32Version, pCallbacks->u32Version));

    Console::Create();
    Console::Init();
    
    ConPrinter::PrintLn(u8"VBoxSoftGpuEmulator::VBoxDevicesRegister: u32Version=0x{X} pCallbacks->u32Version=0x{X}", u32Version, pCallbacks->u32Version);

    AssertLogRelMsgReturn(u32Version >= VBOX_VERSION, ("VirtualBox version %#x, expected %#x or higher\n", u32Version, VBOX_VERSION), VERR_VERSION_MISMATCH);
    AssertLogRelMsgReturn(pCallbacks->u32Version == PDM_DEVREG_CB_VERSION, ("Callbacks version %#x, expected %#x or higher\n", pCallbacks->u32Version, PDM_DEVREG_CB_VERSION), VERR_VERSION_MISMATCH);

    // preRegisterTest(pCallbacks, &g_DeviceSoftGpu);

    const int registerRC = pCallbacks->pfnRegister(pCallbacks, &g_DeviceSoftGpu);
    
    ConLogLn(u8"VBoxSoftGpuEmulator::VBoxDevicesRegister: pfnRegister: [{}] {}, {}", registerRC, ConLogRCDefine<c8>(registerRC), ConLogRCMsgFull<c8>(registerRC));

    return registerRC;
}

