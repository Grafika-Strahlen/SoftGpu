#pragma once

#include <Objects.hpp>
#include "StreamingMultiprocessor.hpp"
#include "Cache.hpp"

class Processor final
{
    DEFAULT_DESTRUCT(Processor);
    DELETE_CM(Processor);
public:
    Processor() noexcept
        : m_MemoryManager(this)
        , m_SMs { { this, 0 }, { this, 1 }, { this, 2 }, { this, 3 } }
    { }

    void Clock() noexcept
    {
        m_SMs[0].Clock();
        m_SMs[1].Clock();
        m_SMs[2].Clock();
        m_SMs[3].Clock();
    }

    void TestLoadProgram(const u32 sm, const u32 dispatchPort, const u8 replicationMask, void* const program)
    {
        m_SMs[sm].TestLoadProgram(dispatchPort, replicationMask, static_cast<u64>(reinterpret_cast<uintptr_t>(program)));
    }

    void TestLoadRegister(const u32 sm, const u32 dispatchPort, const u32 replicationIndex, const u8 registerIndex, const u32 registerValue)
    {
        m_SMs[sm].TestLoadRegister(dispatchPort, replicationIndex, registerIndex, registerValue);
    }

    [[nodiscard]] u32 Read(const u32 coreIndex, const u64 address) noexcept
    {
        return m_MemoryManager.Read(coreIndex, address);
    }

    void Write(const u32 coreIndex, const u64 address, const u32 value) noexcept
    {
        m_MemoryManager.Write(coreIndex, address, value);
    }

    void Prefetch(const u32 coreIndex, const u64 address) noexcept
    {
        m_MemoryManager.Prefetch(coreIndex, address);
    }

    void FlushCache(const u32 coreIndex) noexcept
    {
        m_MemoryManager.Flush(coreIndex);
    }
private:
    MemoryManager m_MemoryManager;
    StreamingMultiprocessor m_SMs[4];
};
