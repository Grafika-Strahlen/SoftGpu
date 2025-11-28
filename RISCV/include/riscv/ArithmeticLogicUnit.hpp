/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <Common.hpp>
#include "ControlBus.hpp"

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

class ArithmeticLogicUnit final
{
    DEFAULT_DESTRUCT(ArithmeticLogicUnit);
    DELETE_CM(ArithmeticLogicUnit);
public:
    using Receiver = ALUReceiverSample;

    constexpr static inline bool EnableISA_M    = false; // Mul/Div
    constexpr static inline bool EnableISA_Zba  = false; // Shifted-add bit manipulation
    constexpr static inline bool EnableISA_Zbb  = false; // Basic bit manipulation
    constexpr static inline bool EnableISA_Zbkb = false; // Bit manipulation for cryptography
    constexpr static inline bool EnableISA_Zbkc = false; // Carry-less multiplication
    constexpr static inline bool EnableISA_Zbkx = false; // Cryptography crossbar permutation
    constexpr static inline bool EnableISA_Zbs  = false; // Single bit manipulation
    constexpr static inline bool EnableISA_Zfinx  = false; // 32-Bit floating point
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
        , m_Pad0(0)
        , p_ControlBus{ }
        , p_RS1(0)
        , p_RS2(0)
        , p_RS3(0)
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

    void SetRS2(const u32 rs2) noexcept
    {
        p_RS2 = rs2;
    }

    void SetRS3(const u32 rs3) noexcept
    {
        p_RS3 = rs3;
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

    [[maybe_unused]] u32 m_Pad0 : 30;

    ControlBus p_ControlBus;
    u32 p_RS1;
    u32 p_RS2;
    u32 p_RS3;
};

}
