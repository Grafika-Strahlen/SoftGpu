#if 0
#include "Cache.hpp"
#endif

#include <cstring>

template<uSys IndexBits, uSys SetLineCount>
u32 Cache<IndexBits, SetLineCount>::Read(const u64 address) noexcept
{
    const u64 lineOffset = address & 0x7;
    CacheLine<IndexBits>* cacheLine = GetCacheLine(address);
    
    if(!cacheLine || cacheLine->Mesi == MESI::Invalid)
    {
        if(!cacheLine)
        {
            cacheLine = GetFreeCacheLine(address);
        }

        if(m_MemoryManager->ReadCacheLine(m_LineIndex, address, cacheLine->Data))
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
void Cache<IndexBits, SetLineCount>::Write(const u64 address, const u32 value) noexcept
{
    const u64 lineOffset = address & 0x7;
    CacheLine<IndexBits>* cacheLine = GetCacheLine(address);

    if(!cacheLine || cacheLine->Mesi == MESI::Invalid)
    {
        if(!cacheLine)
        {
            cacheLine = GetFreeCacheLine(address);
        }

        (void) m_MemoryManager->ReadXCacheLine(m_LineIndex, address, cacheLine->Data);
        cacheLine->Mesi = MESI::Modified;
    }
    else if(cacheLine->Mesi == MESI::Exclusive || cacheLine->Mesi == MESI::Modified)
    {
        cacheLine->Mesi = MESI::Modified;
    }
    else if(cacheLine->Mesi == MESI::Shared)
    {
        cacheLine->Mesi = MESI::Modified;
        m_MemoryManager->UpgradeCacheLine(m_LineIndex, address);
    }

    cacheLine->Data[lineOffset] = value;
}

template<uSys IndexBits, uSys SetLineCount>
CacheLine<IndexBits>* Cache<IndexBits, SetLineCount>::GetFreeCacheLine(const u64 address) noexcept
{
    const u64 setIndex = (address >> 3) & ((1 << IndexBits) - 1);

    CacheSet<IndexBits, SetLineCount>& targetSet = m_Sets[setIndex];

    for(uSys i = 0; i < SetLineCount; ++i)
    {
        if(targetSet.SetLines[i].Mesi == MESI::Invalid)
        {
            return &targetSet.SetLines[i];
        }
    }

    for(uSys i = 0; i < SetLineCount; ++i)
    {
        if(targetSet.SetLines[i].Mesi == MESI::Exclusive || targetSet.SetLines[i].Mesi == MESI::Shared)
        {
            return &targetSet.SetLines[i];
        }
    }

    // This will be implemented as an n-bit rolling integer.
    const u64 rollingSelector = (m_RollingSelector++) % SetLineCount;

    CacheLine<IndexBits>* const cacheLine = &targetSet.SetLines[rollingSelector];

    m_MemoryManager->WriteBackCacheLine(m_LineIndex, address, cacheLine->Data);

    return cacheLine;
}

template<uSys IndexBits, uSys SetLineCount>
bool Cache<IndexBits, SetLineCount>::SnoopBusRead(const u32 requestorLine, const u64 address, u32* const dataBus) noexcept
{
    if(requestorLine == m_LineIndex)
    {
        return false;
    }

    CacheLine<IndexBits>* cacheLine = GetCacheLine(address);

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
        m_MemoryManager->WriteBackCacheLine(m_LineIndex, address, cacheLine->Data);
    }

    return true;
}

template<uSys IndexBits, uSys SetLineCount>
bool Cache<IndexBits, SetLineCount>::SnoopBusReadX(const u32 requestorLine, const u64 address, u32* const dataBus) noexcept
{
    if(requestorLine == m_LineIndex)
    {
        return false;
    }

    CacheLine<IndexBits>* cacheLine = GetCacheLine(address);

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
        m_MemoryManager->WriteBackCacheLine(m_LineIndex, address, cacheLine->Data);
    }

    return true;
}

template<uSys IndexBits, uSys SetLineCount>
void Cache<IndexBits, SetLineCount>::SnoopBusUpgrade(const u32 requestorLine, const u64 address) noexcept
{
    if(requestorLine == m_LineIndex)
    {
        return;
    }

    CacheLine<IndexBits>* cacheLine = GetCacheLine(address);

    if(cacheLine->Mesi == MESI::Shared)
    {
        cacheLine->Mesi = MESI::Invalid;
    }
}
