#pragma once

#include <Common.hpp>
#include "ControlBus.hpp"

namespace riscv {

template<bool EnableE = true, bool EnableRS3 = true>
class RegisterFile final
{
    DEFAULT_DESTRUCT(RegisterFile);
    DELETE_CM(RegisterFile);
private:
    enum class Sensitivity
    {
        Reset = 0,
        Clock
    };

    SIGNAL_ENTITIES();
public:
    constexpr static inline u32 AddressBitCount = EnableE ? 4 : 5;
    constexpr static inline u32 NumRegisters = 1 << AddressBitCount;
    constexpr static inline u32 RegisterMask = NumRegisters - 1;
public:
    RegisterFile()
        : p_Reset_n(0)
        , p_Clock(0)
        , m_Pad0(0)
        , p_ControlBus{ }
        , p_RD(0)
        , p_out_RS1(0)
        , p_out_RS2(0)
        , p_out_RS3(0)
        , m_Registers{ }
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

    void SetControlBus(const ControlBus& controlBus) noexcept
    {
        p_ControlBus = controlBus;
    }

    void SetRD(const u32 rd) noexcept
    {
        p_RD = rd;
    }

    [[nodiscard]] u32 GetRS1() const noexcept { return p_out_RS1; }
    [[nodiscard]] u32 GetRS2() const noexcept { return p_out_RS2; }
    [[nodiscard]] u32 GetRS3() const noexcept { return p_out_RS3; }
private:
    void Processes(const Sensitivity trigger) noexcept
    {
        WriteHandler(trigger);
        SynchronousReadHandler(trigger);
        if constexpr(EnableRS3)
        {
            Rs3ReadHandler(trigger);
        }
    }

    void WriteHandler(const Sensitivity trigger) noexcept
    {
        if(Event<Sensitivity::Reset>(trigger) || Event<Sensitivity::Clock>(trigger))
        {
            for(u32 i = 1; i < NumRegisters; ++i)
            {
                if(!BIT_TO_BOOL(p_Reset_n))
                {
                    m_Registers[i] = 0;
                }
                else if(RisingEdge<Sensitivity::Clock>(p_Clock, trigger))
                {
                    if(p_ControlBus.RF_WriteBackEnable && (p_ControlBus.RF_RD_Address & RegisterMask) == i)
                    {
                        m_Registers[i] = p_RD;
                    }
                }
            }
        }
    }

    void SynchronousReadHandler(const Sensitivity trigger) noexcept
    {
        if(Event<Sensitivity::Reset>(trigger) || Event<Sensitivity::Clock>(trigger))
        {
            if(!BIT_TO_BOOL(p_Reset_n))
            {
                p_out_RS1 = 0;
                p_out_RS2 = 0;
            }
            else if(RisingEdge<Sensitivity::Clock>(p_Clock, trigger))
            {
                p_out_RS1 = m_Registers[p_ControlBus.RF_RS1_Address & RegisterMask];
                p_out_RS2 = m_Registers[p_ControlBus.RF_RS2_Address & RegisterMask];
            }
        }
    }

    void Rs3ReadHandler(const Sensitivity trigger) noexcept
    {
        if(RisingEdge<Sensitivity::Clock>(p_Clock, trigger))
        {
            p_out_RS3 = m_Registers[(p_ControlBus.IR_Funct12 >> 7) & RegisterMask];
        }
    }
private:
    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    u32 m_Pad0 : 30;
    ControlBus p_ControlBus;

    u32 p_RD;
    u32 p_out_RS1;
    u32 p_out_RS2;
    u32 p_out_RS3;

    u32 m_Registers[NumRegisters];
};

}
