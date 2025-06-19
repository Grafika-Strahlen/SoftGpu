#pragma once

#include <Common.hpp>
#include <Objects.hpp>
#include <NumTypes.hpp>

namespace riscv {

class ClockGateReceiverSample
{
public:
    void ReceiveClockGate_Clock(const u32 index, const bool clock) noexcept { }
};

template<typename Receiver = ClockGateReceiverSample>
class ClockGate final
{
    DEFAULT_DESTRUCT(ClockGate);
    DELETE_CM(ClockGate);
private:
    SENSITIVITY_DECL(p_Reset_n, p_Clock);

    SIGNAL_ENTITIES();
public:
    ClockGate(Receiver* const parent, const u32 index = 0) noexcept
        : m_Parent(parent)
        , m_Index(index)
        , p_Reset_n(0)
        , p_Clock(0)
        , p_Halt(0)
        , m_Enable(0)
        , m_Pad0(0)
    { }

    void SetResetN(const bool reset_n) noexcept
    {
        p_Reset_n = BOOL_TO_BIT(reset_n);

        TRIGGER_SENSITIVITY(p_Reset_n);
    }

    void SetClock(const bool clock) noexcept
    {
        p_Clock = BOOL_TO_BIT(clock);

        // This is just a mux, so it needs to check before the process.
        if(BIT_TO_BOOL(m_Enable))
        {
            m_Parent->ReceiveClockGate_Clock(m_Index, p_Clock);
        }

        TRIGGER_SENSITIVITY(p_Clock);
    }

    void SetHalt(const bool halt) noexcept
    {
        p_Halt = BOOL_TO_BIT(halt);
    }
private:
    PROCESSES_DECL()
    {
        PROCESS_ENTER(ClockSwitch, p_Reset_n, p_Clock)
    }

    PROCESS_DECL(ClockSwitch)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_Enable = 1;
        }
        else if(FALLING_EDGE(p_Clock))
        {
            m_Enable = BOOL_TO_BIT(!BIT_TO_BOOL(p_Halt));
        }
    }
private:
    Receiver* m_Parent;
    u32 m_Index;

    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    u32 p_Halt : 1;

    u32 m_Enable : 1;
    [[maybe_unused]] u32 m_Pad0 : 28;
};

}
