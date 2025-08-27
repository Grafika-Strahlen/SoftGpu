/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include "ClockGate.hpp"
#include <TauUnit.hpp>

namespace riscv::test {

struct ClockGateReceiver
{
    bool Log = true;

    bool Clock = false;
    int ReceivedCount = 0;

    void ReceiveClockGate_Clock(const u32 index, const bool clock) noexcept
    {
        (void) index;
        Clock = clock;
        ++ReceivedCount;
        if(Log)
        {
            ConPrinter::Print("Received Clock: {}\n", Clock);
        }
    }

    void AffirmReceive(const int count = 1) noexcept
    {
        ReceivedCount -= count;
    }

    void ResetReceive() noexcept
    {
        ReceivedCount = 0;
    }
};

static void CheckDefaultClock(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ClockGateReceiver receiver;
    ClockGate clockGate(&receiver);

    TAU_UNIT_EQ(receiver.Clock, false, "Clock was active at initial start.");
    TAU_UNIT_EQ(receiver.Clock, false, "Clock state changed without input.");
    TAU_UNIT_EQ(receiver.Clock, false, "Clock state changed without input.");
    TAU_UNIT_EQ(receiver.Clock, false, "Clock state changed without input.");
}

static void CheckLowResetClock(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ClockGateReceiver receiver;
    ClockGate clockGate(&receiver);

    receiver.Log = log;

    clockGate.SetClock(false);

    TAU_UNIT_EQ(receiver.Clock, false, "Clock was active after setting clock low.");

    clockGate.SetClock(true);

    TAU_UNIT_EQ(receiver.Clock, true, "Clock was not active after setting clock high while Reset_n was low.");

    clockGate.SetResetN(false);

    clockGate.SetClock(false);

    TAU_UNIT_EQ(receiver.Clock, false, "Clock was active after setting clock low.");

    clockGate.SetClock(true);

    TAU_UNIT_EQ(receiver.Clock, true, "Clock was active after setting clock high after setting Reset_n low.");
}

static void CheckActiveClock(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ClockGateReceiver receiver;
    ClockGate clockGate(&receiver);

    receiver.Log = log;

    clockGate.SetResetN(true);

    clockGate.SetClock(false);

    TAU_UNIT_EQ(receiver.Clock, false, "Clock was active after setting clock low.");

    clockGate.SetClock(true);

    TAU_UNIT_EQ(receiver.Clock, true, "Clock was not active after setting clock high.");

    clockGate.SetClock(false);

    TAU_UNIT_EQ(receiver.Clock, false, "Clock was active after setting clock low.");

    clockGate.SetClock(true);

    TAU_UNIT_EQ(receiver.Clock, true, "Clock was not active after setting clock high.");
}

static void CheckActiveClockReset(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ClockGateReceiver receiver;
    ClockGate clockGate(&receiver);

    receiver.Log = log;

    clockGate.SetResetN(true);

    clockGate.SetClock(false);

    TAU_UNIT_EQ(receiver.Clock, false, "Clock was active after setting clock low.");

    clockGate.SetClock(true);

    TAU_UNIT_EQ(receiver.Clock, true, "Clock was not active after setting clock high.");

    clockGate.SetClock(false);

    TAU_UNIT_EQ(receiver.Clock, false, "Clock was active after setting clock low.");

    clockGate.SetClock(true);

    TAU_UNIT_EQ(receiver.Clock, true, "Clock was not active after setting clock high.");

    clockGate.SetResetN(false);

    clockGate.SetClock(false);

    TAU_UNIT_EQ(receiver.Clock, false, "Clock was active after setting clock low.");

    clockGate.SetClock(true);

    TAU_UNIT_EQ(receiver.Clock, true, "Clock was not active after setting clock high after setting Reset_n low.");
}

static void CheckActiveClockHalt(bool log = false) noexcept
{
    TAU_UNIT_TEST();

    ClockGateReceiver receiver;
    ClockGate clockGate(&receiver);

    receiver.Log = log;

    clockGate.SetResetN(true);

    clockGate.SetClock(false);

    TAU_UNIT_EQ(receiver.Clock, false, "Clock was active after setting clock low.");

    clockGate.SetClock(true);

    TAU_UNIT_EQ(receiver.Clock, true, "Clock was not active after setting clock high during halt.");

    clockGate.SetHalt(true);

    TAU_UNIT_EQ(receiver.Clock, true, "Clock was not active after setting clock high before falling edge halt trigger.");

    clockGate.SetClock(false);

    TAU_UNIT_EQ(receiver.Clock, false, "Clock was active after setting clock low.");

    clockGate.SetClock(true);

    TAU_UNIT_EQ(receiver.Clock, false, "Clock was active after setting clock high during halt.");

    clockGate.SetResetN(false);

    clockGate.SetClock(false);

    TAU_UNIT_EQ(receiver.Clock, false, "Clock was active after setting clock low.");

    clockGate.SetClock(true);

    TAU_UNIT_EQ(receiver.Clock, true, "Clock was not active after setting clock high during halt & reset.");
}

static void ClockGateTestBench(bool log = false) noexcept
{
    CheckDefaultClock(log);
    CheckLowResetClock(log);
    CheckActiveClock(log);
    CheckActiveClockReset(log);
    CheckActiveClockHalt(log);
}

}
