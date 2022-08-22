#pragma once

#include <ConPrinter.hpp>
#include <Objects.hpp>
#include "StreamingMultiprocessor.hpp"
#include "PCIControlRegisters.hpp"
#include "Cache.hpp"
#include "DebugManager.hpp"

class Processor final
{
    DEFAULT_DESTRUCT(Processor);
    DELETE_CM(Processor);
public:
    Processor() noexcept
        : m_PCIRegisters(this)
        , m_MemoryManager(this)
        , m_SMs { { this, 0 }, { this, 1 }, { this, 2 }, { this, 3 } }
        , m_ClockCycle(0)
    { }

    void Reset()
    {
        m_PCIRegisters.Reset();
        m_MemoryManager.Reset();
        m_SMs[0].Reset();
        m_SMs[1].Reset();
        m_SMs[2].Reset();
        m_SMs[3].Reset();
        m_ClockCycle = 0;
    }

    void Clock() noexcept
    {
        ++m_ClockCycle;
        if(GlobalDebug.IsAttached())
        {
            GlobalDebug.Write(DebugCodeReportTiming, &m_ClockCycle, sizeof(m_ClockCycle));

            if(!GlobalDebug.Stepping())
            {
                GlobalDebug.Write(DebugCodeCheckForPause);

                const u32 dataCode = GlobalDebug.Read<u32>();
                const u32 dataSize = GlobalDebug.Read<u32>();

                if(dataCode == DebugCodePause)
                {
                    GlobalDebug.Stepping() = true;
                }
            }

            if(GlobalDebug.Stepping())
            {
                ConPrinter::Print("Waiting for Step.\n");
                GlobalDebug.Write(DebugCodeReportStepReady);

                while(true)
                {
                    const u32 dataCode = GlobalDebug.Read<u32>();
                    const u32 dataSize = GlobalDebug.Read<u32>();

                    if(dataCode == DebugCodeStep)
                    {
                        ConPrinter::Print("Step Received.\n");
                        break;
                    }
                    else if(dataCode == DebugCodeResume)
                    {
                        ConPrinter::Print("Resume Received.\n");
                        GlobalDebug.Stepping() = false;
                        break;
                    }
                }
            }
        }

        m_SMs[0].Clock();
        m_SMs[1].Clock();
        m_SMs[2].Clock();
        m_SMs[3].Clock();
    }

    void TestLoadProgram(const u32 sm, const u32 dispatchPort, const u8 replicationMask, const u64 program)
    {
        m_SMs[sm].TestLoadProgram(dispatchPort, replicationMask, program);
    }

    void TestLoadProgram(const u32 sm, const u32 dispatchPort, const u8 replicationMask, void* const program)
    {
        TestLoadProgram(sm, dispatchPort, replicationMask, reinterpret_cast<u64>(program));
    }

    void TestLoadRegister(const u32 sm, const u32 dispatchPort, const u32 replicationIndex, const u8 registerIndex, const u32 registerValue)
    {
        m_SMs[sm].TestLoadRegister(dispatchPort, replicationIndex, registerIndex, registerValue);
    }

    [[nodiscard]] u32 MemReadPhy(const u64 address, const bool external = false) noexcept
    {
        u32 ret;
        // The memory granularity is 32 bits, thus we'll adjust to an 8 bit granularity for x86.
        const uintptr_t addressX86 = address << 2;
        (void) ::std::memcpy(&ret, reinterpret_cast<void*>(addressX86), sizeof(u32));
        return ret;
    }
    
    void MemWritePhy(const u64 address, const u32 value, const bool external = false) noexcept
    {
        // The memory granularity is 32 bits, thus we'll adjust to an 8 bit granularity for x86.
        const uintptr_t addressX86 = address << 2;
        (void) ::std::memcpy(reinterpret_cast<void*>(addressX86), &value, sizeof(u32));
    }
    
    [[nodiscard]] u32 Read(const u32 coreIndex, const u64 address, const bool cacheDisable = false, const bool external = false) noexcept
    {
        if(cacheDisable)
        {
            return MemReadPhy(address, external);
        }

        return m_MemoryManager.Read(coreIndex, address, external);
    }

    void Write(const u32 coreIndex, const u64 address, const u32 value, const bool writeThrough = false, const bool cacheDisable = false, const bool external = false) noexcept
    {
        if(cacheDisable)
        {
            MemWritePhy(address, value, external);
            return;
        }

        m_MemoryManager.Write(coreIndex, address, value, external, writeThrough);
    }

    void Prefetch(const u32 coreIndex, const u64 address, const bool external = false) noexcept
    {
        m_MemoryManager.Prefetch(coreIndex, address, external);
    }

    void FlushCache(const u32 coreIndex) noexcept
    {
        m_MemoryManager.Flush(coreIndex);
    }

    void LoadPageDirectoryPointer(const u64 coreIndex, const u64 pageDirectoryPhysicalAddress) noexcept
    {
        m_SMs[coreIndex].LoadPageDirectoryPointer(pageDirectoryPhysicalAddress);
    }

    void LoadPageDirectoryPointer(const u64 coreIndex, const void* const pageDirectoryPhysicalAddress) noexcept
    {
        LoadPageDirectoryPointer(coreIndex, reinterpret_cast<u64>(pageDirectoryPhysicalAddress) >> 16);
    }

    void FlushMmuCache(const u64 coreIndex) noexcept
    {
        m_SMs[coreIndex].FlushCache();
    }
private:
    PCIControlRegisters m_PCIRegisters;
    MemoryManager m_MemoryManager;
    StreamingMultiprocessor m_SMs[4];
    u32 m_ClockCycle;
};
