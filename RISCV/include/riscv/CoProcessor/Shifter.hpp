#pragma once

#include <Common.hpp>
#include "riscv/ControlBus.hpp"
#include <array>

namespace riscv::coprocessor {


class ShifterReceiverSample
{
public:
    void ReceiveShifter_Valid(const u32 index, const bool valid) noexcept { }
    void ReceiveShifter_Result(const u32 index, const u32 result) noexcept { }
};

template<typename Receiver = ShifterReceiverSample, bool EnableFastShift = true>
class Shifter final
{
    DEFAULT_DESTRUCT(Shifter);
    DELETE_CM(Shifter);
private:
    SENSITIVITY_DECL(p_Reset_n, p_Clock);

    SIGNAL_ENTITIES();
public:
    Shifter(Receiver* const parent, const u32 index = 0) noexcept
        : m_Parent(parent)
        , m_Index(index)
        , p_Reset_n(0)
        , p_Clock(0)
        , p_ShiftAmount(0)
        , m_Pad0(0)
        , p_ControlBus { }
        , p_RS1(0)
        , m_Busy(0)
        , m_Done(0)
        , m_Count(0)
        , m_Pad1(0)
        , m_ShiftReg(0)
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

    void SetRS1(const u32 rs1) noexcept
    {
        p_RS1 = rs1;
    }

    void SetShiftAmount(const u32 shiftAmount) noexcept
    {
        p_ShiftAmount = shiftAmount;
    }
private:
    // Wires

    [[nodiscard]] bool CommandValid() const noexcept
    {
        if(!BIT_TO_BOOL(p_ControlBus.ALU_BaseCoProcessorTrigger))
        {
            return false;
        }

        if(p_ControlBus.IR_Function3 == ControlBus::Function3_ALU_ShiftLeft && (p_ControlBus.IR_Function12 >> 5) == 0)
        {
            return true;
        }

        if(p_ControlBus.IR_Function3 == ControlBus::Function3_ALU_ShiftRight)
        {
            // Logical or Arithmetical
            if((p_ControlBus.IR_Function12 >> 5) == 0 || (p_ControlBus.IR_Function12 >> 5) == 0b0100000)
            {
                return true;
            }
        }

        return false;
    }

    [[nodiscard]] bool Run() const noexcept
    {
        return m_Count != 0;
    }

    [[nodiscard]] bool Done() const noexcept
    {
        return (m_Count & 0xFFFFFFFE) == 0;
    }

    [[nodiscard]] bool BarrelSign() const noexcept
    {
        return BIT_TO_BOOL((p_RS1 >> 31) & 0x1) && BIT_TO_BOOL((p_ControlBus.IR_Function12 >> 10) & 0x1) && CommandValid();
    }

    [[nodiscard]] u32 BarrelLevel0() const noexcept
    {
        if(!CommandValid())
        {
            return 0;
        }
        else
        {
            if(!BIT_TO_BOOL((p_ControlBus.IR_Function3 >> 2) & 0x1))
            {
                return Reverse(p_RS1);
            }
            else
            {
                return p_RS1;
            }
        }
    }

    template<i32 BarrelLevel>
    [[nodiscard]] u32 BarrelLevels() const noexcept
    {
        if constexpr(BarrelLevel < 0)
        {
            return BarrelLevel0();
        }
        else
        {
            u32 ret = 0;

            // These have to be u64s, otherwise a bitshift of 32 gets optimized away, probably because it's UB.
            if(BIT_TO_BOOL((p_ShiftAmount >> BarrelLevel) & 0x1))
            {
                u64 highBits = BarrelSign() ? 0xFFFFFFFF : 0;
                highBits >>= (1 << (BarrelLevel + 1));
                highBits <<= (1 << (BarrelLevel + 1));
                ret |= highBits;
            }
            else
            {
                u64 highBits = BarrelLevels<BarrelLevel - 1>();
                highBits >>= (1 << (BarrelLevel + 1));
                highBits <<= (1 << (BarrelLevel + 1));
                ret |= highBits;
            }

            if(BIT_TO_BOOL((p_ShiftAmount >> BarrelLevel) & 0x1))
            {
                u64 lowBits = BarrelLevels<BarrelLevel - 1>();
                lowBits >>= (1 << (BarrelLevel));
                ret |= lowBits;
            }
            else
            {
                u64 lowBits = BarrelLevels<BarrelLevel - 1>();
                lowBits <<= (1 << (BarrelLevel));
                lowBits >>= (1 << (BarrelLevel));
                ret |= lowBits;
            }

            return ret;
        }
    }
private:
    static u32 Reverse(u32 x) noexcept
    {
        x = ((x >> 1) & 0x55555555u) | ((x & 0x55555555u) << 1);
        x = ((x >> 2) & 0x33333333u) | ((x & 0x33333333u) << 2);
        x = ((x >> 4) & 0x0f0f0f0fu) | ((x & 0x0f0f0f0fu) << 4);
        x = ((x >> 8) & 0x00ff00ffu) | ((x & 0x00ff00ffu) << 8);
        x = ((x >> 16) & 0xffffu) | ((x & 0xffffu) << 16);
        return x;
    }
private:
    PROCESSES_DECL()
    {
        if constexpr(EnableFastShift)
        {
            PROCESS_ENTER(BarrelShifter, p_Reset_n, p_Clock);
        }
        else
        {
            PROCESS_ENTER(SerialShifter, p_Reset_n, p_Clock);
        }
    }

    PROCESS_DECL(SerialShifter)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_Busy = 0;
            m_Done = 0;
            m_Count = 0;
            m_ShiftReg = 0;

            m_Parent->ReceiveShifter_Valid(m_Index, false);
            m_Parent->ReceiveShifter_Result(m_Index, 0);
        }
        else if(RISING_EDGE(p_Clock))
        {
            m_Done = m_Busy & Done();

            if(CommandValid())
            {
                m_Busy = 1;
            }
            else if(BIT_TO_BOOL(Done()) || BIT_TO_BOOL(p_ControlBus.CPU_Trap)) // Abort on trap
            {
                m_Busy = 0;
            }

            if(CommandValid())
            {
                m_Count = p_ShiftAmount;
                m_ShiftReg = p_RS1;
            }
            else if(BIT_TO_BOOL(Run()))
            {
                --m_Count;
                if(!BIT_TO_BOOL((p_ControlBus.IR_Function3 >> 2) & 0x1)) // Shift Left
                {
                    m_ShiftReg <<= 1;
                }
                else // Shift right
                {
                    // Handle Logical and Arithmetical right shift.
                    m_ShiftReg = ((((m_ShiftReg >> 31) & 0x1) & ((p_ControlBus.IR_Function12 >> 10) & 0x1)) << 31) | ((m_ShiftReg >> 1) & 0x7FFFFFFF);  // NOLINT(misc-redundant-expression)
                }
            }

            const bool valid = !BIT_TO_BOOL(m_Busy) && Done();

            m_Parent->ReceiveShifter_Valid(m_Index, valid);

            if(valid)
            {
                m_Parent->ReceiveShifter_Result(m_Index, m_ShiftReg);
            }
            else
            {
                m_Parent->ReceiveShifter_Result(m_Index, 0);
            }
        }
    }

    PROCESS_DECL(BarrelShifter)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_ShiftReg = 0;
            m_Done = 0;

            m_Parent->ReceiveShifter_Valid(m_Index, false);
            m_Parent->ReceiveShifter_Result(m_Index, 0);
        }
        else if(RISING_EDGE(p_Clock))
        {
            if(CommandValid())
            {
                m_Done = 1;
                m_ShiftReg = BarrelLevels<5>();

                m_Parent->ReceiveShifter_Valid(m_Index, BIT_TO_BOOL(m_Done));

                if(!BIT_TO_BOOL((p_ControlBus.IR_Function3 >> 2) & 0x1))
                {
                    m_Parent->ReceiveShifter_Result(m_Index, Reverse(m_ShiftReg));
                }
                else
                {
                    m_Parent->ReceiveShifter_Result(m_Index, m_ShiftReg);
                }
            }
        }
    }
private:
    Receiver* m_Parent;
    u32 m_Index;

    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    u32 p_ShiftAmount : 6;
    [[maybe_unused]] u32 m_Pad0 : 24;
    ControlBus p_ControlBus;
    u32 p_RS1;

    u32 m_Busy : 1;
    u32 m_Done : 1;
    u32 m_Count : 6;
    [[maybe_unused]] u32 m_Pad1 : 24;

    u32 m_ShiftReg;
};


}
