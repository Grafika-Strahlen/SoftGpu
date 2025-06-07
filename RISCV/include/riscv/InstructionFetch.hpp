#pragma once

#include <Common.hpp>
#include "ControlBus.hpp"
#include "MemoryBus.hpp"

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

class InstructionFetch final
{
    DEFAULT_DESTRUCT(InstructionFetch);
    DELETE_CM(InstructionFetch);
private:
    SENSITIVITY_DECL(p_Reset_n, p_Clock);

    SIGNAL_ENTITIES();

    enum class FetchState : u32
    {
        Restart,
        Request,
        Pending
    };
public:
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
                    
                    break;
                case FetchState::Pending:
                    m_Restart |= p_ControlBus.IF_Reset;
                    if(BIT_TO_BOOL(m_BusResponse))
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
    }
private:
    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    u32 m_Pad0 : 30;

    ControlBus p_ControlBus;

    MemoryBusRequest p_out_MemoryRequest;
    MemoryBusResponse p_MemoryResponse;

    InstructionFetchBus p_out_InstructionFetch;

    FetchState m_FetchState : 2;
    u32 m_Restart : 1;
    u32 m_BusResponse : 1;
    u32 m_Pad1 : 28;
    u32 m_ProgramCounter;

};

}
