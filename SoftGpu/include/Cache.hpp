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
#include <Common.hpp>
#include <BitVector.hpp>

#include "IPConfig.hpp"

enum class MesiState : u8
{
    Modified = 3,
    Exclusive = 1,
    Shared = 2,
    Invalid = 0
};

template<uSys IndexBits>
struct CacheLine final
{
    DEFAULT_DESTRUCT(CacheLine);
    DELETE_CM(CacheLine);
public:
    MesiState Mesi : 2;
    u64 Tag : 61 - IndexBits; // 3 bits of offset, IndexBits bits of set index
    u64 External : 1; // Was this from external memory?
    u64 Pad : 8; // Because we're at a high level we'll pad the structure to be nice.
    u32 Data[8]; // 32 bytes / 8 words per cache line.

    CacheLine() noexcept
        : Mesi(MesiState::Invalid)
        , Tag{ }
        , External{ }
        , Pad{ }
        , Data{ }
    { }

    void Reset()
    {
        Mesi = MesiState::Invalid;
        Tag = { };
        External = { };
        Pad = { };
    }
};

template<uSys IndexBits, uSys NumSetLines>
struct CacheSet final
{
    DEFAULT_CONSTRUCT_PU(CacheSet);
    DEFAULT_DESTRUCT(CacheSet);
    DELETE_CM(CacheSet);
public:
    CacheLine<IndexBits> SetLines[NumSetLines];

    void Reset()
    {
        for(uSys i = 0; i < NumSetLines; ++i)
        {
            SetLines[i].Reset();
        }
    }
};

class CacheController;

struct NonCoherentCacheReceiverSample final
{
    void ReceiveNonCoherentCache_DataValid(const u32 index, const bool dataValid) noexcept { }
    void ReceiveNonCoherentCache_Data(const u32 index, const TBitVector<IPConfig::MEMORY_BUS_BIT_WIDTH, 3, StdLogic>& data) noexcept { }
};

template<typename Receiver, uSys IndexBits, uSys NumSetLines>
class NonCoherentCache final
{
    DEFAULT_DESTRUCT(NonCoherentCache);
    DELETE_CM(NonCoherentCache);
public:
    using DataBus_t = TBitVector<IPConfig::MEMORY_BUS_BIT_WIDTH, 3, StdLogic>;
private:
    enum class ERequestState : u32
    {
        Waiting = 0,
    };

    SENSITIVITY_DECL(p_Reset_n, p_Clock);

    SIGNAL_ENTITIES();

    STD_LOGIC_DECL(p_inout_DataBus);
public:
    NonCoherentCache(
        Receiver* const parent,
        const u32 index
    ) noexcept
        : m_Parent(parent)
        , m_Index(index)
        , p_Reset_n(0)
        , p_Clock(0)
        , p_RequestActive(0)
        , p_ReadWrite(EReadWrite::Read)
        , m_Pad0{}
        , p_TargetAddress(0)
        , p_inout_DataBus()
        , m_RequestState(ERequestState::Waiting)
        , m_Pad1{}
        , m_Sets{ }
        , m_RollingSelector(0)
    { }

    void SetResetN(const bool reset_n) noexcept
    {
        p_Reset_n = BOOL_TO_BIT(reset_n);

        TRIGGER_SENSITIVITY(p_Reset_n);
    }

    void SetClock(const bool clock) noexcept
    {
        p_Clock = BOOL_TO_BIT(clock);

        TRIGGER_SENSITIVITY(p_Clock);
    }

    void SetRequestActive(const bool requestActive) noexcept
    {
        p_RequestActive = BOOL_TO_BIT(requestActive);
    }

    void SetReadWrite(const EReadWrite readWrite) noexcept
    {
        p_ReadWrite = readWrite;
    }

    void SetTargetRequest(const u64 targetAddress) noexcept
    {
        p_TargetAddress = targetAddress;
    }

    void SetDataBus(const DataBus_t& dataBus) noexcept
    {
        STD_LOGIC_SET(p_inout_DataBus, dataBus);

        if constexpr(IPConfig::ASSERT_ON_STD_LOGIC_CONFLICT)
        {
            assert(!p_inout_DataBus.Contains(StdULogic::MultipleDrivers));
            assert(!p_inout_DataBus.Contains(StdULogic::WeakMultipleDrivers));
        }
    }
private:
    PROCESSES_DECL()
    {
        STD_LOGIC_PROCESS_RESET_HANDLER(p_Clock);
        PROCESS_ENTER(ClockHandler, p_Reset_n, p_Clock);
    }

    PROCESS_DECL(ClockHandler)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_RollingSelector = 0;

            for(uSys i = 0; i < ::std::size(m_Sets); ++i)
            {
                m_Sets[i].Reset();
            }
        }
        else if(RISING_EDGE(p_Clock))
        {
            switch(m_RequestState)
            {
                case ERequestState::Waiting:
                    if(BIT_TO_BOOL(p_RequestActive))
                    {
                        m_TargetAddress = p_TargetAddress;
                    }
                    break;
                default:
                    assert(false);
                    break;
            }
        }
    }

    [[nodiscard]] CacheLine<IndexBits>* GetCacheLine(const u64 address, const bool external) noexcept
    {
        const u64 setIndex = (address >> 3) & ((1 << IndexBits) - 1);
        const u64 tag = address >> (IndexBits + 3);

        CacheSet<IndexBits, NumSetLines>& targetSet = m_Sets[setIndex];

        for(uSys i = 0; i < NumSetLines; ++i)
        {
            if(targetSet.SetLines[i].Tag == tag && targetSet.SetLines[i].External == external)
            {
                return &targetSet.SetLines[i];
            }
        }

        return nullptr;
    }
private:
    Receiver* m_Parent;
    u32 m_Index;

    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    u32 p_RequestActive : 1;
    EReadWrite p_ReadWrite : 1;
    u32 m_Pad0 : 28;

    u64 p_TargetAddress : IPConfig::PHYSICAL_ADDRESS_BITS - IPConfig::MEMORY_BUS_BIT_WIDTH_EXPONENT;
    DataBus_t p_inout_DataBus;

    ERequestState m_RequestState : 3;
    u32 m_Pad1 : 29;

    u64 m_TargetAddress;

    CacheSet<IndexBits, NumSetLines> m_Sets[1 << IndexBits];
    u32 m_RollingSelector;
};

template<uSys IndexBits, uSys NumSetLines>
class Cache final
{
    DEFAULT_DESTRUCT(Cache);
    DELETE_CM(Cache);
private:
    using Receiver = CacheController;

    SENSITIVITY_DECL(p_Reset_n, p_Clock);

    SIGNAL_ENTITIES();
public:
    Cache(
        Receiver* const parent,
        const u32 lineIndex
    ) noexcept
        : m_Parent(parent)
        , m_LineIndex(lineIndex)
        , p_Reset_n(0)
        , p_Clock(0)
        , p_Pad0{}
        , m_Sets{ }
        , m_RollingSelector(0)
    { }

    void SetResetN(const bool reset_n) noexcept
    {
        p_Reset_n = BOOL_TO_BIT(reset_n);

        TRIGGER_SENSITIVITY(p_Reset_n);
    }

    void SetClock(const bool clock) noexcept
    {
        p_Clock = BOOL_TO_BIT(clock);

        TRIGGER_SENSITIVITY(p_Clock);
    }

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
    PROCESSES_DECL()
    {
        PROCESS_ENTER(ClockHandler, p_Reset_n, p_Clock);
    }

    PROCESS_DECL(ClockHandler)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_RollingSelector = 0;

            for(uSys i = 0; i < ::std::size(m_Sets); ++i)
            {
                m_Sets[i].Reset();
            }
        }
    }

    [[nodiscard]] CacheLine<IndexBits>* GetCacheLine(const u64 address, const bool external) noexcept
    {
        const u64 setIndex = (address >> 3) & ((1 << IndexBits) - 1);
        const u64 tag = address >> (IndexBits + 3);

        CacheSet<IndexBits, NumSetLines>& targetSet = m_Sets[setIndex];

        for(uSys i = 0; i < NumSetLines; ++i)
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
    Receiver* m_Parent;
    u32 m_LineIndex;

    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    u32 p_Pad0 : 30;


    CacheSet<IndexBits, NumSetLines> m_Sets[1 << IndexBits];
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
