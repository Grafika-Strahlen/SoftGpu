// ReSharper disable CppEqualOperandsInBinaryExpression
#define LOG_GROUP LOG_GROUP_MISC

#define RT_STRICT
#include <VBox/com/assert.h>
#include <VBox/com/defs.h>
#include <VBox/com/Guid.h>
#include <VBox/vmm/pdmdrv.h>
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

/**
 * The driver registration structure.
 */
static constexpr PDMDRVREG g_DriverSoftGpu =
{
    /* .u32Version = */             PDM_DRVREG_VERSION,
    /* .szName = */                 "softgpudriver",
    /* .szRCMod = */                "",
    /* .szR0Mod = */                "",
    /* .pszDescription = */         "SoftGPU Driver",
    /* .fFlags = */                 PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* .fClass = */                 PDM_DRVREG_CLASS_DISPLAY,
    /* .cMaxInstances = */          ~0U,
    /* .cbInstance = */             sizeof(SoftGpuDevice),
    /* .pfnConstruct = */           nullptr,
    /* .pfnDestruct = */            nullptr,
    /* .pfnRelocate = */            nullptr,
    /* .pfnIOCtl = */               nullptr,
    /* .pfnPowerOn = */             nullptr,
    /* .pfnReset = */               nullptr,
    /* .pfnSuspend = */             nullptr,
    /* .pfnResume = */              nullptr,
    /* .pfnAttach = */              nullptr,
    /* .pfnDetach = */              nullptr,
    /* .pfnPowerOff = */            nullptr,
    /* .pfnSoftReset = */           nullptr,
    /* .u32VersionEnd = */          PDM_DRVREG_VERSION
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

[[maybe_unused]] static DECLCALLBACK(int) preRegisterTest(PCPDMDRVREGCB pCallbacks, PCPDMDRVREG pReg)
{
    /*
     * Validate the registration structure.
     */
    AssertPtrReturn(pReg, VERR_INVALID_POINTER);
    AssertLogRelMsgReturn(pReg->u32Version == PDM_DRVREG_VERSION,
        ("%#x\n", pReg->u32Version),
        VERR_PDM_UNKNOWN_DRVREG_VERSION);
    AssertReturn(pReg->szName[0], VERR_PDM_INVALID_DRIVER_REGISTRATION);
    AssertLogRelMsgReturn(RTStrEnd(pReg->szName, sizeof(pReg->szName)),
        ("%.*s\n", sizeof(pReg->szName), pReg->szName),
        VERR_PDM_INVALID_DRIVER_REGISTRATION);
    AssertLogRelMsgReturn(pdmR3IsValidName(pReg->szName), ("%.*s\n", sizeof(pReg->szName), pReg->szName),
        VERR_PDM_INVALID_DRIVER_REGISTRATION);
    AssertLogRelMsgReturn(!(pReg->fFlags & PDM_DRVREG_FLAGS_R0)
        || (pReg->szR0Mod[0]
            && RTStrEnd(pReg->szR0Mod, sizeof(pReg->szR0Mod))),
        ("%s: %.*s\n", pReg->szName, sizeof(pReg->szR0Mod), pReg->szR0Mod),
        VERR_PDM_INVALID_DRIVER_REGISTRATION);
    AssertLogRelMsgReturn(!(pReg->fFlags & PDM_DRVREG_FLAGS_RC)
        || (pReg->szRCMod[0]
            && RTStrEnd(pReg->szRCMod, sizeof(pReg->szRCMod))),
        ("%s: %.*s\n", pReg->szName, sizeof(pReg->szRCMod), pReg->szRCMod),
        VERR_PDM_INVALID_DRIVER_REGISTRATION);
    AssertLogRelMsgReturn(VALID_PTR(pReg->pszDescription),
        ("%s: %p\n", pReg->szName, pReg->pszDescription),
        VERR_PDM_INVALID_DRIVER_REGISTRATION);
    AssertLogRelMsgReturn(!(pReg->fFlags & ~(PDM_DRVREG_FLAGS_HOST_BITS_MASK | PDM_DRVREG_FLAGS_R0 | PDM_DRVREG_FLAGS_RC)),
        ("%s: %#x\n", pReg->szName, pReg->fFlags),
        VERR_PDM_INVALID_DRIVER_REGISTRATION);
    AssertLogRelMsgReturn((pReg->fFlags & PDM_DRVREG_FLAGS_HOST_BITS_MASK) == PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
        ("%s: %#x\n", pReg->szName, pReg->fFlags),
        VERR_PDM_INVALID_DRIVER_HOST_BITS);
    AssertLogRelMsgReturn(pReg->cMaxInstances > 0,
        ("%s: %#x\n", pReg->szName, pReg->cMaxInstances),
        VERR_PDM_INVALID_DRIVER_REGISTRATION);
    AssertLogRelMsgReturn(pReg->cbInstance <= _1M,
        ("%s: %#x\n", pReg->szName, pReg->cbInstance),
        VERR_PDM_INVALID_DRIVER_REGISTRATION);
    AssertLogRelMsgReturn(VALID_PTR(pReg->pfnConstruct),
        ("%s: %p\n", pReg->szName, pReg->pfnConstruct),
        VERR_PDM_INVALID_DRIVER_REGISTRATION);
    AssertLogRelMsgReturn(VALID_PTR(pReg->pfnRelocate) || !(pReg->fFlags & PDM_DRVREG_FLAGS_RC),
        ("%s: %#x\n", pReg->szName, pReg->cbInstance),
        VERR_PDM_INVALID_DRIVER_REGISTRATION);
    AssertLogRelMsgReturn(pReg->pfnSoftReset == NULL,
        ("%s: %p\n", pReg->szName, pReg->pfnSoftReset),
        VERR_PDM_INVALID_DRIVER_REGISTRATION);
    AssertLogRelMsgReturn(pReg->u32VersionEnd == PDM_DRVREG_VERSION,
        ("%s: %#x\n", pReg->szName, pReg->u32VersionEnd),
        VERR_PDM_INVALID_DRIVER_REGISTRATION);

    return VINF_SUCCESS;
}

extern "C" DECLEXPORT(int) VBoxDriversRegister(PCPDMDRVREGCB pCallbacks, const uint32_t u32Version)
{
    LogRelFlow(("VBoxSoftGpuEmulator::VBoxDriversRegister: u32Version=%#x pCallbacks->u32Version=%#x\n", u32Version, pCallbacks->u32Version));

    Console::Create();
    Console::Init();

    ConPrinter::PrintLn(u8"VBoxSoftGpuEmulator::VBoxDriversRegister: u32Version={X} pCallbacks->u32Version={X}", u32Version, pCallbacks->u32Version);

    AssertLogRelMsgReturn(u32Version >= VBOX_VERSION, ("VirtualBox version %#x, expected %#x or higher\n", u32Version, VBOX_VERSION), VERR_VERSION_MISMATCH);
    AssertLogRelMsgReturn(pCallbacks->u32Version == PDM_DRVREG_CB_VERSION, ("Callbacks version %#x, expected %#x or higher\n", pCallbacks->u32Version, PDM_DRVREG_CB_VERSION), VERR_VERSION_MISMATCH);
    
    // preRegisterTest(pCallbacks, &g_DriverSoftGpu);
    //
    // const int registerRC = pCallbacks->pfnRegister(pCallbacks, &g_DriverSoftGpu);
    //
    // const PCRTSTATUSMSG msg = RTErrGet(registerRC);
    //
    // ConLogLn(u8"VBoxSoftGpuEmulator::VBoxDevicesRegister: pfnRegister: [{}] {}, {}", registerRC, msg->pszDefine, msg->pszMsgFull);

    return VINF_SUCCESS;
}
