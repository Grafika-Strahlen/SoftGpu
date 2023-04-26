#define LOG_GROUP LOG_GROUP_MISC

#define RT_STRICT
#include <VBox/com/assert.h>
#include <VBox/com/defs.h>
#include <VBox/com/Guid.h>
#include <VBox/com/string.h>
#include <VBox/com/VirtualBox.h>
#include <VBox/vmm/pdm.h>
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmdrv.h>
#include <VBox/version.h>
#include <VBox/log.h>

#include <iprt/assert.h>

#include <Windows.h>
#include <cstring>
#include "ConLogger.hpp"

typedef HMODULE(*WINAPI LoadLibraryExW_f)(LPCWSTR, HANDLE, DWORD);
typedef FARPROC(*WINAPI GetProcAddress_f)(HMODULE, LPCSTR);
typedef NTSTATUS(*NTAPI NtProtectVirtualMemory_f)(HANDLE, PVOID*, PSIZE_T, ULONG, PULONG);

static constexpr unsigned char LdrLoadDllOriginalBytes[16] = {
    0x48, 0x89, 0x5C, 0x24, 0x10,
    0x56,
    0x57,
    0x41, 0x56,
    0x48, 0x81, 0xEC, 0xD0, 0x00, 0x00, 0x00
};

static constexpr unsigned char NtCreateSectionOriginalBytes[16] = {
    0x4C, 0x8B, 0xD1,
    0xB8, 0x4A, 0x00, 0x00, 0x00,
    0xF6, 0x04, 0x25, 0x08, 0x03, 0xFE, 0x7F, 0x01
};

static HMODULE NtDll = nullptr;
static NtProtectVirtualMemory_f NtProtectVirtualMemory = nullptr;
static void* LdrLoadDll = nullptr;
static void* NtCreateSection = nullptr;
static DWORD OldProtectionLdrLoadDll = 0;
static DWORD OldProtectionNtCreateSection = 0;

static unsigned char LdrLoadDllPatchedBytes[sizeof(LdrLoadDllOriginalBytes)];
static unsigned char NtCreateSectionPatchedBytes[sizeof(NtCreateSectionOriginalBytes)];

#define NtCurrentProcess() ((HANDLE)-(intptr_t)1)
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

static constexpr const char* EnvTauVBoxDevice = "TAU_VBOX_DEVICE";
static constexpr const char* EnvTauVBoxDriver = "TAU_VBOX_DRIVER";

template<typename ToFunc, typename FromFunc>
inline ToFunc func_cast(FromFunc funcPtr) noexcept
{
    return reinterpret_cast<ToFunc>(reinterpret_cast<void*>(funcPtr));
}

static void LogWin32Error(const char* callerTag)
{
    const DWORD errorCode = GetLastError();
    const DWORD requiredLength = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, errorCode, LANG_SYSTEM_DEFAULT, nullptr, 0, nullptr);
    char* errorBuffer = new(::std::nothrow) char[requiredLength + 1];
    (void) FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, errorCode, LANG_SYSTEM_DEFAULT, errorBuffer, requiredLength + 1, nullptr);

    errorBuffer[requiredLength] = '\0';

    ConLogLn(u8"{} Error [0x{XP0}] {}\n", callerTag, errorCode, errorBuffer);

    delete[] errorBuffer;
}

static void LogVirtualProtectError()
{
    LogWin32Error("VirtualProtect");
}

static void LogLoadLibraryError()
{
    LogWin32Error("LoadLibrary");
}

static int LoadNtDll()
{
    if(!NtDll)
    {
        // Get a handle to the NTDLL.dll library.
        const HMODULE ntdll = LoadLibraryA("ntdll.dll");

        if(!ntdll)
        {
            LogLoadLibraryError();
            AssertLogRelMsgFailedReturn(("Could not load ntdll.dll\n"), VERR_NOT_FOUND);
        }

        // Get a pointer to LdrLoadDll and NtCreateSection.
        LdrLoadDll = reinterpret_cast<void*>(GetProcAddress(ntdll, "LdrLoadDll"));
        NtCreateSection = reinterpret_cast<void*>(GetProcAddress(ntdll, "NtCreateSection"));
        NtProtectVirtualMemory = func_cast<NtProtectVirtualMemory_f>(GetProcAddress(ntdll, "NtProtectVirtualMemory"));

        if(!LdrLoadDll)
        {
            AssertLogRelMsgFailedReturn(("Could not load LdrLoadDll.\n"), VERR_NOT_FOUND);
        }

        if(!NtCreateSection)
        {
            AssertLogRelMsgFailedReturn(("Could not load NtCreateSection.\n"), VERR_NOT_FOUND);
        }

        if(!NtProtectVirtualMemory)
        {
            AssertLogRelMsgFailedReturn(("Could not load NtProtectVirtualMemory.\n"), VERR_NOT_FOUND);
        }

        ConLogLn(u8"Loaded LdrLoadDll {} and NtCreateSection {}.", LdrLoadDll, NtCreateSection);
    }

    return VINF_SUCCESS;
}

static void SavePatchesAndRestoreCode() noexcept
{
    // Copy the patches.
    (void) ::std::memcpy(LdrLoadDllPatchedBytes, LdrLoadDll, sizeof(LdrLoadDllPatchedBytes));
    ConLogLn(u8"Saved patches LdrLoadDll.");

    (void) ::std::memcpy(NtCreateSectionPatchedBytes, NtCreateSection, sizeof(NtCreateSectionPatchedBytes));
    ConLogLn(u8"Saved patches NtCreateSection.");

    // Restore the original bytes.
    (void) ::std::memcpy(LdrLoadDll, LdrLoadDllOriginalBytes, sizeof(LdrLoadDllOriginalBytes));
    ConLogLn(u8"Restored original code for LdrLoadDll.");

    (void) ::std::memcpy(NtCreateSection, NtCreateSectionOriginalBytes, sizeof(NtCreateSectionOriginalBytes));
    ConLogLn(u8"Restored original code for NtCreateSection.");
}

static void RestorePatches() noexcept
{
    // Restore the hardening patches.
    (void) ::std::memcpy(LdrLoadDll, LdrLoadDllPatchedBytes, sizeof(LdrLoadDllPatchedBytes));
    ConLogLn(u8"Restored hardening patches for LdrLoadDll.");

    (void) ::std::memcpy(NtCreateSection, NtCreateSectionPatchedBytes, sizeof(NtCreateSectionPatchedBytes));
    ConLogLn(u8"Restored hardening patches for NtCreateSection.");
}

static bool SetProtection(void* codePointer, size_t size, DWORD protection, DWORD* const oldProtection)
{
    // Change the protection of LdrLoadDll
    const NTSTATUS status = NtProtectVirtualMemory(NtCurrentProcess(), &codePointer, &size, protection, oldProtection);
    if(NT_SUCCESS(status))
    {
        return true;
    }

    LogVirtualProtectError();
    return false;
}

static int DeProtectSections()
{
    __try
    {
        // Change the protection of LdrLoadDll
        if(!SetProtection(LdrLoadDll, sizeof(LdrLoadDllOriginalBytes), PAGE_EXECUTE_READWRITE, &OldProtectionLdrLoadDll))
        {
            AssertLogRelMsgFailedReturn(("Failed to set the protection for LdrLoadDll to read-write.\n"), VERR_WRITE_PROTECT);
        }

        LogRel(("Changed access protections for LdrLoadDll to read-write.\n"));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        LogRel(("Exception thrown while changing write protections for LdrLoadDll to read-write.\n"));
    }

    __try
    {
        // Change the protection of NtCreateSection
        if(!SetProtection(NtCreateSection, sizeof(NtCreateSectionOriginalBytes), PAGE_EXECUTE_READWRITE, &OldProtectionNtCreateSection))
        {
            AssertLogRelMsgFailedReturn(("Failed to set the protection for NtCreateSection to read-write.\n"), VERR_WRITE_PROTECT);
        }

        LogRel(("Changed access protections for NtCreateSection to read-write.\n"));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        LogRel(("Exception thrown while changing write protections for NtCreateSection to read-write.\n"));
    }

    return VINF_SUCCESS;
}

static int ReProtectSections()
{
    __try
    {
        // Restore execution protection for LdrLoadDll
        if(!SetProtection(LdrLoadDll, sizeof(LdrLoadDllOriginalBytes), OldProtectionLdrLoadDll, &OldProtectionLdrLoadDll))
        {
            AssertLogRelMsgFailedReturn(("Failed to restore the protection for LdrLoadDll.\n"), VERR_WRITE_PROTECT);
        }

        LogRel(("Restored access protections for LdrLoadDll.\n"));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        LogRel(("Exception thrown while restoring access protections for LdrLoadDll.\n"));
    }

    __try
    {
        // Restore execution protection for LdrLoadDll
        if(!SetProtection(NtCreateSection, sizeof(NtCreateSectionOriginalBytes), OldProtectionNtCreateSection, &OldProtectionNtCreateSection))
        {
            AssertLogRelMsgFailedReturn(("Failed to restore the protection for NtCreateSection.\n"), VERR_WRITE_PROTECT);
        }

        LogRel(("Restored access protections for NtCreateSection.\n"));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        LogRel(("Exception thrown while restoring access protections for NtCreateSection.\n"));
    }

    return VINF_SUCCESS;
}

static void FlushCaches()
{
    (void) FlushInstructionCache(NtDll, LdrLoadDll, sizeof(LdrLoadDllOriginalBytes));
    ConLogLn(u8"Flushed instruction cache for LdrLoadDll.");
    (void) FlushInstructionCache(NtDll, NtCreateSection, sizeof(NtCreateSectionOriginalBytes));
    ConLogLn(u8"Flushed instruction cache for NtCreateSection.");
}

static int PreLoadLibrary()
{
    {
        const int loadNtDllError = LoadNtDll();
        if(!RT_SUCCESS(loadNtDllError))
        {
            AssertLogRelMsgFailedReturn(("Failed to load ntdll.dll and its core functions."), loadNtDllError);
        }
    }

    {
        const int protectResult = DeProtectSections();
        if(!RT_SUCCESS(protectResult))
        {
            return protectResult;
        }
    }

    SavePatchesAndRestoreCode();

    {
        const int protectResult = ReProtectSections();
        if(!RT_SUCCESS(protectResult))
        {
            return protectResult;
        }
    }

    FlushCaches();

    return VINF_SUCCESS;
}

static int PostLoadLibrary() noexcept
{
    {
        const int protectResult = DeProtectSections();
        if(!RT_SUCCESS(protectResult))
        {
            return protectResult;
        }
    }

    RestorePatches();

    {
        const int protectResult = ReProtectSections();
        if(!RT_SUCCESS(protectResult))
        {
            return protectResult;
        }
    }

    FlushCaches();

    return VINF_SUCCESS;
}

static int LoadLibraryFromEnv(const char* targetEnv, HMODULE* module)
{
    *module = nullptr;

    size_t trueLibraryPathLength = 0;
    if(const errno_t err = getenv_s(&trueLibraryPathLength, nullptr, 0, targetEnv))
    {
        AssertLogRelMsgFailedReturn(("Initial call to getenv_s failed, Error: %d, Path Length: %u.\n", err, static_cast<unsigned>(trueLibraryPathLength)), VERR_NOT_FOUND);
    }

    if(!trueLibraryPathLength)
    {
        AssertLogRelMsgFailedReturn(("Failed to get the path length to a device to load, Path Length: %u.\n", static_cast<unsigned>(trueLibraryPathLength)), VERR_NOT_FOUND);
    }

    char* trueLibraryPathBuffer = new(::std::nothrow) char[trueLibraryPathLength];

    if(!trueLibraryPathBuffer)
    {
        AssertLogRelMsgFailedReturn(("Failed to allocate space for the device path.\n"), VERR_NO_MEMORY);
    }

    if(const errno_t err = getenv_s(&trueLibraryPathLength, trueLibraryPathBuffer, trueLibraryPathLength, targetEnv))
    {
        delete[] trueLibraryPathBuffer;
        AssertLogRelMsgFailedReturn(("Failed to get the path to a device to load with a valid buffer. Error: %d\n", err), VERR_NOT_FOUND);
    }

    // Load our real device driver.
    const HMODULE lib = LoadLibraryA(trueLibraryPathBuffer);

    if(!lib)
    {
        delete[] trueLibraryPathBuffer;
        LogLoadLibraryError();
        AssertLogRelMsgFailedReturn(("Could not load real %s\n", trueLibraryPathBuffer), VERR_NOT_FOUND);
    }

    ConLogLn(u8"Loaded real {}", trueLibraryPathBuffer);

    delete[] trueLibraryPathBuffer;

    *module = lib;

    return VINF_SUCCESS;
}

[[nodiscard]] static int GetExportAndRegisterDevices(PPDMDEVREGCB pCallbacks, const uint32_t u32Version, HMODULE vbDevice) noexcept
{
    FNPDMVBOXDEVICESREGISTER* registerDevices = func_cast<FNPDMVBOXDEVICESREGISTER*>(GetProcAddress(vbDevice, "VBoxDevicesRegister"));

    if(!registerDevices)
    {
        AssertLogRelMsgFailedReturn(("Could not load VBoxDevicesRegister\n"), VERR_NOT_FOUND);
    }

    ConLogLn(u8"Loaded real VBoxDevicesRegister: {}", reinterpret_cast<void*>(registerDevices));

    return registerDevices(pCallbacks, u32Version);
}

[[nodiscard]] static int GetExportAndRegisterDrivers(PCPDMDRVREGCB pCallbacks, const uint32_t u32Version, HMODULE vbDevice) noexcept
{
    FNPDMVBOXDRIVERSREGISTER* registerDrivers = func_cast<FNPDMVBOXDRIVERSREGISTER*>(GetProcAddress(vbDevice, "VBoxDriversRegister"));

    if(!registerDrivers)
    {
        AssertLogRelMsgFailedReturn(("Could not load VBoxDriversRegister\n"), VERR_NOT_FOUND);
    }

    ConLogLn(u8"Loaded real VBoxDriversRegister: {}", reinterpret_cast<void*>(registerDrivers));

    return registerDrivers(pCallbacks, u32Version);
}

extern "C" DECLEXPORT(int) VBoxDevicesRegister(PPDMDEVREGCB pCallbacks, const uint32_t u32Version)
{
    LogRelFlow(("VBoxSoftGpuLoader::VBoxDevicesRegister: u32Version=%#x pCallbacks->u32Version=%#x\n", u32Version, pCallbacks->u32Version));

    Console::Create();
    Console::Init();

    ConPrinter::PrintLn(u8"VBoxSoftGpuLoader::VBoxDevicesRegister: u32Version=0x{X} pCallbacks->u32Version=0x{X}", u32Version, pCallbacks->u32Version);

    AssertLogRelMsgReturn(u32Version >= VBOX_VERSION, ("VirtualBox version %#x, expected %#x or higher\n", u32Version, VBOX_VERSION), VERR_VERSION_MISMATCH);
    AssertLogRelMsgReturn(pCallbacks->u32Version == PDM_DEVREG_CB_VERSION, ("Callbacks version %#x, expected %#x or higher\n", pCallbacks->u32Version, PDM_DEVREG_CB_VERSION), VERR_VERSION_MISMATCH);

    {
        const int preLoadLibraryResult = PreLoadLibrary();
        if(!RT_SUCCESS(preLoadLibraryResult))
        {
            return preLoadLibraryResult;
        }
    }

    HMODULE vbDevice;

    {
        const int preLoadLibraryResult = LoadLibraryFromEnv(EnvTauVBoxDevice, &vbDevice);
        if(!RT_SUCCESS(preLoadLibraryResult))
        {
            return preLoadLibraryResult;
        }
    }

    {
        const int postLoadLibraryResult = PostLoadLibrary();
        if(!RT_SUCCESS(postLoadLibraryResult))
        {
            return postLoadLibraryResult;
        }
    }
    
    return GetExportAndRegisterDevices(pCallbacks, u32Version, vbDevice);
}

extern "C" DECLEXPORT(int) VBoxDriversRegister(PCPDMDRVREGCB pCallbacks, const uint32_t u32Version)
{
    LogRelFlow(("VBoxSoftGpuLoader::VBoxDriversRegister: u32Version=%#x pCallbacks->u32Version=%#x\n", u32Version, pCallbacks->u32Version));

    Console::Create();
    Console::Init();

    ConPrinter::PrintLn(u8"VBoxSoftGpuLoader::VBoxDriversRegister: u32Version=0x{X} pCallbacks->u32Version=0x{X}", u32Version, pCallbacks->u32Version);

    AssertLogRelMsgReturn(u32Version >= VBOX_VERSION, ("VirtualBox version %#x, expected %#x or higher\n", u32Version, VBOX_VERSION), VERR_VERSION_MISMATCH);
    AssertLogRelMsgReturn(pCallbacks->u32Version == PDM_DRVREG_CB_VERSION, ("Callbacks version %#x, expected %#x or higher\n", pCallbacks->u32Version, PDM_DRVREG_CB_VERSION), VERR_VERSION_MISMATCH);

    {
        const int preLoadLibraryResult = PreLoadLibrary();
        if(!RT_SUCCESS(preLoadLibraryResult))
        {
            return preLoadLibraryResult;
        }
    }

    HMODULE vbDriver;

    {
        const int preLoadLibraryResult = LoadLibraryFromEnv(EnvTauVBoxDriver, &vbDriver);
        if(!RT_SUCCESS(preLoadLibraryResult))
        {
            return preLoadLibraryResult;
        }
    }

    {
        const int postLoadLibraryResult = PostLoadLibrary();
        if(!RT_SUCCESS(postLoadLibraryResult))
        {
            return postLoadLibraryResult;
        }
    }

    return GetExportAndRegisterDrivers(pCallbacks, u32Version, vbDriver);
}
