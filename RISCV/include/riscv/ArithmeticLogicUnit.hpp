/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <Common.hpp>
#include "ControlBus.hpp"
#include "CoProcessor/Shifter.hpp"

namespace riscv {

class ALUReceiverSample
{
public:
    void ReceiveArithmeticLogicUnit_ComparatorStatus([[maybe_unused]] const u32 index, [[maybe_unused]] const u8 status) noexcept { }
    void ReceiveArithmeticLogicUnit_Result([[maybe_unused]] const u32 index, [[maybe_unused]] const u32 result) noexcept { }
    void ReceiveArithmeticLogicUnit_Address([[maybe_unused]] const u32 index, [[maybe_unused]] const u32 address) noexcept { }
    void ReceiveArithmeticLogicUnit_CSR([[maybe_unused]] const u32 index, [[maybe_unused]] const u32 csr) noexcept { }
    void ReceiveArithmeticLogicUnit_Done([[maybe_unused]] const u32 index, [[maybe_unused]] const bool done) noexcept { }
};

template<typename Receiver = ALUReceiverSample>
class ArithmeticLogicUnit final
{
    DEFAULT_DESTRUCT(ArithmeticLogicUnit);
    DELETE_CM(ArithmeticLogicUnit);
public:
    // using Receiver = ALUReceiverSample;

    constexpr static inline bool EnableISA_M      = false; // Mul/Div
    constexpr static inline bool EnableISA_Zba    = false; // Shifted-add bit manipulation
    constexpr static inline bool EnableISA_Zbb    = false; // Basic bit manipulation
    constexpr static inline bool EnableISA_Zbkb   = false; // Bit manipulation for cryptography
    constexpr static inline bool EnableISA_Zbkc   = false; // Carry-less multiplication
    constexpr static inline bool EnableISA_Zbkx   = false; // Cryptography crossbar permutation
    constexpr static inline bool EnableISA_Zbs    = false; // Single bit manipulation
    constexpr static inline bool EnableISA_Zfinx  = false; // 32-Bit floating point
    constexpr static inline bool EnableISA_Zihpm  = false; // Hardware performance monitors
    constexpr static inline bool EnableISA_Zkne   = false; // AES encryption
    constexpr static inline bool EnableISA_Zknd   = false; // AES decryption
    constexpr static inline bool EnableISA_Zknh   = false; // Hashing
    constexpr static inline bool EnableISA_Zmmul  = false; // Multiply only M sub-extension
    constexpr static inline bool EnableISA_Zxcfu  = false; // Custom functions unit

    constexpr static inline bool EnableFastShift  = true; // Use barrel shifter
private:
    SENSITIVITY_DECL(p_Reset_n, p_Clock);

    SIGNAL_ENTITIES();
public:
    ArithmeticLogicUnit(
        Receiver* const parent,
        const u32 index = 0
    ) noexcept
        : m_Parent(parent)
        , m_Index(index)
        , p_Reset_n(0)
        , p_Clock(0)
        , m_Valid(0)
        , m_Pad0(0)
        , p_ControlBus{ }
        , p_RS1(0)
        , p_RS2(0)
        , p_RS3(0)
        , m_Results{ }
        , m_Shifter(this, 0)
    { }

    void SetResetN(const bool reset_n) noexcept
    {
        p_Reset_n = BOOL_TO_BIT(reset_n);

        TRIGGER_SENSITIVITY(p_Reset_n);

        m_Shifter.SetResetN(reset_n);
    }

    void SetClock(const bool clock) noexcept
    {
        p_Clock = BOOL_TO_BIT(clock);

        TRIGGER_SENSITIVITY(p_Clock);

        m_Shifter.SetClock(clock);
    }

    void SetControlBus(const ControlBus& controlBus) noexcept
    {
        const bool changedSign = controlBus.ALU_Unsigned != p_ControlBus.ALU_Unsigned;
        const bool changedSelectB = controlBus.ALU_OperandSelectB != p_ControlBus.ALU_OperandSelectB;
        const bool changedImmediate = controlBus.ALU_Immediate != p_ControlBus.ALU_Immediate;

        p_ControlBus = controlBus;

        if(changedSign)
        {
            UpdateCompare();
        }

        // This can be triggered by updating sign, OperandSelectA, OperandSelectB, PC_Current, Immediate, and Subtract,
        // so we'll just always trigger an update.
        UpdateAddress();

        m_Shifter.SetControlBus(controlBus);

        if(changedSelectB || changedImmediate)
        {
            UpdateShiftAmount();
        }

        UpdateResult();
    }

    void SetRS1(const u32 rs1) noexcept
    {
        p_RS1 = rs1;

        UpdateCompare();

        m_Shifter.SetRS1(rs1);
        UpdateResult();
    }

    void SetRS2(const u32 rs2) noexcept
    {
        p_RS2 = rs2;

        UpdateCompare();
        UpdateShiftAmount();
        UpdateResult();
    }

    void SetRS3(const u32 rs3) noexcept
    {
        p_RS3 = rs3;
    }
public:
    void ReceiveShifter_Valid(const u32 index, const bool valid) noexcept
    {
        (void) index;
        m_Valid = SetBit(m_Valid, 0, valid);
        UpdateDone();
    }

    void ReceiveShifter_Result(const u32 index, const u32 result) noexcept
    {
        (void) index;
        m_Results[0] = result;
        UpdateResult();
    }
private:
    // Wires

    [[nodiscard]] bool CompareEqual() const noexcept
    {
        return p_RS1 == p_RS2;
    }

    [[nodiscard]] bool CompareLessThan() const noexcept
    {
        if(p_ControlBus.ALU_Unsigned)
        {
            return p_RS1 < p_RS2;
        }
        else
        {
            return static_cast<i32>(p_RS1) < static_cast<i32>(p_RS2);
        }
    }

    void UpdateCompare() const noexcept
    {
        const u8 value = BOOL_TO_BIT(CompareEqual()) | (BOOL_TO_BIT(CompareLessThan()) << 1);
        m_Parent->ReceiveArithmeticLogicUnit_ComparatorStatus(m_Index, value);
    }

    [[nodiscard]] u32 OperandA() const noexcept
    {
        if(BOOL_TO_BIT(p_ControlBus.ALU_OperandSelectA))
        {
            return p_ControlBus.PC_Current;
        }
        else
        {
            return p_RS1;
        }
    }

    [[nodiscard]] u32 OperandB() const noexcept
    {
        if(BOOL_TO_BIT(p_ControlBus.ALU_OperandSelectB))
        {
            return p_ControlBus.ALU_Immediate;
        }
        else
        {
            return p_RS2;
        }
    }

    [[nodiscard]] u32 AddSubtractResult() const noexcept
    {
        if(BOOL_TO_BIT(p_ControlBus.ALU_Subtract))
        {
            if(p_ControlBus.ALU_Unsigned)
            {
                return OperandA() - OperandB();
            }
            else
            {
                return static_cast<i32>(OperandA()) - static_cast<i32>(OperandB());
            }
        }
        else
        {
            if(p_ControlBus.ALU_Unsigned)
            {
                return OperandA() + OperandB();
            }
            else
            {
                return static_cast<i32>(OperandA()) + static_cast<i32>(OperandB());
            }
        }
    }

    [[nodiscard]] bool CarryBorrow() const noexcept
    {
        const u32 b = BIT_TO_BOOL(p_ControlBus.ALU_Subtract) ? ~OperandB() : OperandB();
        const u32 add0 = OperandA() ^ b;
        const u32 carry0 = OperandA() & b;
        const u32 carryIn0 = (carry0 << 1) | p_ControlBus.ALU_Subtract;
        const u32 carry1 = add0 & carryIn0;
        const u32 carryOut = carry0 | carry1;
        return BIT_TO_BOOL(carryOut >> 31);
    }

    void UpdateAddress() const noexcept
    {
        m_Parent->ReceiveArithmeticLogicUnit_Address(m_Index, AddSubtractResult());
    }

    [[nodiscard]] bool Done() const noexcept
    {
        return m_Valid != 0;
    }

    void UpdateDone() const noexcept
    {
        m_Parent->ReceiveArithmeticLogicUnit_Done(m_Index, Done());
    }

    [[nodiscard]] u32 Result() const noexcept
    {
        return m_Results[0] | m_Results[1] | m_Results[2] | m_Results[3] | m_Results[4] | m_Results[5] | m_Results[6];
    }

    void UpdateShiftAmount()
    {
        m_Shifter.SetShiftAmount(OperandB() & 0x2F);
    }

    [[nodiscard]] u32 ComputeResult() const noexcept
    {
        switch(p_ControlBus.ALU_OperationSelect)
        {
            case ControlBus::ALU_Operation_Zero:        return 0;
            case ControlBus::ALU_Operation_Add:         return AddSubtractResult();
            case ControlBus::ALU_Operation_CoProcessor: return Result();
            case ControlBus::ALU_Operation_LessThan:    return BOOL_TO_BIT(CarryBorrow());
            case ControlBus::ALU_Operation_MoveB:       return OperandB();
            case ControlBus::ALU_Operation_XOR:         return OperandB() ^ p_RS1;
            case ControlBus::ALU_Operation_OR:          return OperandB() | p_RS1;
            case ControlBus::ALU_Operation_AND:         return OperandB() & p_RS1;
            default:                                    return 0;
        }
    }

    void UpdateResult() const noexcept
    {
        m_Parent->ReceiveArithmeticLogicUnit_Result(m_Index, ComputeResult());
    }
private:
    PROCESSES_DECL()
    {
    }
private:
    Receiver* m_Parent;
    u32 m_Index;

    u32 p_Reset_n : 1;
    u32 p_Clock : 1;

    u32 m_Valid : 7;
    [[maybe_unused]] u32 m_Pad0 : 23;

    ControlBus p_ControlBus;
    u32 p_RS1;
    u32 p_RS2;
    u32 p_RS3;

    u32 m_Results[7];

    coprocessor::Shifter<ArithmeticLogicUnit, EnableFastShift> m_Shifter;
};

}
