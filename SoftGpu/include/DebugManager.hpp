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
        if(m_Pipe != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_Pipe);
        }
    }

    [[nodiscard]] bool IsAttached() const noexcept { return m_IsAttached; }
    
    [[nodiscard]] bool Stepping() const noexcept { return m_Stepping; }
    [[nodiscard]] bool& Stepping() noexcept { return m_Stepping; }

    template<typename T>
    void WriteRaw(const T& data) noexcept
    {
        if(m_Pipe == INVALID_HANDLE_VALUE)
        {
            return;
        }

        (void) WriteFile(m_Pipe, &data, sizeof(data), nullptr, nullptr);
    }

    void WriteRaw(const void* data, const u32 dataSize) noexcept
    {
        if(m_Pipe == INVALID_HANDLE_VALUE)
        {
            return;
        }
        
        (void) WriteFile(m_Pipe, data, dataSize, nullptr, nullptr);
    }

    template<typename T>
    void Write(const u32 dataCode, const T& data) noexcept
    {
        if(m_Pipe == INVALID_HANDLE_VALUE)
        {
            return;
        }

        constexpr u32 dataSize = sizeof(data);

        (void) WriteFile(m_Pipe, &dataCode, sizeof(dataCode), nullptr, nullptr);
        (void) WriteFile(m_Pipe, &dataSize, sizeof(dataSize), nullptr, nullptr);
        (void) WriteFile(m_Pipe, &data, dataSize, nullptr, nullptr);
    }

    void Write(const u32 dataCode, const void* data, const u32 dataSize) noexcept
    {
        if(m_Pipe == INVALID_HANDLE_VALUE)
        {
            return;
        }

        (void) WriteFile(m_Pipe, &dataCode, sizeof(dataCode), nullptr, nullptr);
        (void) WriteFile(m_Pipe, &dataSize, sizeof(dataSize), nullptr, nullptr);
        (void) WriteFile(m_Pipe, data, dataSize, nullptr, nullptr);
    }

    void Write(const u32 dataCode) noexcept
    {
        if(m_Pipe == INVALID_HANDLE_VALUE)
        {
            return;
        }

        constexpr u32 dataSize = 0;

        (void) WriteFile(m_Pipe, &dataCode, sizeof(dataCode), nullptr, nullptr);
        (void) WriteFile(m_Pipe, &dataSize, sizeof(dataSize), nullptr, nullptr);
    }

    template<typename T>
    [[nodiscard]] T Read() noexcept
    {
        if(m_Pipe == INVALID_HANDLE_VALUE)
        {
            return {};
        }

        T ret;
        DWORD numBytesRead;
        (void) ReadFile(m_Pipe, &ret, sizeof(ret), &numBytesRead, nullptr);

        return ret;
    }

    i32 Read(void* const buffer, const u32 dataSize) noexcept
    {
        if(m_Pipe == INVALID_HANDLE_VALUE)
        {
            return {};
        }
        
        DWORD numBytesRead;
        if(!ReadFile(m_Pipe, buffer, dataSize, &numBytesRead, nullptr))
        {
            return -1;
        }

        return static_cast<i32>(numBytesRead);
    }
public:
    template<typename CharT>
    static HRESULT Create(DebugManager* const manager, const DynStringT<CharT>& pipeServer) noexcept
    {
        // Check that the manager is valid.
        if(!manager)
        {
            return E_INVALIDARG;
        }

        // Convert the pipe server to wide char.
        const WDynString wPipeServer = StringCast<CharT>(pipeServer);

        // Open the file handle.
        const HANDLE pipeHandle = CreateFileW(wPipeServer.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

        // If we failed to open the file, report.
        if(pipeHandle == INVALID_HANDLE_VALUE)
        {
            return E_FAIL;
        }

        // Destroy the previous manager. This should do pretty much nothing.
        manager->~DebugManager();

        // Initialize the new debug manager.
        new(reinterpret_cast<void*>(manager)) DebugManager(pipeHandle);

        {
            const u32 dataCode = manager->Read<u32>();
            const u32 dataSize = manager->Read<u32>();

            if(dataCode == DebugCodeStartPaused)
            {
                const bool startPaused = manager->Read<bool>();
                manager->Stepping() = startPaused;
            }
            else
            {
                void* buffer = operator new(dataSize);
                (void) manager->Read(buffer, dataSize);
                operator delete(buffer);
            }
        }

        return S_OK;
    }

    template<typename CharT, uSys Len>
    static HRESULT Create(DebugManager* manager, const CharT(&str)[Len]) noexcept
    {
        return Create(manager, DynStringT<CharT>::FromStatic(str));
    }
private:
    DebugManager(const HANDLE pipe) noexcept
        : m_Pipe(pipe)
        , m_IsAttached(true)
        , m_Stepping(false)
    { }
private:
    HANDLE m_Pipe;
    bool m_IsAttached = false;
    bool m_Stepping;
};

extern DebugManager GlobalDebug;

