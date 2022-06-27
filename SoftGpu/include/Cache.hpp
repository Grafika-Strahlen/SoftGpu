#pragma once

#include <cstring>

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
    u64 Pad : 9; // Because we're at a high level we'll pad the structure to be nice.
    u32 Data[8]; // 32 bytes / 8 words per cache line.

    CacheLine() noexcept
        : Mesi(MESI::Invalid)
        , Tag{ }
        , Pad{ }
        , Data{ }
    { }
};

template<uSys IndexBits, uSys SetLineCount>
struct CacheSet final
{
    DEFAULT_CONSTRUCT_PU(CacheSet);
    DEFAULT_DESTRUCT(CacheSet);
    DELETE_CM(CacheSet);
public:
    CacheLine<IndexBits> SetLines[SetLineCount];
};

class MemoryManager;

template<uSys IndexBits, uSys SetLineCount>
class Cache final
{
    DEFAULT_DESTRUCT(Cache);
    DELETE_CM(Cache);
public:
    Cache(MemoryManager* const memoryManager, const u32 lineIndex) noexcept
        : m_MemoryManager(memoryManager)
        , m_LineIndex(lineIndex)
        , m_Sets{ }
        , m_RollingSelector(0)
    { }

    [[nodiscard]] u32 Read(u64 address) noexcept;
    void Write(u64 address, u32 value) noexcept;
    // void FillCacheLine(u64 address, const u32* data) noexcept;

    bool SnoopBusRead(u32 requestorLine, u64 address, u32* dataBus) noexcept;
    bool SnoopBusReadX(u32 requestorLine, u64 address, u32* dataBus) noexcept;
    void SnoopBusUpgrade(u32 requestorLine, u64 address) noexcept;
private:
    [[nodiscard]] CacheLine<IndexBits>* GetCacheLine(const u64 address) noexcept
    {
        const u64 setIndex = (address >> 3) & ((1 << IndexBits) - 1);
        const u64 tag = address >> (IndexBits + 3);

        CacheSet<IndexBits, SetLineCount>& targetSet = m_Sets[setIndex];

        for(uSys i = 0; i < SetLineCount; ++i)
        {
            if(targetSet.SetLines[i].Tag == tag)
            {
                return &targetSet.SetLines[i];
            }
        }

        return nullptr;
    }

    [[nodiscard]] CacheLine<IndexBits>* GetFreeCacheLine(u64 address) noexcept;
private:
    MemoryManager* m_MemoryManager;
    u32 m_LineIndex;
    CacheSet<IndexBits, SetLineCount> m_Sets[1 << IndexBits];
    u32 m_RollingSelector;
};

class Processor;

class MemoryManager final
{
public:
    MemoryManager(Processor* const processor) noexcept
        : m_Processor(processor)
        , m_L0Caches{ { this, 0 }, { this, 1 }, { this, 2 }, { this, 3 } }
        // , L1Cache(this, 4)
    { }

    [[nodiscard]] u32 Read(const u32 coreIndex, const u64 address) noexcept
    {
        return m_L0Caches[coreIndex].Read(address);
    }

    void Write(const u32 coreIndex, const u64 address, const u32 value) noexcept
    {
        m_L0Caches[coreIndex].Write(address, value);
    }

    void Prefetch(const u32 coreIndex, const u64 address) noexcept
    {
        // For pre-fetching we'll be acting asynchronously typically, but we're forced to act synchronously in software, so we'll just redirect to read.
        (void) m_L0Caches[coreIndex].Read(address);
    }

    bool ReadCacheLine(const u32 requestorLine, const u64 address, u32* const cacheLine) noexcept
    {
        bool didWrite = m_L0Caches[0].SnoopBusRead(requestorLine, address, cacheLine);
        didWrite = m_L0Caches[1].SnoopBusRead(requestorLine, address, didWrite ? nullptr : cacheLine);
        didWrite = m_L0Caches[2].SnoopBusRead(requestorLine, address, didWrite ? nullptr : cacheLine);
        didWrite = m_L0Caches[3].SnoopBusRead(requestorLine, address, didWrite ? nullptr : cacheLine);

        if(!didWrite)
        {
            // The memory granularity is 32 bits, thus we'll adjust to an 8 bit granularity for x86.
            const uintptr_t addressX86 = address << 2;
            ::std::memcpy(cacheLine, reinterpret_cast<void*>(addressX86), sizeof(u32[8]));
            return false;
        }

        return true;
    }

    bool ReadXCacheLine(const u32 requestorLine, const u64 address, u32* const cacheLine) noexcept
    {
        bool didWrite = m_L0Caches[0].SnoopBusReadX(requestorLine, address, cacheLine);
        didWrite = m_L0Caches[1].SnoopBusReadX(requestorLine, address, didWrite ? nullptr : cacheLine);
        didWrite = m_L0Caches[2].SnoopBusReadX(requestorLine, address, didWrite ? nullptr : cacheLine);
        didWrite = m_L0Caches[3].SnoopBusReadX(requestorLine, address, didWrite ? nullptr : cacheLine);

        if(!didWrite)
        {
            // The memory granularity is 32 bits, thus we'll adjust to an 8 bit granularity for x86.
            const uintptr_t addressX86 = address << 2;
            ::std::memcpy(cacheLine, reinterpret_cast<void*>(addressX86), sizeof(u32[8]));
            return false;
        }

        return true;
    }

    void UpgradeCacheLine(const u32 requestorLine, const u64 address) noexcept
    {
        m_L0Caches[0].SnoopBusUpgrade(requestorLine, address);
        m_L0Caches[1].SnoopBusUpgrade(requestorLine, address);
        m_L0Caches[2].SnoopBusUpgrade(requestorLine, address);
        m_L0Caches[3].SnoopBusUpgrade(requestorLine, address);
    }

    void WriteBackCacheLine(const u32 requestorLine, const u64 address, const u32* cacheLine) noexcept
    {
        if(requestorLine == 4)
        {
            // The memory granularity is 32 bits, thus we'll adjust to an 8 bit granularity for x86.
            const uintptr_t addressX86 = address << 2;
            ::std::memcpy(reinterpret_cast<void*>(addressX86), cacheLine, sizeof(u32[8]));
        }
    }
private:
    Processor* m_Processor;
    Cache<8, 4> m_L0Caches[4];
    // Cache<10, 8> L1Cache;
};

#include "Cache.inl"
