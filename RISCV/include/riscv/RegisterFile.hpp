#pragma once

#include <Common.hpp>
#include "ControlBus.hpp"

namespace riscv {

class RegisterFileReceiverSample
{
public:
    void ReceiveRegisterFile_RS1(const u32 index, const u32 rs1) noexcept { }
    void ReceiveRegisterFile_RS2(const u32 index, const u32 rs2) noexcept { }
    void ReceiveRegisterFile_RS3(const u32 index, const u32 rs3) noexcept { }
};

template<typename Receiver = RegisterFileReceiverSample, bool EnableE = true, bool EnableRS3 = true>
class RegisterFile final
{
    DEFAULT_DESTRUCT(RegisterFile);
    DELETE_CM(RegisterFile);
private:
    SENSITIVITY_DECL(p_Reset_n, p_Clock);

    SIGNAL_ENTITIES();
public:
    constexpr static inline u32 AddressBitCount = EnableE ? 4 : 5;
    constexpr static inline u32 NumRegisters = 1 << AddressBitCount;
    constexpr static inline u32 RegisterMask = NumRegisters - 1;
public:
    RegisterFile(Receiver* const parent, const u32 index = 0) noexcept
        : m_Parent(parent)
        , m_Index(index)
        , p_Reset_n(0)
        , p_Clock(0)
        , m_Pad0(0)
        , p_ControlBus{ }
        , p_RD(0)
        , m_Registers{ }
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

    void SetControlBus(const ControlBus& controlBus) noexcept
    {
        p_ControlBus = controlBus;
    }

    void SetRD(const u32 rd) noexcept
    {
        p_RD = rd;
    }
private:
    PROCESSES_DECL()
    {
        PROCESS_ENTER(WriteHandler, p_Reset_n, p_Clock);
        PROCESS_ENTER(SynchronousReadHandler, p_Reset_n, p_Clock);
        if constexpr(EnableRS3)
        {
            PROCESS_ENTER(Rs3ReadHandler, p_Clock);
        }
    }

    PROCESS_DECL(WriteHandler)
    {
        for(u32 i = 1; i < NumRegisters; ++i)
        {
            if(!BIT_TO_BOOL(p_Reset_n))
            {
                m_Registers[i] = 0;
            }
            else if(RISING_EDGE(p_Clock))
            {
                if(p_ControlBus.RF_WriteBackEnable && (p_ControlBus.RF_RD_Address & RegisterMask) == i)
                {
                    m_Registers[i] = p_RD;
                }
            }
        }
    }

    PROCESS_DECL(SynchronousReadHandler)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_Parent->ReceiveRegisterFile_RS1(m_Index, 0);
            m_Parent->ReceiveRegisterFile_RS2(m_Index, 0);
        }
        else if(RISING_EDGE(p_Clock))
        {
            m_Parent->ReceiveRegisterFile_RS1(m_Index, m_Registers[p_ControlBus.RF_RS1_Address & RegisterMask]);
            m_Parent->ReceiveRegisterFile_RS2(m_Index, m_Registers[p_ControlBus.RF_RS2_Address & RegisterMask]);
        }
    }

    PROCESS_DECL(Rs3ReadHandler)
    {
        if(RISING_EDGE(p_Clock))
        {
            m_Parent->ReceiveRegisterFile_RS3(m_Index, m_Registers[(p_ControlBus.IR_Function12 >> 7) & RegisterMask]);
        }
    }
private:
    Receiver* m_Parent;
    u32 m_Index;

    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    u32 m_Pad0 : 30;
    ControlBus p_ControlBus;

    u32 p_RD;

    u32 m_Registers[NumRegisters];
};

}
