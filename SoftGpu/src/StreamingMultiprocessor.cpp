/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#include "StreamingMultiprocessor.hpp"
#include "Processor.hpp"

u32 StreamingMultiprocessor::Read(const u64 address) noexcept
{
    bool success;
    bool cacheDisable;
    bool external;
    const u64 physicalAddress = m_Mmu.TranslateAddress(address, &success, nullptr, nullptr, nullptr, &cacheDisable, &external);

    // Was the virtual address valid?
    if(!success)
    {
        return 0xFFFFFFFF;
    }

    return m_Processor->Read(m_SMIndex, physicalAddress, cacheDisable, external);
}

void StreamingMultiprocessor::Write(const u64 address, const u32 value) noexcept
{
    bool success;
    bool readWrite;
    bool execute;
    bool writeThrough;
    bool cacheDisable;
    bool external;
    const u64 physicalAddress = m_Mmu.TranslateAddress(address, &success, &readWrite, &execute, &writeThrough, &cacheDisable, &external);

    // Was the virtual address valid?
    if(!success)
    {
        return;
    }

    // Cannot write to read-only pages.
    if(!readWrite)
    {
        return;
    }

    // Cannot write to executable pages.
    if(execute)
    {
        return;
    }

    m_Mmu.MarkDirty(address);

    m_Processor->Write(m_SMIndex, physicalAddress, value, writeThrough, cacheDisable, external);
}

void StreamingMultiprocessor::Prefetch(u64 address) noexcept
{
    bool success;
    bool cacheDisable;
    bool external;
    const u64 physicalAddress = m_Mmu.TranslateAddress(address, &success, nullptr, nullptr, nullptr, &cacheDisable, &external);

    // Was the virtual address valid?
    if(!success)
    {
        return;
    }

    // Prefetching is meaningless when caching is disabled.
    if(cacheDisable)
    {
        return;
    }

    m_Processor->Prefetch(m_SMIndex, physicalAddress, external);
}

void StreamingMultiprocessor::FlushCache() noexcept
{
    m_Processor->FlushCache(m_SMIndex);
}

void StreamingMultiprocessor::WriteMmuPageInfo(const u64 physicalAddress, const u64 pageTableEntry) noexcept
{
    m_Processor->Write(m_SMIndex, physicalAddress, static_cast<u32>(pageTableEntry), true, true, false);
    m_Processor->Write(m_SMIndex, physicalAddress + 1, static_cast<u32>(pageTableEntry >> 32), true, true, false);
}
