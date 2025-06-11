#pragma once

#include <Common.hpp>
#include "ControlBus.hpp"
#include "MemoryBus.hpp"
#include "FIFO.hpp"

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
    u32 Halted : 1;
    u32 Pad0 : 29;
};

struct InstructionPrefetchBuffer final
{
    DEFAULT_CONSTRUCT_PU(InstructionPrefetchBuffer);
    DEFAULT_DESTRUCT(InstructionPrefetchBuffer);
    DEFAULT_CM_PU(InstructionPrefetchBuffer);
public:
    u16 Data[2];
    u32 Error : 2;
    u32 Enable : 2;
    u32 State : 2;
    u32 Pad0 : 26;
};

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
    InstructionFetch() noexcept
        : p_Reset_n(0)
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

    void SetMemoryResponse(const MemoryBusResponse& response) noexcept
    {
        p_MemoryResponse = response;

        m_PrefetchFifo[0].SetWriteData(WritePrefetchBuffer_Data0());
        m_PrefetchFifo[0].SetWriteEnable(WritePrefetchBuffer_Enable0());
        m_PrefetchFifo[0].SetReadEnable(ReadPrefetchBuffer_Enable0());


        m_PrefetchFifo[1].SetWriteData(WritePrefetchBuffer_Data1());
        m_PrefetchFifo[1].SetWriteEnable(WritePrefetchBuffer_Enable1());
        m_PrefetchFifo[1].SetReadEnable(ReadPrefetchBuffer_Enable1());
    }

    [[nodiscard]] const MemoryBusRequest& GetMemoryRequest() noexcept
    {
        p_out_MemoryRequest.Address = MemoryRequest_Address();
        p_out_MemoryRequest.Strobe = BOOL_TO_BIT(MemoryRequest_Strobe());
        return p_out_MemoryRequest;
    }

    [[nodiscard]] const InstructionFetchBus& GetInstructionFetch() const noexcept
    {
        return p_out_InstructionFetch;
    }
public:
    void ReceiveFIFO_HalfFull(const u32 index, const bool halfFull) noexcept
    { }

    void ReceiveFIFO_Level(const u32 index, const u32 level) noexcept
    { }

    void ReceiveFIFO_Free(const u32 index, const bool free) noexcept
    {
        m_WritePrefetchBuffer.State = SetBit(m_WritePrefetchBuffer.State, index, free);
    }

    void ReceiveFIFO_ReadData(const u32 index, const u16 data) noexcept
    {
        m_ReadPrefetchBuffer.Data[index] = data;
    }

    void ReceiveFIFO_Available(const u32 index, const bool available) noexcept
    {
        m_ReadPrefetchBuffer.State = SetBit(m_ReadPrefetchBuffer.State, index, available);
    }
private:
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

    [[nodiscard]] bool BusResponse() const noexcept
    {
        return BIT_TO_BOOL(p_MemoryResponse.Acknowledge) || BIT_TO_BOOL(p_MemoryResponse.Error);
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
        return m_FetchState == FetchState::Pending && BusResponse();
    }

    [[nodiscard]] bool WritePrefetchBuffer_Enable1() const noexcept
    {
        return m_FetchState == FetchState::Pending && BusResponse();
    }

    [[nodiscard]] bool ReadPrefetchBuffer_Enable0() const noexcept
    {
        return true && p_ControlBus.IF_Acknowledge;
    }

    [[nodiscard]] bool ReadPrefetchBuffer_Enable1() const noexcept
    {
        return true && p_ControlBus.IF_Acknowledge;
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
            m_FetchState = FetchState::Restart;
            m_Restart = 1;
            m_ProgramCounter = 0;
        }
        else if(RISING_EDGE(p_Clock))
        {
            switch(m_FetchState)
            {
                case FetchState::Request:
                    m_Restart |= p_ControlBus.IF_Reset;

                    // Does the Instruction Prefetch Buffer have space.
                    if(InstructionPrefetchBus_Free() == 0b11)
                    {
                        m_FetchState = FetchState::Pending;
                    }
                    if(BIT_TO_BOOL(m_Restart) || BIT_TO_BOOL(p_ControlBus.IF_Reset))
                    {
                        m_FetchState = FetchState::Restart;
                    }
                    break;
                case FetchState::Pending:
                    m_Restart |= p_ControlBus.IF_Reset;
                    if(BusResponse())
                    {
                        m_ProgramCounter += 4;
                        m_ProgramCounter &= 0xFFFFFFFC;
                        if(BIT_TO_BOOL(m_Restart) || BIT_TO_BOOL(p_ControlBus.IF_Reset))
                        {
                            m_FetchState = FetchState::Restart;
                        }
                        else
                        {
                            m_FetchState = FetchState::Request;
                        }
                    }
                    break;
                case FetchState::Restart:
                default:
                    m_Restart = 0;
                    m_ProgramCounter = p_ControlBus.PC_Next;
                    m_FetchState = FetchState::Request;
                    break;
            }
        }

        m_PrefetchFifo[0].SetClear(BIT_TO_BOOL(m_Restart));
        m_PrefetchFifo[1].SetClear(BIT_TO_BOOL(m_Restart));
    }
private:
    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    u32 m_Pad0 : 30;  // NOLINT(clang-diagnostic-unused-private-field)

    ControlBus p_ControlBus;

    MemoryBusRequest p_out_MemoryRequest;
    MemoryBusResponse p_MemoryResponse;

    InstructionFetchBus p_out_InstructionFetch;

    FetchState m_FetchState : 2;
    u32 m_Restart : 1;
    u32 m_Pad1 : 29;  // NOLINT(clang-diagnostic-unused-private-field)

    u32 m_ProgramCounter;

    InstructionPrefetchBuffer m_WritePrefetchBuffer;
    InstructionPrefetchBuffer m_ReadPrefetchBuffer;

    FIFO<InstructionFetch, u16, 2, false, false, false> m_PrefetchFifo[2];
};

}
