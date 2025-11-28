/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once


#include "ArithmeticLogicUnit.hpp"
#include <TauUnit.hpp>
#include <chrono>

namespace riscv::test {

class ALUReceiver
{
public:
    u8 ComparatorStatus = 0;
    u32 Result = 0;
    u32 Address = 0;
    u32 CSR = 0;
    bool Done = false;

    void ReceiveArithmeticLogicUnit_ComparatorStatus([[maybe_unused]] const u32 index, [[maybe_unused]] const u8 status) noexcept
    {
        ComparatorStatus = status;
    }

    void ReceiveArithmeticLogicUnit_Result([[maybe_unused]] const u32 index, [[maybe_unused]] const u32 result) noexcept
    {
        Result = result;
        ConPrinter::PrintLn("Received result: {}", result);
    }

    void ReceiveArithmeticLogicUnit_Address([[maybe_unused]] const u32 index, [[maybe_unused]] const u32 address) noexcept
    {
        Address = address;
    }

    void ReceiveArithmeticLogicUnit_CSR([[maybe_unused]] const u32 index, [[maybe_unused]] const u32 csr) noexcept
    {
        CSR = csr;
    }

    void ReceiveArithmeticLogicUnit_Done([[maybe_unused]] const u32 index, [[maybe_unused]] const bool done) noexcept
    {
        Done = done;
    }
};

[[maybe_unused]] static void ALUTestShifter() noexcept
{
    ALUReceiver receiver;
    ArithmeticLogicUnit<ALUReceiver> ip(&receiver);

    ip.SetResetN(true);


    ip.SetRS1(32);
    ip.SetRS2(1);
    ControlBus controlBus {};
    controlBus.ALU_OperationSelect = ControlBus::ALU_Operation_CoProcessor;
    controlBus.ALU_OperandSelectA = 0;
    controlBus.ALU_OperandSelectB = 0;
    controlBus.ALU_BaseCoProcessorTrigger = 1;
    controlBus.IR_Function3 = ControlBus::Function3_ALU_ShiftRight;
    controlBus.IR_Function12 = 0;
    ip.SetControlBus(controlBus);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Done, true, "Result should be valid after 1st clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 16, "Result should be 64 after 1st clock cycle. {}");
}

}
