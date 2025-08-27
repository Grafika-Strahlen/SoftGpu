/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#if 0
#include "Cache.hpp"
#endif

#include <cstring>

template<uSys IndexBits, uSys SetLineCount>
u32 Cache<IndexBits, SetLineCount>::Read(u64 address, const bool external) noexcept
{
    const u64 lineOffset = address & 0x7;
    address >>= 3;
    address <<= 3;
    CacheLine<IndexBits>* cacheLine = GetCacheLine(address, external);
    
    if(!cacheLine || cacheLine->Mesi == MESI::Invalid)
    {
        if(!cacheLine)
        {
            cacheLine = GetFreeCacheLine(address, external);
        }

        if(m_MemoryManager->ReadCacheLine(m_LineIndex, address, external, cacheLine->Data))
        {
            cacheLine->Mesi = MESI::Shared;
        }
        else
        {
            cacheLine->Mesi = MESI::Exclusive;
        }
    }

    return cacheLine->Data[lineOffset];
}

template<uSys IndexBits, uSys SetLineCount>
void Cache<IndexBits, SetLineCount>::Write(u64 address, const u32 value, const bool external, const bool writeThrough) noexcept
{
    const u64 lineOffset = address & 0x7;
    address >>= 3;
    address <<= 3;
    CacheLine<IndexBits>* cacheLine = GetCacheLine(address, external);

    if(!cacheLine || cacheLine->Mesi == MESI::Invalid)
    {
        if(!cacheLine)
        {
            cacheLine = GetFreeCacheLine(address, external);
        }

        (void) m_MemoryManager->ReadXCacheLine(m_LineIndex, address, external, cacheLine->Data);
        cacheLine->Mesi = MESI::Modified;
    }
    else if(cacheLine->Mesi == MESI::Exclusive || cacheLine->Mesi == MESI::Modified)
    {
        cacheLine->Mesi = MESI::Modified;
    }
    else if(cacheLine->Mesi == MESI::Shared)
    {
        cacheLine->Mesi = MESI::Modified;
        m_MemoryManager->UpgradeCacheLine(m_LineIndex, address, external);
    }

    cacheLine->Data[lineOffset] = value;
    if(writeThrough)
    {
        m_MemoryManager->WriteBackCacheLine(m_LineIndex, address, external, cacheLine->Data);
    }
}

template<uSys IndexBits, uSys SetLineCount>
void Cache<IndexBits, SetLineCount>::Flush() noexcept
{
    for(uSys i = 0; i < 1 << IndexBits; ++i)
    {
        CacheSet<IndexBits, SetLineCount>& cacheSet = m_Sets[i];

        for(u32 j = 0; j < SetLineCount; ++j)
        {
            CacheLine<IndexBits>& cacheLine = cacheSet.SetLines[j];

            if(cacheLine.Mesi == MESI::Modified)
            {
                const u64 address = (cacheLine.Tag << (IndexBits + 3)) | (i << 3);

                m_MemoryManager->WriteBackCacheLine(m_LineIndex, address, cacheLine.External, cacheLine.Data);
                cacheLine.Mesi = MESI::Exclusive;
            }
        }
    }
}

template<uSys IndexBits, uSys SetLineCount>
CacheLine<IndexBits>* Cache<IndexBits, SetLineCount>::GetFreeCacheLine(const u64 address, const bool external) noexcept
{
    const u64 setIndex = (address >> 3) & ((1 << IndexBits) - 1);
    const u64 tag = address >> (IndexBits + 3);

    CacheSet<IndexBits, SetLineCount>& targetSet = m_Sets[setIndex];

    for(uSys i = 0; i < SetLineCount; ++i)
    {
        if(targetSet.SetLines[i].Mesi == MESI::Invalid)
        {
            targetSet.SetLines[i].Tag = tag;
            targetSet.SetLines[i].External = external;
            return &targetSet.SetLines[i];
        }
    }

    for(uSys i = 0; i < SetLineCount; ++i)
    {
        if(targetSet.SetLines[i].Mesi == MESI::Exclusive || targetSet.SetLines[i].Mesi == MESI::Shared)
        {
            targetSet.SetLines[i].Tag = tag;
            targetSet.SetLines[i].External = external;
            return &targetSet.SetLines[i];
        }
    }

    // This will be implemented as an n-bit rolling integer.
    const u64 rollingSelector = (m_RollingSelector++) % SetLineCount;

    CacheLine<IndexBits>* const cacheLine = &targetSet.SetLines[rollingSelector];

    m_MemoryManager->WriteBackCacheLine(m_LineIndex, address, external, cacheLine->Data);
    cacheLine->Tag = tag;

    return cacheLine;
}

template<uSys IndexBits, uSys SetLineCount>
bool Cache<IndexBits, SetLineCount>::SnoopBusRead(const u32 requestorLine, const u64 address, const bool external, u32* const dataBus) noexcept
{
    if(requestorLine == m_LineIndex)
    {
        return false;
    }

    CacheLine<IndexBits>* cacheLine = GetCacheLine(address, external);

    if(!cacheLine || cacheLine->Mesi == MESI::Invalid)
    {
        return false;
    }

    if(cacheLine->Mesi == MESI::Exclusive)
    {
        cacheLine->Mesi = MESI::Shared;
        if(dataBus)
        {
            (void) ::std::memcpy(dataBus, cacheLine->Data, sizeof(cacheLine->Data));
        }
    }
    else if(cacheLine->Mesi == MESI::Shared)
    {
        if(dataBus)
        {
            (void) ::std::memcpy(dataBus, cacheLine->Data, sizeof(cacheLine->Data));
        }
    }
    else if(cacheLine->Mesi == MESI::Modified)
    {
        cacheLine->Mesi = MESI::Shared;
        if(dataBus)
        {
            (void) ::std::memcpy(dataBus, cacheLine->Data, sizeof(cacheLine->Data));
        }
        m_MemoryManager->WriteBackCacheLine(m_LineIndex, address, external, cacheLine->Data);
    }

    return true;
}

template<uSys IndexBits, uSys SetLineCount>
bool Cache<IndexBits, SetLineCount>::SnoopBusReadX(const u32 requestorLine, const u64 address, const bool external, u32* const dataBus) noexcept
{
    if(requestorLine == m_LineIndex)
    {
        return false;
    }

    CacheLine<IndexBits>* cacheLine = GetCacheLine(address, external);

    if(!cacheLine || cacheLine->Mesi == MESI::Invalid)
    {
        return false;
    }

    if(cacheLine->Mesi == MESI::Exclusive)
    {
        cacheLine->Mesi = MESI::Invalid;
        if(dataBus)
        {
            (void) ::std::memcpy(dataBus, cacheLine->Data, sizeof(cacheLine->Data));
        }
    }
    else if(cacheLine->Mesi == MESI::Shared)
    {
        cacheLine->Mesi = MESI::Invalid;
        if(dataBus)
        {
            (void) ::std::memcpy(dataBus, cacheLine->Data, sizeof(cacheLine->Data));
        }
    }
    else if(cacheLine->Mesi == MESI::Modified)
    {
        cacheLine->Mesi = MESI::Invalid;
        if(dataBus)
        {
            (void) ::std::memcpy(dataBus, cacheLine->Data, sizeof(cacheLine->Data));
        }
        m_MemoryManager->WriteBackCacheLine(m_LineIndex, address, external, cacheLine->Data);
    }

    return true;
}

template<uSys IndexBits, uSys SetLineCount>
void Cache<IndexBits, SetLineCount>::SnoopBusUpgrade(const u32 requestorLine, const u64 address, const bool external) noexcept
{
    if(requestorLine == m_LineIndex)
    {
        return;
    }

    CacheLine<IndexBits>* cacheLine = GetCacheLine(address, external);

    if(cacheLine->Mesi == MESI::Shared)
    {
        cacheLine->Mesi = MESI::Invalid;
    }
}
