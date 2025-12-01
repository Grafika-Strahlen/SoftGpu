/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <Common.hpp>
#include "ControlBus.hpp"
#include "MemoryBus.hpp"
#include "FIFO.hpp"
#include "RISCVCommon.hpp"

namespace riscv {

struct InstructionFetchBus final
{
    DEFAULT_CONSTRUCT_PU(InstructionFetchBus);
    DEFAULT_DESTRUCT(InstructionFetchBus);
    DEFAULT_CM_PU(InstructionFetchBus);
public:
    u32 Instruction;
    u32 Valid : 1;
    u32 Error : 1;
    u32 Pad0 : 30;
};

struct InstructionDataPacked final
{
    DEFAULT_CONSTRUCT_PU(InstructionDataPacked);
    DEFAULT_DESTRUCT(InstructionDataPacked);
    DEFAULT_CM_PU(InstructionDataPacked);
public:
    u16 Data;
    u16 Error : 1;
    u16 Pad : 15;

    InstructionDataPacked(
        const u16 data,
        const bool error
    ) noexcept
        : Data(data)
        , Error(BOOL_TO_BIT(error))
        , Pad(0)
    { }
};

struct InstructionPrefetchBuffer final
{
    DEFAULT_CONSTRUCT_PU(InstructionPrefetchBuffer);
    DEFAULT_DESTRUCT(InstructionPrefetchBuffer);
    DEFAULT_CM_PU(InstructionPrefetchBuffer);
public:
    InstructionDataPacked Data[2];
    u32 Enable : 2;
    u32 State : 2;
    u32 Pad0 : 26;
};

class InstructionFetchReceiverSample
{
public:
    void ReceiveInstructionFetch_Bus(const u32 index, const InstructionFetchBus& bus) noexcept { }
    void ReceiveInstructionFetch_MemoryRequest(const u32 index, const MemoryBusRequest& bus) noexcept { }
};

template<typename Receiver = InstructionFetchReceiverSample>
class InstructionFetch final
{
    DEFAULT_DESTRUCT(InstructionFetch);
    DELETE_CM(InstructionFetch);
private:
    SENSITIVITY_DECL(p_Reset_n, p_Clock);

    SIGNAL_ENTITIES();

    enum class FetchState : u32  // NOLINT(performance-enum-size)
    {
        Restart,
        Request,
        Pending
    };
public:
    InstructionFetch(Receiver* const parent, const u32 index = 0) noexcept
        : m_Parent(parent)
        , m_Index(index)
        , p_Reset_n(0)
        , p_Clock(0)
        , m_Pad0(0)
        , p_ControlBus()
        , p_out_MemoryRequest()
        , p_MemoryResponse()
        , p_out_InstructionFetch()
        , m_FetchState(FetchState::Restart)
        , m_Restart(0)
        , m_Pad1(0)
        , m_ProgramCounter(0)
        , m_WritePrefetchBuffer()
        , m_ReadPrefetchBuffer()
        , m_PrefetchFifo { { this, 0 }, { this, 1 } }
    { }

    void SetResetN(const bool reset_n) noexcept
    {
        p_Reset_n = BOOL_TO_BIT(reset_n);

        m_PrefetchFifo[0].SetResetN(reset_n);
        m_PrefetchFifo[1].SetResetN(reset_n);

        TRIGGER_SENSITIVITY(p_Reset_n);
    }

    void SetClock(const bool clock) noexcept
    {
        p_Clock = BOOL_TO_BIT(clock);

        m_PrefetchFifo[0].SetClock(clock);
        m_PrefetchFifo[1].SetClock(clock);

        TRIGGER_SENSITIVITY(p_Clock);
    }

    void SetControlBus(const ControlBus& controlBus) noexcept
    {
        p_ControlBus = controlBus;

        UpdateMemoryRequest();

        m_PrefetchFifo[0].SetReadEnable(ReadPrefetchBuffer_Enable0());
        m_PrefetchFifo[1].SetReadEnable(ReadPrefetchBuffer_Enable1());
    }

    void SetMemoryResponse(const MemoryBusResponse& response) noexcept
    {
        p_MemoryResponse = response;

        m_PrefetchFifo[0].SetWriteData(InstructionDataPacked(WritePrefetchBuffer_Data0(), WritePrefetchBuffer_Error0()));
        m_PrefetchFifo[0].SetWriteEnable(WritePrefetchBuffer_Enable0());
        m_PrefetchFifo[0].SetReadEnable(ReadPrefetchBuffer_Enable0());


        m_PrefetchFifo[1].SetWriteData(InstructionDataPacked(WritePrefetchBuffer_Data1(), WritePrefetchBuffer_Error1()));
        m_PrefetchFifo[1].SetWriteEnable(WritePrefetchBuffer_Enable1());
        m_PrefetchFifo[1].SetReadEnable(ReadPrefetchBuffer_Enable1());
    }
public:
    void ReceiveFIFO_HalfFull(const u32 index, const bool halfFull) noexcept
    { }

    void ReceiveFIFO_Level(const u32 index, const u32 level) noexcept
    { }

    void ReceiveFIFO_Free(const u32 index, const bool free) noexcept
    {
        m_WritePrefetchBuffer.State = SetBit(m_WritePrefetchBuffer.State, index, free);

        UpdateMemoryRequest();
        UpdateInstructionFetchBus();
    }

    void ReceiveFIFO_ReadData(const u32 index, const InstructionDataPacked& data) noexcept
    {
        m_ReadPrefetchBuffer.Data[index] = data;

        UpdateInstructionFetchBus();
    }

    void ReceiveFIFO_Available(const u32 index, const bool available) noexcept
    {
        m_ReadPrefetchBuffer.State = SetBit(m_ReadPrefetchBuffer.State, index, available);

        UpdateInstructionFetchBus();
    }
private:
    void SetFetchState(const FetchState fetchState) noexcept
    {
        m_FetchState = fetchState;

        UpdateMemoryRequest();
        UpdateInstructionFetchBus();

        m_PrefetchFifo[0].SetWriteEnable(WritePrefetchBuffer_Enable0());
        m_PrefetchFifo[1].SetWriteEnable(WritePrefetchBuffer_Enable1());
    }

    void SetRestart(const bool restart) noexcept
    {
        m_Restart = BOOL_TO_BIT(restart);

        m_PrefetchFifo[0].SetClear(restart);
        m_PrefetchFifo[1].SetClear(restart);
    }

    void SetProgramCounter(const u32 programCounter) noexcept
    {
        m_ProgramCounter = programCounter;

        UpdateMemoryRequest();
    }

    void SetPrivileged(const u32 privileged) noexcept
    {
        m_Privileged = privileged;

        UpdateMemoryRequest();
    }

    void UpdateMemoryRequest() noexcept
    {
        p_out_MemoryRequest.Address = MemoryRequest_Address();
        p_out_MemoryRequest.WriteData = 0;
        p_out_MemoryRequest.ByteEnable = 0b1111;
        p_out_MemoryRequest.ReadWrite = 0; // Read Only
        p_out_MemoryRequest.Strobe = BOOL_TO_BIT(MemoryRequest_Strobe());
        p_out_MemoryRequest.Source = 1; // Instruction Fetch
        p_out_MemoryRequest.Atomic = 0; // Not Atomic
        p_out_MemoryRequest.AtomicOperation = 0; // Not Atomic
        p_out_MemoryRequest.Privileged = m_Privileged;
        p_out_MemoryRequest.Debug = p_ControlBus.CPU_Debug;
        p_out_MemoryRequest.Fence = p_ControlBus.IF_Fence;

        m_Parent->ReceiveInstructionFetch_MemoryRequest(m_Index, p_out_MemoryRequest);
    }

    void UpdateInstructionFetchBus() noexcept
    {
        p_out_InstructionFetch.Instruction = InstructionFetch_Instruction();
        p_out_InstructionFetch.Valid = BOOL_TO_BIT(InstructionFetch_Valid());
        p_out_InstructionFetch.Error = BOOL_TO_BIT(InstructionFetch_Error());

        m_Parent->ReceiveInstructionFetch_Bus(m_Index, p_out_InstructionFetch);
    }

    [[nodiscard]] u32 MemoryRequest_Address() const noexcept
    {
        // Align the PC to the 4 byte boundary.
        return m_ProgramCounter & ~(0x3);
    }

    [[nodiscard]] bool MemoryRequest_Strobe() const noexcept
    {
        return m_FetchState == FetchState::Request && InstructionPrefetchBus_Free() == 0b11;
    }

    [[nodiscard]] u32 InstructionPrefetchBus_Free() const noexcept
    {
        return m_WritePrefetchBuffer.State;
    }

    [[nodiscard]] u32 InstructionPrefetchBus_Available() const noexcept
    {
        return m_ReadPrefetchBuffer.State;
    }

    [[nodiscard]] u16 WritePrefetchBuffer_Data0() const noexcept
    {
        return static_cast<u16>(p_MemoryResponse.Data & 0xFFFF);
    }

    [[nodiscard]] u16 WritePrefetchBuffer_Data1() const noexcept
    {
        return static_cast<u16>(p_MemoryResponse.Data >> 16);
    }

    [[nodiscard]] bool WritePrefetchBuffer_Error0() const noexcept
    {
        return BIT_TO_BOOL(p_MemoryResponse.Error);
    }

    [[nodiscard]] bool WritePrefetchBuffer_Error1() const noexcept
    {
        return BIT_TO_BOOL(p_MemoryResponse.Error);
    }

    [[nodiscard]] bool WritePrefetchBuffer_Enable0() const noexcept
    {
        return m_FetchState == FetchState::Pending && BIT_TO_BOOL(p_MemoryResponse.Acknowledge);
    }

    [[nodiscard]] bool WritePrefetchBuffer_Enable1() const noexcept
    {
        return m_FetchState == FetchState::Pending && BIT_TO_BOOL(p_MemoryResponse.Acknowledge);
    }

    [[nodiscard]] bool ReadPrefetchBuffer_Enable0() const noexcept
    {
        return BIT_TO_BOOL(p_ControlBus.IF_Acknowledge);
    }

    [[nodiscard]] bool ReadPrefetchBuffer_Enable1() const noexcept
    {
        return BIT_TO_BOOL(p_ControlBus.IF_Acknowledge);
    }

    [[nodiscard]] bool Issue_Error() const noexcept
    {
        return m_ReadPrefetchBuffer.Data[0].Error;
    }

    [[nodiscard]] u32 Issue_Instruction() const noexcept
    {
        return (static_cast<u32>(m_ReadPrefetchBuffer.Data[1].Data) << 16) | m_ReadPrefetchBuffer.Data[0].Data;
    }

    [[nodiscard]] u32 InstructionFetch_Instruction() const noexcept
    {
        return Issue_Instruction();
    }

    [[nodiscard]] bool InstructionFetch_Valid() const noexcept
    {
        return BIT_TO_BOOL(InstructionPrefetchBus_Available() & 0x1);
    }

    [[nodiscard]] bool InstructionFetch_Error() const noexcept
    {
        return Issue_Error();
    }
private:
    PROCESSES_DECL()
    {
        PROCESS_ENTER(FetchHandler, p_Reset_n, p_Clock);
    }

    PROCESS_DECL(FetchHandler)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            SetFetchState(FetchState::Restart);
            SetRestart(true);
            SetProgramCounter(0);
            SetPrivileged(PrivilegeModeMachine);
        }
        else if(RISING_EDGE(p_Clock))
        {
            switch(m_FetchState)
            {
                case FetchState::Request:
                    SetRestart(m_Restart || BIT_TO_BOOL(p_ControlBus.IF_Reset));

                    // Does the Instruction Prefetch Buffer have space?
                    if(InstructionPrefetchBus_Free() == 0b11)
                    {
                        SetFetchState(FetchState::Pending);
                    }
                    if(BIT_TO_BOOL(m_Restart) || BIT_TO_BOOL(p_ControlBus.IF_Reset))
                    {
                        SetFetchState(FetchState::Restart);
                    }
                    break;
                case FetchState::Pending:
                    SetRestart(m_Restart || BIT_TO_BOOL(p_ControlBus.IF_Reset));
                    if(BIT_TO_BOOL(p_MemoryResponse.Acknowledge))
                    {
                        SetProgramCounter((m_ProgramCounter + 4) & 0xFFFFFFFC);
                        if(BIT_TO_BOOL(m_Restart) || BIT_TO_BOOL(p_ControlBus.IF_Reset))
                        {
                            SetFetchState(FetchState::Restart);
                        }
                        else
                        {
                            SetFetchState(FetchState::Request);
                        }
                    }
                    break;
                case FetchState::Restart:
                default:
                    SetRestart(false);
                    SetProgramCounter(p_ControlBus.PC_Next);
                    SetPrivileged(BIT_TO_BOOL(p_ControlBus.CPU_Privileged));
                    SetFetchState(FetchState::Request);
                    break;
            }
        }
    }
private:
    Receiver* m_Parent;
    u32 m_Index;

    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    [[maybe_unused]] u32 m_Pad0 : 30;  // NOLINT(clang-diagnostic-unused-private-field)

    ControlBus p_ControlBus;

    MemoryBusRequest p_out_MemoryRequest;
    MemoryBusResponse p_MemoryResponse;

    InstructionFetchBus p_out_InstructionFetch;

    FetchState m_FetchState : 2;
    u32 m_Restart : 1;
    u32 m_Privileged : 1;
    [[maybe_unused]] u32 m_Pad1 : 28;

    u32 m_ProgramCounter;

    InstructionPrefetchBuffer m_WritePrefetchBuffer;
    InstructionPrefetchBuffer m_ReadPrefetchBuffer;

    FIFO<InstructionFetch, InstructionDataPacked, 1, false, false, false, false> m_PrefetchFifo[2];
};

}
