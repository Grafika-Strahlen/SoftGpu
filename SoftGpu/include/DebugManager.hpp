#pragma once

#include <Objects.hpp>
#include <NumTypes.hpp>
#include <String.hpp>
#include <Windows.h>

inline constexpr u32 DebugCodeNop = 0;
inline constexpr u32 DebugCodeStartPaused = 1;
inline constexpr u32 DebugCodeReportTiming = 2;
inline constexpr u32 DebugCodeReportStepReady = 3;
inline constexpr u32 DebugCodePause = 4;
inline constexpr u32 DebugCodeResume = 5;
inline constexpr u32 DebugCodeStep = 6;
inline constexpr u32 DebugCodeCheckForPause = 7;
inline constexpr u32 DebugCodeReportRegisterFile = 8;
inline constexpr u32 DebugCodeReportBaseRegister = 9;
inline constexpr u32 DebugCodeReportRegisterContestion = 10;

class DebugManager final
{
    DEFAULT_CONSTRUCT_PU(DebugManager);
    DELETE_CM(DebugManager);
public:
    ~DebugManager() noexcept
    {
        if(m_SteppingPipe != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_SteppingPipe);
            m_SteppingPipe = INVALID_HANDLE_VALUE;
        }

        if(m_InfoPipe != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_InfoPipe);
            m_InfoPipe = INVALID_HANDLE_VALUE;
        }
    }

    [[nodiscard]] bool IsAttached() const noexcept { return m_IsAttached; }
    
    [[nodiscard]] bool Stepping() const noexcept { return m_Stepping && !m_DisableStepping; }
    [[nodiscard]] bool& Stepping() noexcept { return m_Stepping; }

    [[nodiscard]] bool DisableStepping() const noexcept { return m_DisableStepping; }

    template<typename T>
    void WriteRawStepping(const T& data) noexcept
    {
        if(m_SteppingPipe == INVALID_HANDLE_VALUE)
        {
            return;
        }

        (void) WriteFile(m_SteppingPipe, &data, sizeof(data), nullptr, nullptr);
    }

    void WriteRawStepping(const void* data, const u32 dataSize) noexcept
    {
        if(m_SteppingPipe == INVALID_HANDLE_VALUE)
        {
            return;
        }

        (void) WriteFile(m_SteppingPipe, data, dataSize, nullptr, nullptr);
    }

    template<typename T>
    void WriteStepping(const u32 dataCode, const T& data) noexcept
    {
        if(m_SteppingPipe == INVALID_HANDLE_VALUE)
        {
            return;
        }

        constexpr u32 dataSize = sizeof(data);

        (void) WriteFile(m_SteppingPipe, &dataCode, sizeof(dataCode), nullptr, nullptr);
        (void) WriteFile(m_SteppingPipe, &dataSize, sizeof(dataSize), nullptr, nullptr);
        (void) WriteFile(m_SteppingPipe, &data, dataSize, nullptr, nullptr);
    }

    void WriteStepping(const u32 dataCode, const void* data, const u32 dataSize) noexcept
    {
        if(m_SteppingPipe == INVALID_HANDLE_VALUE)
        {
            return;
        }

        (void) WriteFile(m_SteppingPipe, &dataCode, sizeof(dataCode), nullptr, nullptr);
        (void) WriteFile(m_SteppingPipe, &dataSize, sizeof(dataSize), nullptr, nullptr);
        (void) WriteFile(m_SteppingPipe, data, dataSize, nullptr, nullptr);
    }

    void WriteStepping(const u32 dataCode) noexcept
    {
        if(m_SteppingPipe == INVALID_HANDLE_VALUE)
        {
            return;
        }

        constexpr u32 dataSize = 0;

        (void) WriteFile(m_SteppingPipe, &dataCode, sizeof(dataCode), nullptr, nullptr);
        (void) WriteFile(m_SteppingPipe, &dataSize, sizeof(dataSize), nullptr, nullptr);
    }

    template<typename T>
    [[nodiscard]] T ReadStepping() noexcept
    {
        if(m_SteppingPipe == INVALID_HANDLE_VALUE)
        {
            return {};
        }

        T ret;
        DWORD numBytesRead;
        (void) ReadFile(m_SteppingPipe, &ret, sizeof(ret), &numBytesRead, nullptr);

        return ret;
    }

    i32 ReadStepping(void* const buffer, const u32 dataSize) noexcept
    {
        if(m_SteppingPipe == INVALID_HANDLE_VALUE)
        {
            return {};
        }

        DWORD numBytesRead;
        if(!ReadFile(m_SteppingPipe, buffer, dataSize, &numBytesRead, nullptr))
        {
            return -1;
        }

        return static_cast<i32>(numBytesRead);
    }

    template<typename T>
    void WriteRawInfo(const T& data) noexcept
    {
        if(m_InfoPipe == INVALID_HANDLE_VALUE)
        {
            return;
        }

        (void) WriteFile(m_InfoPipe, &data, sizeof(data), nullptr, nullptr);
    }

    void WriteRawInfo(const void* data, const u32 dataSize) noexcept
    {
        if(m_InfoPipe == INVALID_HANDLE_VALUE)
        {
            return;
        }
        
        (void) WriteFile(m_InfoPipe, data, dataSize, nullptr, nullptr);
    }

    template<typename T>
    void WriteInfo(const u32 dataCode, const T& data) noexcept
    {
        if(m_InfoPipe == INVALID_HANDLE_VALUE)
        {
            return;
        }

        constexpr u32 dataSize = sizeof(data);

        (void) WriteFile(m_InfoPipe, &dataCode, sizeof(dataCode), nullptr, nullptr);
        (void) WriteFile(m_InfoPipe, &dataSize, sizeof(dataSize), nullptr, nullptr);
        (void) WriteFile(m_InfoPipe, &data, dataSize, nullptr, nullptr);
    }

    void WriteInfo(const u32 dataCode, const void* data, const u32 dataSize) noexcept
    {
        if(m_InfoPipe == INVALID_HANDLE_VALUE)
        {
            return;
        }

        (void) WriteFile(m_InfoPipe, &dataCode, sizeof(dataCode), nullptr, nullptr);
        (void) WriteFile(m_InfoPipe, &dataSize, sizeof(dataSize), nullptr, nullptr);
        (void) WriteFile(m_InfoPipe, data, dataSize, nullptr, nullptr);
    }

    void WriteInfo(const u32 dataCode) noexcept
    {
        if(m_InfoPipe == INVALID_HANDLE_VALUE)
        {
            return;
        }

        constexpr u32 dataSize = 0;

        (void) WriteFile(m_InfoPipe, &dataCode, sizeof(dataCode), nullptr, nullptr);
        (void) WriteFile(m_InfoPipe, &dataSize, sizeof(dataSize), nullptr, nullptr);
    }

    template<typename T>
    [[nodiscard]] T ReadInfo() noexcept
    {
        if(m_InfoPipe == INVALID_HANDLE_VALUE)
        {
            return {};
        }

        T ret;
        DWORD numBytesRead;
        (void) ReadFile(m_InfoPipe, &ret, sizeof(ret), &numBytesRead, nullptr);

        return ret;
    }

    i32 ReadInfo(void* const buffer, const u32 dataSize) noexcept
    {
        if(m_InfoPipe == INVALID_HANDLE_VALUE)
        {
            return {};
        }
        
        DWORD numBytesRead;
        if(!ReadFile(m_InfoPipe, buffer, dataSize, &numBytesRead, nullptr))
        {
            return -1;
        }

        return static_cast<i32>(numBytesRead);
    }
public:
    template<typename CharT>
    static HRESULT Create(DebugManager* const manager, const DynStringT<CharT>& steppingPipeServer, const DynStringT<CharT>& infoPipeServer, const bool disableStepping) noexcept
    {
        // Check that the manager is valid.
        if(!manager)
        {
            return E_INVALIDARG;
        }

        // Convert the pipe server to wide char.
        const WDynString wSteppingPipeServer = StringCast<CharT>(steppingPipeServer);
        const WDynString wInfoPipeServer = StringCast<CharT>(infoPipeServer);

        // Open the file handle.
        const HANDLE steppingPipeHandle = CreateFileW(wSteppingPipeServer.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        const HANDLE infoPipeHandle = CreateFileW(wInfoPipeServer.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);

        // If we failed to open the file, report.
        if(steppingPipeHandle == INVALID_HANDLE_VALUE || infoPipeHandle == INVALID_HANDLE_VALUE)
        {
            return E_FAIL;
        }

        // Destroy the previous manager. This should do pretty much nothing.
        manager->~DebugManager();

        // Initialize the new debug manager.
        new(reinterpret_cast<void*>(manager)) DebugManager(steppingPipeHandle, infoPipeHandle, disableStepping);

        {
            const u32 dataCode = manager->ReadStepping<u32>();
            const u32 dataSize = manager->ReadStepping<u32>();

            if(dataCode == DebugCodeStartPaused)
            {
                const bool startPaused = manager->ReadStepping<bool>();
                manager->Stepping() = startPaused;
            }
            else
            {
                void* buffer = operator new(dataSize);
                (void) manager->ReadStepping(buffer, dataSize);
                operator delete(buffer);
            }
        }

        return S_OK;
    }

    template<typename CharT, uSys SteppingLen, uSys InfoLen>
    static HRESULT Create(DebugManager* manager, const CharT(&steppingPath)[SteppingLen], const CharT(&infoPath)[InfoLen], const bool disableStepping) noexcept
    {
        return Create(manager, DynStringT<CharT>::FromStatic(steppingPath), DynStringT<CharT>::FromStatic(infoPath), disableStepping);
    }
private:
    DebugManager(const HANDLE steppingPipe, const HANDLE infoPipe, const bool disableStepping) noexcept
        : m_SteppingPipe(steppingPipe)
        , m_InfoPipe(infoPipe)
        , m_IsAttached(true)
        , m_Stepping(false)
        , m_DisableStepping(disableStepping)
    { }
private:
    HANDLE m_SteppingPipe;
    HANDLE m_InfoPipe;
    bool m_IsAttached = false;
    bool m_Stepping;
    bool m_DisableStepping;
};

extern DebugManager GlobalDebug;

