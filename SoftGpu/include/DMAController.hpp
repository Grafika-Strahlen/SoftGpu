/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include "Common.hpp"
#include "BusArbiter.hpp"

class Processor;

struct DMAChannelBus final
{
    DEFAULT_CONSTRUCT_PUC(DMAChannelBus);
    DEFAULT_DESTRUCT(DMAChannelBus);
    DEFAULT_CM_PUC(DMAChannelBus);
public:
    u64 CPUPhysicalAddress;
    u64 GPUVirtualAddress;
    u64 WordCount; // If 0 then this unit is not active.
    u32 ReadWrite : 1; // If 1 then read from the CPU, if 0 write to the CPU.
    u32 Atomic : 1;
    u32 Active : 1;
    u32 Pad : 29;
};

class DMAChannel final
{
    DEFAULT_DESTRUCT(DMAChannel);
    DELETE_CM(DMAChannel);
private:
    enum class Sensitivity
    {
        Reset = 0,
        Clock,
        Active
    };

    SIGNAL_ENTITIES();
public:
    DMAChannel(Processor* const processor, const u16 dmaIndex) noexcept
        : p_Reset_n(0)
        , p_Clock(0)
        , p_out_Finished(1)
        , p_Pad0(0)
        , p_CPUPhysicalAddress(0)
        , p_GPUVirtualAddress(0)
        , p_WordCount(0)
        , p_ReadWrite(0)
        , p_Atomic(0)
        , p_Active(0)
        , p_Pad1(0)
        , p_inout_RequestNumber(0)
        , m_Processor(processor)
        , m_DmaIndex(dmaIndex)
        , m_WordsTransferred(0)
        , m_WordsInTransferBlock(0)
        , m_Pad0(0)
        , m_TransferBlock {  }
    { }

    void SetResetN(const u32 reset_n) noexcept
    {
        p_Reset_n = reset_n;

        Processes(Sensitivity::Reset);
    }

    void SetClock(const bool clock) noexcept
    {
        p_Clock = BOOL_TO_BIT(clock);

        Processes(Sensitivity::Clock);
    }

    [[nodiscard]] bool GetFinished() const noexcept { return p_out_Finished; }

    void SetCPUPhysicalAddress(const u64 cpuPhysicalAddress) noexcept
    {
        p_CPUPhysicalAddress = cpuPhysicalAddress;
    }

    void SetGPUVirtualAddress(const u64 gpuVirtualAddress) noexcept
    {
        p_GPUVirtualAddress = gpuVirtualAddress;
    }

    void SetWordCount(const u64 wordCount) noexcept
    {
        p_WordCount = wordCount;
    }

    void SetReadWrite(const bool readWrite) noexcept
    {
        p_ReadWrite = BOOL_TO_BIT(readWrite);
    }

    void SetAtomic(const bool atomic) noexcept
    {
        p_Atomic = BOOL_TO_BIT(atomic);
    }

    void SetActive(const bool active) noexcept
    {
        p_Active = BOOL_TO_BIT(active);

        Processes(Sensitivity::Active);
    }

    void SetRequestNumber(const u32 requestNumber) noexcept
    {
        p_inout_RequestNumber = requestNumber;
    }

    [[nodiscard]] u32 GetRequestNumber() const noexcept { return p_inout_RequestNumber; }
private:
    void Processes(const Sensitivity trigger) noexcept
    {
        ResetHandler(trigger);
        ClockHandler(trigger);
        ActiveHandler(trigger);
    }

    void ResetHandler(const Sensitivity trigger) noexcept
    {
        if(FallingEdge<Sensitivity::Reset>(p_Reset_n, trigger))
        {
            p_out_Finished = 1;

            p_CPUPhysicalAddress = 0;
            p_GPUVirtualAddress = 0;
            p_WordCount = 0;
            p_ReadWrite = 0;
            p_Atomic = 0;
            m_WordsTransferred = 0;
            m_WordsInTransferBlock = 0;
        }
    }

    void ClockHandler(const Sensitivity trigger) noexcept
    {
        if(RisingEdge<Sensitivity::Clock>(p_Clock, trigger))
        {
            ExecuteRead();
        }
        else if(FallingEdge<Sensitivity::Clock>(p_Clock, trigger))
        {
            ExecuteWrite();
        }
    }

    void ActiveHandler(const Sensitivity trigger) noexcept
    {
        if(RisingEdge<Sensitivity::Active>(p_Active, trigger))
        {
            p_out_Finished = 0;
        }
    }

    void ExecuteRead() noexcept;
    void ExecuteWrite() noexcept;
private:
    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    u32 p_out_Finished : 1;
    u32 p_Pad0 : 29;

    u64 p_CPUPhysicalAddress;
    u64 p_GPUVirtualAddress;
    u64 p_WordCount; // If 0 then this unit is not active.
    u32 p_ReadWrite : 1; // If 1 then read from the CPU, if 0 write to the CPU.
    u32 p_Atomic : 1;
    u32 p_Active : 1;
    u32 p_Pad1 : 29;
    u32 p_inout_RequestNumber;

    Processor* m_Processor;
    u16 m_DmaIndex;
    u64 m_WordsTransferred;
    u16 m_WordsInTransferBlock : 10;
    u16 m_Pad0 : 6;

    u32 m_TransferBlock[1024];
};

class DMAController final
{
    DEFAULT_DESTRUCT(DMAController);
    DELETE_CM(DMAController);
public:
    static inline constexpr u16 DMA_CHANNEL_COUNT = 4;
    static inline constexpr u16 PCI_CONTROLLER_BUS_INDEX_BASE = 0;
public:
    enum BusState : u32
    {
        ReadyForInput = 0,
        RespondingRequestNumber,
        CompletedRequest
    };
private:
    enum class Sensitivity
    {
        Reset = 0,
        Clock
    };

    SIGNAL_ENTITIES()
public:
    DMAController(Processor* const processor) noexcept
        : p_Reset_n(1)
        , p_Clock(0)
        , p_out_BusState(ReadyForInput)
        , p_Pad0(0)
        , p_inout_CPUPhysicalAddress(0)
        , p_GPUVirtualAddress(0)
        , p_WordCount(0)
        , p_ReadWrite(0)
        , p_Atomic(0)
        , p_Active(0)
        , p_Pad1(0)
        , m_Processor(processor)
        , m_BusArbiter()
        , m_Channels { { processor, 0 }, { processor, 1 }, { processor, 2 }, { processor, 3 } }
        , m_CurrentRequestNumber(0)
        , m_AssignmentFinished(0)
        , m_Pad0(0)
    { }

    void SetResetN(const bool reset_n) noexcept
    {
        p_Reset_n = BOOL_TO_BIT(reset_n);

        for(u32 i = 0; i < DMA_CHANNEL_COUNT; ++i)
        {
            m_Channels->SetResetN(reset_n);
        }

        m_BusArbiter.SetResetN(reset_n);

        Processes(Sensitivity::Reset);
    }

    void SetClock(const bool clock) noexcept
    {
        p_Clock = BOOL_TO_BIT(clock);

        for(u32 i = 0; i < DMA_CHANNEL_COUNT; ++i)
        {
            m_Channels->SetClock(clock);
        }

        m_BusArbiter.SetClock(clock);

        Processes(Sensitivity::Clock);
    }

    [[nodiscard]] BusState GetBusState() const noexcept { return p_out_BusState; }

    void SetCPUPhysicalAddress(const u64 cpuPhysicalAddress) noexcept
    {
        p_inout_CPUPhysicalAddress = cpuPhysicalAddress;
    }

    [[nodiscard]] u64 GetCPUPhysicalAddress() const noexcept { return p_inout_CPUPhysicalAddress; }

    void SetGPUVirtualAddress(const u64 gpuVirtualAddress) noexcept
    {
        p_GPUVirtualAddress = gpuVirtualAddress;
    }

    void SetWordCount(const u64 wordCount) noexcept
    {
        p_WordCount = wordCount;
    }

    void SetReadWrite(const bool readWrite) noexcept
    {
        p_ReadWrite = BOOL_TO_BIT(readWrite);
    }

    void SetAtomic(const bool atomic) noexcept
    {
        p_Atomic = BOOL_TO_BIT(atomic);
    }

    void SetActive(const bool active) noexcept
    {
        p_Active = BOOL_TO_BIT(active);
    }

    void SetReady(const u32 readyLineNumber, const bool ready) noexcept
    {
        m_BusArbiter.SetReady(readyLineNumber, ready);
    }

    [[nodiscard]] u32 GetBusSelect() const noexcept { return m_BusArbiter.GetBusSelect(); }
private:
    void Processes(const Sensitivity trigger) noexcept
    {
        ResetHandler(trigger);
        ClockHandler(trigger);
    }

    void ResetHandler(const Sensitivity trigger) noexcept
    {
        if(FallingEdge<Sensitivity::Reset>(p_Reset_n, trigger))
        {
            p_out_BusState = ReadyForInput;

            p_inout_CPUPhysicalAddress = 0;
            p_GPUVirtualAddress = 0;
            p_WordCount = 0;
            p_ReadWrite = 0;
            p_Atomic = 0;

            m_CurrentRequestNumber = 0;
            m_AssignmentFinished = 0;
        }
    }

    void ClockHandler(const Sensitivity trigger) noexcept
    {
        if(RisingEdge<Sensitivity::Clock>(p_Clock, trigger))
        {
            if(p_out_BusState == ReadyForInput)
            {
                if(m_BusArbiter.GetFinished())
                {
                    p_out_BusState = RespondingRequestNumber;
                    ++m_CurrentRequestNumber;
                    p_inout_CPUPhysicalAddress = m_CurrentRequestNumber;

                    TryAssignChannel();
                }
            }
            else if(p_out_BusState == RespondingRequestNumber || p_out_BusState == CompletedRequest)
            {
                TryAssignChannel();
                TrySendCompletedRequest();
            }
        }
        else if(FallingEdge<Sensitivity::Clock>(p_Clock, trigger))
        {
            if(m_AssignmentFinished)
            {
                m_BusArbiter.SetMasterReady(false);
            }
        }
    }

    void TryAssignChannel()
    {
        if(!m_AssignmentFinished)
        {
            for(u32 i = 0; i < DMA_CHANNEL_COUNT; ++i)
            {
                if(m_Channels[i].GetFinished() && m_Channels[i].GetRequestNumber() == 0)
                {
                    m_Channels[i].SetCPUPhysicalAddress(p_inout_CPUPhysicalAddress);
                    m_Channels[i].SetGPUVirtualAddress(p_GPUVirtualAddress);
                    m_Channels[i].SetWordCount(p_WordCount);
                    m_Channels[i].SetReadWrite(p_ReadWrite);
                    m_Channels[i].SetAtomic(p_Atomic);
                    m_Channels[i].SetActive(p_Active);

                    m_Channels[i].SetRequestNumber(m_CurrentRequestNumber);

                    m_AssignmentFinished = 1;
                    break;
                }
            }

            if(m_AssignmentFinished)
            {
                m_BusArbiter.SetMasterReady(true);
            }
        }
    }

    void TrySendCompletedRequest()
    {
        bool found = false;

        for(u32 i = 0; i < DMA_CHANNEL_COUNT; ++i)
        {
            if(m_Channels[i].GetFinished() && m_Channels[i].GetRequestNumber() != 0)
            {
                p_out_BusState = CompletedRequest;
                p_inout_CPUPhysicalAddress = m_Channels[i].GetRequestNumber();
                found = true;
                break;
            }
        }

        if(!found)
        {
            p_out_BusState = ReadyForInput;
            SET_HI_Z(p_inout_CPUPhysicalAddress);
        }
    }
private:
    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    BusState p_out_BusState : 2;
    u32 p_Pad0 : 29;

    u64 p_inout_CPUPhysicalAddress;
    u64 p_GPUVirtualAddress;
    u64 p_WordCount; // If 0 then this unit is not active.
    u32 p_ReadWrite : 1; // If 1 then read from the CPU, if 0 write to the CPU.
    u32 p_Atomic : 1;
    u32 p_Active : 1;
    u32 p_Pad1 : 29;

    Processor* m_Processor;
    BusArbiter<6> m_BusArbiter;
    DMAChannel m_Channels[DMA_CHANNEL_COUNT];
    u32 m_CurrentRequestNumber;
    u32 m_AssignmentFinished : 1;
    u32 m_Pad0 : 31;
};
