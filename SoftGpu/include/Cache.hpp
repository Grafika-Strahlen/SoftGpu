/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <cstring>
#include <array>

#include <Objects.hpp>
#include <NumTypes.hpp>

enum class MESI : u8
{
    Modified = 0,
    Exclusive = 1,
    Shared = 2,
    Invalid = 3
};

template<uSys IndexBits>
struct CacheLine final
{
    DEFAULT_DESTRUCT(CacheLine);
    DELETE_CM(CacheLine);
public:
    MESI Mesi : 2;
    u64 Tag : 61 - IndexBits; // 3 bits of offset, IndexBits bits of set index
    u64 External : 1; // Was this from external memory?
    u64 Pad : 8; // Because we're at a high level we'll pad the structure to be nice.
    u32 Data[8]; // 32 bytes / 8 words per cache line.

    CacheLine() noexcept
        : Mesi(MESI::Invalid)
        , Tag{ }
        , External{ }
        , Pad{ }
        , Data{ }
    { }

    void Reset()
    {
        Mesi = MESI::Invalid;
        Tag = { };
        External = { };
        Pad = { };
    }
};

template<uSys IndexBits, uSys SetLineCount>
struct CacheSet final
{
    DEFAULT_CONSTRUCT_PU(CacheSet);
    DEFAULT_DESTRUCT(CacheSet);
    DELETE_CM(CacheSet);
public:
    CacheLine<IndexBits> SetLines[SetLineCount];

    void Reset()
    {
        for(uSys i = 0; i < SetLineCount; ++i)
        {
            SetLines[i].Reset();
        }
    }
};

class CacheController;

template<uSys IndexBits, uSys SetLineCount>
class Cache final
{
    DEFAULT_DESTRUCT(Cache);
    DELETE_CM(Cache);
public:
    Cache(CacheController* const memoryManager, const u32 lineIndex) noexcept
        : m_MemoryManager(memoryManager)
        , m_LineIndex(lineIndex)
        , m_Sets{ }
        , m_RollingSelector(0)
    { }

    void Reset()
    {
        m_RollingSelector = 0;

        for(uSys i = 0; i < ::std::size(m_Sets); ++i)
        {
            m_Sets[i].Reset();
        }
    }

    [[nodiscard]] u32 Read(u64 address, bool external) noexcept;
    void Write(u64 address, u32 value, bool external, bool writeThrough) noexcept;
    // void FillCacheLine(u64 address, const u32* data) noexcept;
    void Flush() noexcept;

    bool SnoopBusRead(u32 requestorLine, u64 address, bool external, u32* dataBus) noexcept;
    bool SnoopBusReadX(u32 requestorLine, u64 address, bool external, u32* dataBus) noexcept;
    void SnoopBusUpgrade(u32 requestorLine, u64 address, bool external) noexcept;
private:
    [[nodiscard]] CacheLine<IndexBits>* GetCacheLine(const u64 address, const bool external) noexcept
    {
        const u64 setIndex = (address >> 3) & ((1 << IndexBits) - 1);
        const u64 tag = address >> (IndexBits + 3);

        CacheSet<IndexBits, SetLineCount>& targetSet = m_Sets[setIndex];

        for(uSys i = 0; i < SetLineCount; ++i)
        {
            if(targetSet.SetLines[i].Tag == tag && targetSet.SetLines[i].External == external)
            {
                return &targetSet.SetLines[i];
            }
        }

        return nullptr;
    }

    [[nodiscard]] CacheLine<IndexBits>* GetFreeCacheLine(u64 address, bool external) noexcept;
private:
    CacheController* m_MemoryManager;
    u32 m_LineIndex;
    CacheSet<IndexBits, SetLineCount> m_Sets[1 << IndexBits];
    u32 m_RollingSelector;
};

class Processor;

class CacheController final
{
public:
    CacheController(Processor* const processor) noexcept
        : m_Processor(processor)
        , m_L0Caches{ { this, 0 }, { this, 1 }, { this, 2 }, { this, 3 } }
        // , L1Cache(this, 4)
    { }

    void Reset()
    {
        m_L0Caches[0].Reset();
        m_L0Caches[1].Reset();
        m_L0Caches[2].Reset();
        m_L0Caches[3].Reset();
    }

    [[nodiscard]] u32 Read(const u32 coreIndex, const u64 address, const bool external) noexcept
    {
        return m_L0Caches[coreIndex].Read(address, external);
    }

    void Write(const u32 coreIndex, const u64 address, const u32 value, const bool external, const bool writeThrough) noexcept
    {
        m_L0Caches[coreIndex].Write(address, value, external, writeThrough);
    }

    void Prefetch(const u32 coreIndex, const u64 address, const bool external) noexcept
    {
        // For pre-fetching we'll be acting asynchronously typically, but we're forced to act synchronously in software, so we'll just redirect to read.
        (void) m_L0Caches[coreIndex].Read(address, external);
    }

    void Flush(const u32 coreIndex) noexcept
    {
        m_L0Caches[coreIndex].Flush();
    }

    bool ReadCacheLine(const u32 requestorLine, const u64 address, const bool external, u32* const cacheLine) noexcept
    {
        bool didWrite = m_L0Caches[0].SnoopBusRead(requestorLine, address, external, cacheLine);
        didWrite = m_L0Caches[1].SnoopBusRead(requestorLine, address, external, didWrite ? nullptr : cacheLine);
        didWrite = m_L0Caches[2].SnoopBusRead(requestorLine, address, external, didWrite ? nullptr : cacheLine);
        didWrite = m_L0Caches[3].SnoopBusRead(requestorLine, address, external, didWrite ? nullptr : cacheLine);

        if(!didWrite)
        {
            // The memory granularity is 32 bits, thus we'll adjust to an 8 bit granularity for x86.
            ReadCacheLine(address, cacheLine, external);
            return false;
        }

        return true;
    }

    bool ReadXCacheLine(const u32 requestorLine, const u64 address, const bool external, u32* const cacheLine) noexcept
    {
        bool didWrite = m_L0Caches[0].SnoopBusReadX(requestorLine, address, external, cacheLine);
        didWrite = m_L0Caches[1].SnoopBusReadX(requestorLine, address, external, didWrite ? nullptr : cacheLine);
        didWrite = m_L0Caches[2].SnoopBusReadX(requestorLine, address, external, didWrite ? nullptr : cacheLine);
        didWrite = m_L0Caches[3].SnoopBusReadX(requestorLine, address, external, didWrite ? nullptr : cacheLine);

        if(!didWrite)
        {
            // The memory granularity is 32 bits, thus we'll adjust to an 8 bit granularity for x86.
            ReadCacheLine(address, cacheLine, external);
            return false;
        }

        return true;
    }

    void UpgradeCacheLine(const u32 requestorLine, const u64 address, const bool external) noexcept
    {
        m_L0Caches[0].SnoopBusUpgrade(requestorLine, address, external);
        m_L0Caches[1].SnoopBusUpgrade(requestorLine, address, external);
        m_L0Caches[2].SnoopBusUpgrade(requestorLine, address, external);
        m_L0Caches[3].SnoopBusUpgrade(requestorLine, address, external);
    }

    void WriteBackCacheLine(const u32 requestorLine, const u64 address, const bool external, const u32* cacheLine) noexcept
    {
        // if(requestorLine == 4)
        {
            // The memory granularity is 32 bits, thus we'll adjust to an 8 bit granularity for x86.
            WriteCacheLine(address, cacheLine, external);
        }
    }
private:
    void ReadCacheLine(u64 address, u32 data[8], bool external) noexcept;
    void WriteCacheLine(u64 address, const u32 data[8], bool external) noexcept;
private:
    Processor* m_Processor;
    Cache<8, 4> m_L0Caches[4];
    // Cache<10, 8> L1Cache;
};

#include "Cache.inl"
