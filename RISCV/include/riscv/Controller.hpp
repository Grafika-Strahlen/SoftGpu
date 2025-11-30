/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <Common.hpp>
#include "ControlBus.hpp"
#include "InstructionFetch.hpp"

namespace riscv {

class ControllerReceiverSample
{
public:
    void ReceiveController_ControlBus([[maybe_unused]] const u32 index, [[maybe_unused]] const ControlBus& controlBus) noexcept { }
    void ReceiveController_CSRReadData([[maybe_unused]] const u32 index, [[maybe_unused]] const u32 data) noexcept { }
};

// template<typename Receiver>
class Controller final
{
    DEFAULT_DESTRUCT(Controller);
    DELETE_CM(Controller);
public:
    using Receiver = ControllerReceiverSample;
private:
    SENSITIVITY_DECL(p_Reset_n, p_Clock);

    SIGNAL_ENTITIES();
public:
    Controller(
        Receiver* const parent,
        const u32 index
    ) noexcept
        : m_Parent(parent)
        , m_Index(index)
        , p_Reset_n(0)
        , p_Clock(0)
        , p_PMPFault(0)
        , p_ALUCoProcessorDone(0)
        , p_ALUCompareStatus(0)
        , p_IRQDebug(0)
        , p_IRQMachine(0)
        , p_IRQFast(0)
        , p_LSUWait(0)
        , p_LSUError(0)
        , m_Pad0(0)
        , p_InstructionFetchBus{ }
        , p_ALUAddressResult(0)
        , p_RS1(0)
        , p_XCSRReadData(0)
        , p_LSUMemoryAddressRegister(0)
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

    void SetInstructionFetchBus(const InstructionFetchBus& bus) noexcept
    {
        p_InstructionFetchBus = bus;
    }

    void SetPMPFault(const bool pmpFault) noexcept
    {
        p_PMPFault = pmpFault;
    }

    void SetALUCoProcessorDone(const bool aluCoProcessorDone) noexcept
    {
        p_ALUCoProcessorDone = aluCoProcessorDone;
    }

    void SetALUCompareStatus(const u8 compareStatus) noexcept
    {
        p_ALUCompareStatus = compareStatus;
    }

    void SetALUAddressResult(const u32 addressResult) noexcept
    {
        p_ALUAddressResult = addressResult;
    }

    void SetRS1(const u32 rs1) noexcept
    {
        p_RS1 = rs1;
    }

    void SetXCSRReadData(const u32 data) noexcept
    {
        p_XCSRReadData = data;
    }

    void SetIRQDebug(const bool irqDebug) noexcept
    {
        p_IRQDebug = irqDebug;
    }

    void SetIRQMachine(const u8 irqMachine) noexcept
    {
        p_IRQMachine = irqMachine;
    }

    void SetIRQFast(const u16 irqFast) noexcept
    {
        p_IRQFast = irqFast;
    }

    void SetLSUWait(const bool lsuWait) noexcept
    {
        p_LSUWait = lsuWait;
    }

    void SetLSUMemoryAddressRegister(const u32 data) noexcept
    {
        p_LSUMemoryAddressRegister = data;
    }

    void SetLSUError(const u8 error) noexcept
    {
        p_LSUError = error;
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
    u32 p_PMPFault : 1;
    u32 p_ALUCoProcessorDone : 1;
    u32 p_ALUCompareStatus : 2;
    u32 p_IRQDebug : 1;
    u32 p_IRQMachine : 3;
    u32 p_IRQFast : 16;
    u32 p_LSUWait : 1;
    u32 p_LSUError : 4;
    u32 m_Pad0 : 3;

    InstructionFetchBus p_InstructionFetchBus;
    u32 p_ALUAddressResult;
    u32 p_RS1;
    u32 p_XCSRReadData;
    u32 p_LSUMemoryAddressRegister;
};

}
