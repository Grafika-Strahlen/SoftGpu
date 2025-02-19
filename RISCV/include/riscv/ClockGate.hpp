#pragma once

#include <Common.hpp>
#include <Objects.hpp>
#include <NumTypes.hpp>

namespace riscv {

class ClockGate final
{
    DEFAULT_DESTRUCT(ClockGate);
    DELETE_CM(ClockGate);
private:
    enum class Sensitivity
    {
        Reset = 0,
        Clock
    };

    SIGNAL_ENTITIES();
public:
    ClockGate() noexcept
        : p_Reset_n(0)
        , p_Clock(0)
        , p_Halt(0)
        , m_Enable(0)
        , m_Pad0(0)
    { }

    void SetResetN(const bool reset_n) noexcept
    {
        p_Reset_n = BOOL_TO_BIT(reset_n);

        Processes(Sensitivity::Reset);
    }

    void SetClock(const bool clock) noexcept
    {
        p_Clock = BOOL_TO_BIT(clock);

        Processes(Sensitivity::Clock);
    }

    void SetHalt(const bool halt) noexcept
    {
        p_Halt = BOOL_TO_BIT(halt);
    }

    [[nodiscard]] bool GetClock() const noexcept
    {
        if(BIT_TO_BOOL(m_Enable))
        {
            return p_Clock;
        }
        else
        {
            return false;
        }
    }
private:
    void Processes(const Sensitivity trigger) noexcept
    {
        ClockSwitch(trigger);
    }

    void ClockSwitch(const Sensitivity trigger) noexcept
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_Enable = 1;
        }
        else if(FallingEdge<Sensitivity::Clock>(p_Clock, trigger))
        {
            m_Enable = BOOL_TO_BIT(!BIT_TO_BOOL(p_Halt));
        }
    }
private:
    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    u32 p_Halt : 1;

    u32 m_Enable : 1;
    u32 m_Pad0 : 28;
};

}
