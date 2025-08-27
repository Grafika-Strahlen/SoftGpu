/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#include "Cache.hpp"
#include "Processor.hpp"

void CacheController::ReadCacheLine(const u64 address, u32 data[8], const bool external) noexcept
{
    for(uSys i = 0; i < 8; ++i)
    {
        data[i] = m_Processor->MemReadPhy(address + i, external);
    }
}

void CacheController::WriteCacheLine(const u64 address, const u32 data[8], const bool external) noexcept
{
    for(uSys i = 0; i < 8; ++i)
    {
        m_Processor->MemWritePhy(address + i, data[i], external);
    }
}
