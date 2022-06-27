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
private:
    MemoryManager m_MemoryManager;
    StreamingMultiprocessor m_SMs[4];
};
