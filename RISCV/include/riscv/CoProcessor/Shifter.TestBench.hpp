#pragma once

#include "Shifter.hpp"
#include <TauUnit.hpp>
#include <chrono>

namespace riscv::coprocessor::test {

struct ShifterReceiver
{
    bool Log = true;

    bool Valid = false;
    u32 Result = 0;

    void ReceiveShifter_Valid(const u32 index, const bool valid) noexcept
    {
        (void) index;
        Valid = valid;
        if(Log)
        {
            ConPrinter::Print("Received Valid: {}\n", Valid);
        }
    }

    void ReceiveShifter_Result(const u32 index, const u32 result) noexcept
    {
        (void) index;
        Result = result;
        if(Log)
        {
            ConPrinter::Print("Received Result: {}\n", Result);
        }
    }
};

template<bool EnableFastShift>
static void ShifterCheckResetResult(bool log = false) noexcept
{
    ShifterReceiver receiver;
    Shifter<ShifterReceiver, EnableFastShift> ip(&receiver);

    receiver.Log = log;

    ip.SetResetN(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid on reset. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 on reset. {}");
}


static void ShifterSerialCheckResetResult(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ShifterCheckResetResult<false>(log);
}

static void ShifterBarrelCheckResetResult(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ShifterCheckResetResult<true>(log);
}

static void ShifterSerialCheck1S0Right(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ShifterReceiver receiver;
    Shifter<ShifterReceiver, false> ip(&receiver);

    receiver.Log = log;

    ip.SetResetN(true);

    ip.SetRS1(1);
    ip.SetShiftAmount(0);
    ControlBus controlBus {};
    controlBus.ALU_BaseCoProcessorTrigger = 1;
    controlBus.IR_Function3 = ControlBus::Function3_ALU_ShiftRight;
    controlBus.IR_Function12 = 0;
    ip.SetControlBus(controlBus);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 1st clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 1st clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 2nd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 2nd clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 3rd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 3rd clock cycle. {}");

    controlBus.ALU_BaseCoProcessorTrigger = 0;
    ip.SetControlBus(controlBus);
    ip.SetRS1(0);
    ip.SetShiftAmount(0);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 4th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 4th clock cycle. {}");
}

static void ShifterBarrelCheck1S0Right(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ShifterReceiver receiver;
    Shifter<ShifterReceiver, true> ip(&receiver);

    receiver.Log = log;

    ip.SetResetN(true);

    ip.SetRS1(1);
    ip.SetShiftAmount(0);
    ControlBus controlBus {};
    controlBus.ALU_BaseCoProcessorTrigger = 1;
    controlBus.IR_Function3 = ControlBus::Function3_ALU_ShiftRight;
    controlBus.IR_Function12 = 0;
    ip.SetControlBus(controlBus);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 1st clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 1st clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 2nd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 2nd clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 3rd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 3rd clock cycle. {}");

    controlBus.ALU_BaseCoProcessorTrigger = 0;
    ip.SetControlBus(controlBus);
    ip.SetRS1(0);
    ip.SetShiftAmount(0);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 4th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 4th clock cycle. {}");
}

static void ShifterSerialCheck2S1Right(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ShifterReceiver receiver;
    Shifter<ShifterReceiver, false> ip(&receiver);

    receiver.Log = log;

    ip.SetResetN(true);

    ip.SetRS1(2);
    ip.SetShiftAmount(1);
    ControlBus controlBus {};
    controlBus.ALU_BaseCoProcessorTrigger = 1;
    controlBus.IR_Function3 = ControlBus::Function3_ALU_ShiftRight;
    controlBus.IR_Function12 = 0;
    ip.SetControlBus(controlBus);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 1st clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 1st clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 2nd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 2nd clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 3rd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 3rd clock cycle. {}");

    controlBus.ALU_BaseCoProcessorTrigger = 0;
    ip.SetControlBus(controlBus);
    ip.SetRS1(0);
    ip.SetShiftAmount(0);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 4th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 4th clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 5th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 5th clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 6th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 6th clock cycle. {}");

}

static void ShifterBarrelCheck2S1Right(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ShifterReceiver receiver;
    Shifter<ShifterReceiver, true> ip(&receiver);

    receiver.Log = log;

    ip.SetResetN(true);

    ip.SetRS1(2);
    ip.SetShiftAmount(1);
    ControlBus controlBus {};
    controlBus.ALU_BaseCoProcessorTrigger = 1;
    controlBus.IR_Function3 = ControlBus::Function3_ALU_ShiftRight;
    controlBus.IR_Function12 = 0;
    ip.SetControlBus(controlBus);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 1st clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 1st clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 2nd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 2nd clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 3rd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 3rd clock cycle. {}");

    controlBus.ALU_BaseCoProcessorTrigger = 0;
    ip.SetControlBus(controlBus);
    ip.SetRS1(0);
    ip.SetShiftAmount(0);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 4th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 4th clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 5th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 5th clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 6th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 6th clock cycle. {}");

}

static void ShifterSerialCheck4S2Right(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ShifterReceiver receiver;
    Shifter<ShifterReceiver, false> ip(&receiver);

    receiver.Log = log;

    ip.SetResetN(true);

    ip.SetRS1(4);
    ip.SetShiftAmount(2);
    ControlBus controlBus {};
    controlBus.ALU_BaseCoProcessorTrigger = 1;
    controlBus.IR_Function3 = ControlBus::Function3_ALU_ShiftRight;
    controlBus.IR_Function12 = 0;
    ip.SetControlBus(controlBus);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 1st clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 1st clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 2nd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 2nd clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 3rd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 3rd clock cycle. {}");

    controlBus.ALU_BaseCoProcessorTrigger = 0;
    ip.SetControlBus(controlBus);
    ip.SetRS1(0);
    ip.SetShiftAmount(0);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 4th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 4th clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 5th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 5th clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 6th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 6th clock cycle. {}");
}

static void ShifterBarrelCheck4S2Right(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ShifterReceiver receiver;
    Shifter<ShifterReceiver, true> ip(&receiver);

    receiver.Log = log;

    ip.SetResetN(true);

    ip.SetRS1(4);
    ip.SetShiftAmount(2);
    ControlBus controlBus {};
    controlBus.ALU_BaseCoProcessorTrigger = 1;
    controlBus.IR_Function3 = ControlBus::Function3_ALU_ShiftRight;
    controlBus.IR_Function12 = 0;
    ip.SetControlBus(controlBus);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 1st clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 1st clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 2nd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 2nd clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 3rd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 3rd clock cycle. {}");

    controlBus.ALU_BaseCoProcessorTrigger = 0;
    ip.SetControlBus(controlBus);
    ip.SetRS1(0);
    ip.SetShiftAmount(0);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 4th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 4th clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 5th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 5th clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 6th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 6th clock cycle. {}");
}

static void ShifterSerialCheck31S3Right(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ShifterReceiver receiver;
    Shifter<ShifterReceiver, false> ip(&receiver);

    receiver.Log = log;

    ip.SetResetN(true);

    ip.SetRS1(31);
    ip.SetShiftAmount(3);
    ControlBus controlBus {};
    controlBus.ALU_BaseCoProcessorTrigger = 1;
    controlBus.IR_Function3 = ControlBus::Function3_ALU_ShiftRight;
    controlBus.IR_Function12 = 0;
    ip.SetControlBus(controlBus);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 1st clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 1st clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 2nd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 2nd clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 3rd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 3rd clock cycle. {}");

    controlBus.ALU_BaseCoProcessorTrigger = 0;
    ip.SetControlBus(controlBus);
    ip.SetRS1(0);
    ip.SetShiftAmount(0);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 4th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 4th clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 5th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 5th clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 6th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 3, "Result should be 3 after 6th clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 7th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 3, "Result should be 3 after 7th clock cycle. {}");
}

static void ShifterBarrelCheck31S3Right(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ShifterReceiver receiver;
    Shifter<ShifterReceiver, true> ip(&receiver);

    receiver.Log = log;

    ip.SetResetN(true);

    ip.SetRS1(31);
    ip.SetShiftAmount(3);
    ControlBus controlBus {};
    controlBus.ALU_BaseCoProcessorTrigger = 1;
    controlBus.IR_Function3 = ControlBus::Function3_ALU_ShiftRight;
    controlBus.IR_Function12 = 0;
    ip.SetControlBus(controlBus);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 1st clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 3, "Result should be 3 after 1st clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 2nd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 3, "Result should be 3 after 2nd clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 3rd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 3, "Result should be 3 after 3rd clock cycle. {}");

    controlBus.ALU_BaseCoProcessorTrigger = 0;
    ip.SetControlBus(controlBus);
    ip.SetRS1(0);
    ip.SetShiftAmount(0);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 4th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 3, "Result should be 3 after 4th clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 5th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 3, "Result should be 3 after 5th clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 6th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 3, "Result should be 3 after 6th clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 7th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 3, "Result should be 3 after 7th clock cycle. {}");
}

static void ShifterSerialCheckMaxS31Right(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ShifterReceiver receiver;
    Shifter<ShifterReceiver, false> ip(&receiver);

    receiver.Log = log;

    ip.SetResetN(true);

    ip.SetRS1(0xFFFFFFFF);
    ip.SetShiftAmount(31);
    ControlBus controlBus {};
    controlBus.ALU_BaseCoProcessorTrigger = 1;
    controlBus.IR_Function3 = ControlBus::Function3_ALU_ShiftRight;
    controlBus.IR_Function12 = 0;
    ip.SetControlBus(controlBus);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 1st clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 1st clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 2nd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 2nd clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 3rd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 3rd clock cycle. {}");

    controlBus.ALU_BaseCoProcessorTrigger = 0;
    ip.SetControlBus(controlBus);
    ip.SetRS1(0);
    ip.SetShiftAmount(0);

    for(uSys i = 0; i < 30; ++i)
    {
        ip.SetClock(true);
        ip.SetClock(false);

        TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after {}} clock cycle. {}", i + 3);
        TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after {} clock cycle. {}", i + 3);
    }

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 34th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 34th clock cycle. {}");
}

static void ShifterBarrelCheckMaxS31Right(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ShifterReceiver receiver;
    Shifter<ShifterReceiver, true> ip(&receiver);

    receiver.Log = log;

    ip.SetResetN(true);

    ip.SetRS1(0xFFFFFFFF);
    ip.SetShiftAmount(31);
    ControlBus controlBus {};
    controlBus.ALU_BaseCoProcessorTrigger = 1;
    controlBus.IR_Function3 = ControlBus::Function3_ALU_ShiftRight;
    controlBus.IR_Function12 = 0;
    ip.SetControlBus(controlBus);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 1st clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 1st clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 2nd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 2nd clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 3rd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 3rd clock cycle. {}");

    controlBus.ALU_BaseCoProcessorTrigger = 0;
    ip.SetControlBus(controlBus);
    ip.SetRS1(0);
    ip.SetShiftAmount(0);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 4th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 1, "Result should be 1 after 4th clock cycle. {}");
}

static void ShifterSerialCheckMaxS32Right(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ShifterReceiver receiver;
    Shifter<ShifterReceiver, false> ip(&receiver);

    receiver.Log = log;

    ip.SetResetN(true);

    ip.SetRS1(0xFFFFFFFF);
    ip.SetShiftAmount(32);
    ControlBus controlBus {};
    controlBus.ALU_BaseCoProcessorTrigger = 1;
    controlBus.IR_Function3 = ControlBus::Function3_ALU_ShiftRight;
    controlBus.IR_Function12 = 0;
    ip.SetControlBus(controlBus);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 1st clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 1st clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 2nd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 2nd clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 3rd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 3rd clock cycle. {}");

    controlBus.ALU_BaseCoProcessorTrigger = 0;
    ip.SetControlBus(controlBus);
    ip.SetRS1(0);
    ip.SetShiftAmount(0);

    for(uSys i = 0; i < 31; ++i)
    {
        ip.SetClock(true);
        ip.SetClock(false);

        TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after {}} clock cycle. {}", i + 3);
        TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after {} clock cycle. {}", i + 3);
    }

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 35th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 35th clock cycle. {}");
}

static void ShifterBarrelCheckMaxS32Right(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ShifterReceiver receiver;
    Shifter<ShifterReceiver, true> ip(&receiver);

    receiver.Log = log;

    ip.SetResetN(true);

    ip.SetRS1(0xFFFFFFFF);
    ip.SetShiftAmount(32);
    ControlBus controlBus {};
    controlBus.ALU_BaseCoProcessorTrigger = 1;
    controlBus.IR_Function3 = ControlBus::Function3_ALU_ShiftRight;
    controlBus.IR_Function12 = 0;
    ip.SetControlBus(controlBus);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 1st clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 1st clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 2nd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 2nd clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 3rd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 3rd clock cycle. {}");

    controlBus.ALU_BaseCoProcessorTrigger = 0;
    ip.SetControlBus(controlBus);
    ip.SetRS1(0);
    ip.SetShiftAmount(0);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 4th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 4th clock cycle. {}");
}

template<bool EnableFastShift>
static void ShifterCheckFuzzRight(bool log = false) noexcept
{
    ShifterReceiver receiver;
    Shifter<ShifterReceiver, false> ip(&receiver);

    receiver.Log = log;

    ip.SetResetN(EnableFastShift);

    ControlBus controlBus {};
    controlBus.ALU_BaseCoProcessorTrigger = 1;
    controlBus.IR_Function3 = ControlBus::Function3_ALU_ShiftRight;
    controlBus.IR_Function12 = 0;
    ip.SetControlBus(controlBus);

    auto time = ::std::chrono::system_clock::now();
    auto timeSinceEpoch = time.time_since_epoch();
    u64 startTimeMillis = static_cast<u64>(::std::chrono::duration_cast<::std::chrono::milliseconds>(timeSinceEpoch).count());

    u64 millisPerBlock = 1;

    for(u32 i = 1; i != 0; ++i)
    {
        ip.SetRS1(i);

        for(u32 shiftAmount = 0; shiftAmount < 32; ++shiftAmount)
        {
            ip.SetShiftAmount(shiftAmount);

            if constexpr(EnableFastShift)
            {
                controlBus.ALU_BaseCoProcessorTrigger = 1;
                ip.SetControlBus(controlBus);
            }

            ip.SetClock(true);
            ip.SetClock(false);

            if constexpr(EnableFastShift)
            {
                controlBus.ALU_BaseCoProcessorTrigger = 0;
                ip.SetControlBus(controlBus);

                for(uSys serialLoopCount = 0; serialLoopCount < 64; ++serialLoopCount)
                {
                    ip.SetClock(true);
                    ip.SetClock(false);
                }
            }

            const u32 checkValue = static_cast<u32>(static_cast<u64>(i) >> shiftAmount);

            TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid for {} >> {}. {}", i, shiftAmount);
            TAU_UNIT_EQ(receiver.Result, checkValue, "Result should be {} for {} >> {}. {}", checkValue, i, shiftAmount);

            if(!receiver.Valid || receiver.Result != checkValue)
            {
                return;
            }
        }

        constexpr u32 Modulus = 0x20'0000;

        if(i % Modulus == 0)
        {
            const u32 blockNumber = i / Modulus;

            // if(blockNumber == 1)
            {
                time = ::std::chrono::system_clock::now();
                timeSinceEpoch = time.time_since_epoch();
                const u64 endTimeMillis = static_cast<u64>(::std::chrono::duration_cast<::std::chrono::milliseconds>(timeSinceEpoch).count());
                millisPerBlock = (endTimeMillis - startTimeMillis) / blockNumber;
                // ConPrinter::PrintLn("Block time: {} ms", millisPerBlock);
            }

            constexpr u32 totalBlocks = static_cast<u32>((static_cast<u64>(1) << 32) / Modulus);

            const u32 remainingBlocks = totalBlocks - blockNumber;

            const double percentComplete = static_cast<double>(blockNumber) / static_cast<double>(totalBlocks) * 100.0;

            DynStringT<char> percentString = Format<char>("{}", percentComplete);
            DynStringT<char> hoursString = Format<char>("{}", (static_cast<double>(remainingBlocks) * static_cast<double>(millisPerBlock)) / (1000.0 * 60.0 * 60.0));

            ConPrinter::PrintLn("Status: {}, {}% Complete, ETA: {} hours", i, DynStringView(percentString, 0, 6), DynStringView(hoursString, 0, 6));
        }
    }
}

static void ShifterSerialCheckFuzzRight(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ShifterCheckFuzzRight<false>(log);
}

static void ShifterBarrelCheckFuzzRight(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ShifterCheckFuzzRight<true>(log);
}



static void ShifterSerialCheck31S3Left(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ShifterReceiver receiver;
    Shifter<ShifterReceiver, false> ip(&receiver);

    receiver.Log = log;

    ip.SetResetN(true);

    ip.SetRS1(31);
    ip.SetShiftAmount(3);
    ControlBus controlBus {};
    controlBus.ALU_BaseCoProcessorTrigger = 1;
    controlBus.IR_Function3 = ControlBus::Function3_ALU_ShiftLeft;
    controlBus.IR_Function12 = 0;
    ip.SetControlBus(controlBus);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 1st clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 1st clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 2nd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 2nd clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 3rd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 3rd clock cycle. {}");

    controlBus.ALU_BaseCoProcessorTrigger = 0;
    ip.SetControlBus(controlBus);
    ip.SetRS1(0);
    ip.SetShiftAmount(0);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 4th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 4th clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 5th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 5th clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 6th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 248, "Result should be 248 after 6th clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 7th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 248, "Result should be 248 after 7th clock cycle. {}");
}

static void ShifterBarrelCheck31S3Left(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ShifterReceiver receiver;
    Shifter<ShifterReceiver, true> ip(&receiver);

    receiver.Log = log;

    ip.SetResetN(true);

    ip.SetRS1(31);
    ip.SetShiftAmount(3);
    ControlBus controlBus {};
    controlBus.ALU_BaseCoProcessorTrigger = 1;
    controlBus.IR_Function3 = ControlBus::Function3_ALU_ShiftLeft;
    controlBus.IR_Function12 = 0;
    ip.SetControlBus(controlBus);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 1st clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 248, "Result should be 248 after 1st clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 2nd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 248, "Result should be 248 after 2nd clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 3rd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 248, "Result should be 248 after 3rd clock cycle. {}");

    controlBus.ALU_BaseCoProcessorTrigger = 0;
    ip.SetControlBus(controlBus);
    ip.SetRS1(0);
    ip.SetShiftAmount(0);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 4th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 248, "Result should be 248 after 4th clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 5th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 248, "Result should be 248 after 5th clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 6th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 248, "Result should be 248 after 6th clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 7th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 248, "Result should be 248 after 7th clock cycle. {}");
}

static void ShifterSerialCheckMaxS31Left(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ShifterReceiver receiver;
    Shifter<ShifterReceiver, false> ip(&receiver);

    receiver.Log = log;

    ip.SetResetN(true);

    ip.SetRS1(0xFFFFFFFF);
    ip.SetShiftAmount(31);
    ControlBus controlBus {};
    controlBus.ALU_BaseCoProcessorTrigger = 1;
    controlBus.IR_Function3 = ControlBus::Function3_ALU_ShiftLeft;
    controlBus.IR_Function12 = 0;
    ip.SetControlBus(controlBus);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 1st clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 1st clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 2nd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 2nd clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 3rd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 3rd clock cycle. {}");

    controlBus.ALU_BaseCoProcessorTrigger = 0;
    ip.SetControlBus(controlBus);
    ip.SetRS1(0);
    ip.SetShiftAmount(0);

    for(uSys i = 0; i < 30; ++i)
    {
        ip.SetClock(true);
        ip.SetClock(false);

        TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after {}} clock cycle. {}", i + 3);
        TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after {} clock cycle. {}", i + 3);
    }

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 34th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0x8000'0000, "Result should be 1 after 34th clock cycle. {}");
}

static void ShifterBarrelCheckMaxS31Left(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ShifterReceiver receiver;
    Shifter<ShifterReceiver, true> ip(&receiver);

    receiver.Log = log;

    ip.SetResetN(true);

    ip.SetRS1(0xFFFFFFFF);
    ip.SetShiftAmount(31);
    ControlBus controlBus {};
    controlBus.ALU_BaseCoProcessorTrigger = 1;
    controlBus.IR_Function3 = ControlBus::Function3_ALU_ShiftLeft;
    controlBus.IR_Function12 = 0;
    ip.SetControlBus(controlBus);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 1st clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0x8000'0000, "Result should be 1 after 1st clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 2nd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0x8000'0000, "Result should be 1 after 2nd clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 3rd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0x8000'0000, "Result should be 1 after 3rd clock cycle. {}");

    controlBus.ALU_BaseCoProcessorTrigger = 0;
    ip.SetControlBus(controlBus);
    ip.SetRS1(0);
    ip.SetShiftAmount(0);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 4th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0x8000'0000, "Result should be 1 after 4th clock cycle. {}");
}

static void ShifterSerialCheckMaxS32Left(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ShifterReceiver receiver;
    Shifter<ShifterReceiver, false> ip(&receiver);

    receiver.Log = log;

    ip.SetResetN(true);

    ip.SetRS1(0xFFFFFFFF);
    ip.SetShiftAmount(32);
    ControlBus controlBus {};
    controlBus.ALU_BaseCoProcessorTrigger = 1;
    controlBus.IR_Function3 = ControlBus::Function3_ALU_ShiftLeft;
    controlBus.IR_Function12 = 0;
    ip.SetControlBus(controlBus);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 1st clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 1st clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 2nd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 2nd clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after 3rd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 3rd clock cycle. {}");

    controlBus.ALU_BaseCoProcessorTrigger = 0;
    ip.SetControlBus(controlBus);
    ip.SetRS1(0);
    ip.SetShiftAmount(0);

    for(uSys i = 0; i < 31; ++i)
    {
        ip.SetClock(true);
        ip.SetClock(false);

        TAU_UNIT_EQ(receiver.Valid, false, "Result should not be valid after {}} clock cycle. {}", i + 3);
        TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after {} clock cycle. {}", i + 3);
    }

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 35th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 35th clock cycle. {}");
}

static void ShifterBarrelCheckMaxS32Left(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ShifterReceiver receiver;
    Shifter<ShifterReceiver, true> ip(&receiver);

    receiver.Log = log;

    ip.SetResetN(true);

    ip.SetRS1(0xFFFFFFFF);
    ip.SetShiftAmount(32);
    ControlBus controlBus {};
    controlBus.ALU_BaseCoProcessorTrigger = 1;
    controlBus.IR_Function3 = ControlBus::Function3_ALU_ShiftLeft;
    controlBus.IR_Function12 = 0;
    ip.SetControlBus(controlBus);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 1st clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 1st clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 2nd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 2nd clock cycle. {}");

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 3rd clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 3rd clock cycle. {}");

    controlBus.ALU_BaseCoProcessorTrigger = 0;
    ip.SetControlBus(controlBus);
    ip.SetRS1(0);
    ip.SetShiftAmount(0);

    ip.SetClock(true);
    ip.SetClock(false);

    TAU_UNIT_EQ(receiver.Valid, true, "Result should be valid after 4th clock cycle. {}");
    TAU_UNIT_EQ(receiver.Result, 0, "Result should be 0 after 4th clock cycle. {}");
}

}
