#define LOG_GROUP LOG_GROUP_MISC

#define RT_STRICT
#include <VBox/com/assert.h>
#include <VBox/com/defs.h>
#include <VBox/com/Guid.h>
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/version.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <VBox/msi.h>

#include <VBox/com/string.h>
#include <VBox/com/VirtualBox.h>

#include <iprt/assert.h>
#include <Windows.h>
#include <cstring>

typedef HMODULE(*WINAPI LoadLibraryExW_f)(LPCWSTR, HANDLE, DWORD);
typedef FARPROC(*WINAPI GetProcAddress_f)(HMODULE, LPCSTR);
typedef NTSTATUS(*NTAPI NtProtectVirtualMemory_f)(HANDLE, PVOID*, PSIZE_T, ULONG, PULONG);

static NtProtectVirtualMemory_f NtProtectVirtualMemory = nullptr;

#define NtCurrentProcess() ((HANDLE)-(intptr_t)1)
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

static constexpr unsigned char ldrLoadDllOriginalBytes[16] = {
    0x48, 0x89, 0x5C, 0x24, 0x10,
    0x56,
    0x57,
    0x41, 0x56,
    0x48, 0x81, 0xEC, 0xD0, 0x00, 0x00, 0x00
};

static constexpr unsigned char ntCreateSectionOriginalBytes[16] = {
    0x4C, 0x8B, 0xD1,
    0xB8, 0x4A, 0x00, 0x00, 0x00,
    0xF6, 0x04, 0x25, 0x08, 0x03, 0xFE, 0x7F, 0x01
};

static constexpr const char* EnvTauVBoxDevice = "TAU_VBOX_DEVICE";

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

    LogRel(("%s Error [%x] %s\n", callerTag, errorCode, errorBuffer));

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

static int DeProtectSections(void* ldrLoadDll, void* ntCreateSection, DWORD* const oldProtectionLdrLoadDll, DWORD* const oldProtectionNtCreateSection)
{
    __try
    {
        // Change the protection of LdrLoadDll
        if(!SetProtection(ldrLoadDll, sizeof(ldrLoadDllOriginalBytes), PAGE_EXECUTE_READWRITE, oldProtectionLdrLoadDll))
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
        if(!SetProtection(ntCreateSection, sizeof(ntCreateSectionOriginalBytes), PAGE_EXECUTE_READWRITE, oldProtectionNtCreateSection))
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

static int ReProtectSections(void* ldrLoadDll, void* ntCreateSection, DWORD* const oldProtectionLdrLoadDll, DWORD* const oldProtectionNtCreateSection)
{
    __try
    {
        // Restore execution protection for LdrLoadDll
        if(!SetProtection(ldrLoadDll, sizeof(ldrLoadDllOriginalBytes), *oldProtectionLdrLoadDll, oldProtectionLdrLoadDll))
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
        if(!SetProtection(ntCreateSection, sizeof(ntCreateSectionOriginalBytes), *oldProtectionNtCreateSection, oldProtectionNtCreateSection))
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

[[nodiscard]] static int GetExportAndRegister(PPDMDEVREGCB pCallbacks, const uint32_t u32Version, HMODULE vbDevice) noexcept
{
    FNPDMVBOXDEVICESREGISTER* registerDevices = func_cast<FNPDMVBOXDEVICESREGISTER*>(GetProcAddress(vbDevice, "VBoxDevicesRegister"));

    if(!registerDevices)
    {
        AssertLogRelMsgFailedReturn(("Could not load VBoxDevicesRegister\n"), VERR_NOT_FOUND);
    }

    LogRel(("Loaded real VBoxDevicesRegister: %p\n", registerDevices));

    return registerDevices(pCallbacks, u32Version);
}

extern "C" DECLEXPORT(int) VBoxDevicesRegister(PPDMDEVREGCB pCallbacks, const uint32_t u32Version)
{
    LogRelFlow(("VBoxSoftGpuLoader::VBoxDevicesRegister: u32Version=%#x pCallbacks->u32Version=%#x\n", u32Version, pCallbacks->u32Version));
    
    AssertLogRelMsgReturn(u32Version >= VBOX_VERSION, ("VirtualBox version %#x, expected %#x or higher\n", u32Version, VBOX_VERSION), VERR_VERSION_MISMATCH);
    AssertLogRelMsgReturn(pCallbacks->u32Version == PDM_DEVREG_CB_VERSION, ("Callbacks version %#x, expected %#x or higher\n", pCallbacks->u32Version, PDM_DEVREG_CB_VERSION), VERR_VERSION_MISMATCH);

    // Get a handle to the NTDLL.dll library.
    const HMODULE ntdll = LoadLibraryA("ntdll.dll");

    if(!ntdll)
    {
        LogLoadLibraryError();
        AssertLogRelMsgFailedReturn(("Could not load ntdll.dll\n"), VERR_NOT_FOUND);
    }

    // Get a pointer to LdrLoadDll and NtCreateSection.
    void* const ldrLoadDll = reinterpret_cast<void*>(GetProcAddress(ntdll, "LdrLoadDll"));
    void* const ntCreateSection = reinterpret_cast<void*>(GetProcAddress(ntdll, "NtCreateSection"));
    NtProtectVirtualMemory = func_cast<NtProtectVirtualMemory_f>(GetProcAddress(ntdll, "NtProtectVirtualMemory"));

    if(!ldrLoadDll)
    {
        AssertLogRelMsgFailedReturn(("Could not load LdrLoadDll.\n"), VERR_NOT_FOUND);
    }

    if(!ntCreateSection)
    {
        AssertLogRelMsgFailedReturn(("Could not load NtCreateSection.\n"), VERR_NOT_FOUND);
    }

    if(!NtProtectVirtualMemory)
    {
        AssertLogRelMsgFailedReturn(("Could not load NtProtectVirtualMemory.\n"), VERR_NOT_FOUND);
    }

    LogRel(("Loaded LdrLoadDll %p and NtCreateSection %p.\n", ldrLoadDll, ntCreateSection));

    DWORD oldProtectionLdrLoadDll = 0;
    DWORD oldProtectionNtCreateSection = 0;

    {
        const int protectResult = DeProtectSections(ldrLoadDll, ntCreateSection, &oldProtectionLdrLoadDll, &oldProtectionNtCreateSection);
        if(!RT_SUCCESS(protectResult))
        {
            return protectResult;
        }
    }

    unsigned char ldrLoadDllPatchedBytes[sizeof(ldrLoadDllOriginalBytes)];
    unsigned char ntCreateSectionPatchedBytes[sizeof(ntCreateSectionOriginalBytes)];

    // Copy the patches.
    (void) ::std::memcpy(ldrLoadDllPatchedBytes, ldrLoadDll, sizeof(ldrLoadDllPatchedBytes));
    LogRel(("Saved patches LdrLoadDll.\n"));

    (void) ::std::memcpy(ntCreateSectionPatchedBytes, ntCreateSection, sizeof(ntCreateSectionPatchedBytes));
    LogRel(("Saved patches NtCreateSection.\n"));

    // Restore the original bytes.
    (void) ::std::memcpy(ldrLoadDll, ldrLoadDllOriginalBytes, sizeof(ldrLoadDllOriginalBytes));
    LogRel(("Restored original code for LdrLoadDll.\n"));

    (void) ::std::memcpy(ntCreateSection, ntCreateSectionOriginalBytes, sizeof(ntCreateSectionOriginalBytes));
    LogRel(("Restored original code for NtCreateSection.\n"));

    {
        const int protectResult = ReProtectSections(ldrLoadDll, ntCreateSection, &oldProtectionLdrLoadDll, &oldProtectionNtCreateSection);
        if(!RT_SUCCESS(protectResult))
        {
            return protectResult;
        }
    }

    (void) FlushInstructionCache(ntdll, ldrLoadDll, sizeof(ldrLoadDllOriginalBytes));
    LogRel(("Flushed instruction cache for LdrLoadDll.\n"));
    (void) FlushInstructionCache(ntdll, ntCreateSection, sizeof(ntCreateSectionOriginalBytes));
    LogRel(("Flushed instruction cache for NtCreateSection.\n"));

    size_t trueLibraryPathLength = 0;
    if(const errno_t err = getenv_s(&trueLibraryPathLength, nullptr, 0, EnvTauVBoxDevice))
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

    if(const errno_t err = getenv_s(&trueLibraryPathLength, trueLibraryPathBuffer, trueLibraryPathLength, EnvTauVBoxDevice))
    {
        delete[] trueLibraryPathBuffer;
        AssertLogRelMsgFailedReturn(("Failed to get the path to a device to load with a valid buffer. Error: %d\n", err), VERR_NOT_FOUND);
    }


    // Load our real device driver.
    const HMODULE vbDevice = LoadLibraryA(trueLibraryPathBuffer);


    if(!vbDevice)
    {
        LogLoadLibraryError();
        AssertLogRelMsgFailedReturn(("Could not load real %s\n", trueLibraryPathBuffer), VERR_NOT_FOUND);
    }

    LogRel(("Loaded real %s\n", trueLibraryPathBuffer));

    delete[] trueLibraryPathBuffer;

    {
        const int protectResult = DeProtectSections(ldrLoadDll, ntCreateSection, &oldProtectionLdrLoadDll, &oldProtectionNtCreateSection);
        if(!RT_SUCCESS(protectResult))
        {
            return protectResult;
        }
    }

    // Restore the hardening patches.
    (void) ::std::memcpy(ldrLoadDll, ldrLoadDllPatchedBytes, sizeof(ldrLoadDllPatchedBytes));
    LogRel(("Restored hardening patches for LdrLoadDll.\n"));

    (void) ::std::memcpy(ntCreateSection, ntCreateSectionPatchedBytes, sizeof(ntCreateSectionPatchedBytes));
    LogRel(("Restored hardening patches for NtCreateSection.\n"));

    {
        const int protectResult = ReProtectSections(ldrLoadDll, ntCreateSection, &oldProtectionLdrLoadDll, &oldProtectionNtCreateSection);
        if(!RT_SUCCESS(protectResult))
        {
            return protectResult;
        }
    }

    (void) FlushInstructionCache(ntdll, ldrLoadDll, sizeof(ldrLoadDllOriginalBytes));
    LogRel(("Flushed instruction cache for LdrLoadDll.\n"));
    (void) FlushInstructionCache(ntdll, ntCreateSection, sizeof(ntCreateSectionOriginalBytes));
    LogRel(("Flushed instruction cache for NtCreateSection.\n"));
    
    return GetExportAndRegister(pCallbacks, u32Version, vbDevice);
}
