#include <riscv/ClockGate.hpp>
#include <TauUnit.hpp>

static void CheckDefaultClock() noexcept
{
    TAU_UNIT_TEST();

    riscv::ClockGate clockGate;

    TAU_UNIT_EQ(clockGate.GetClock(), false, "Clock was active at initial start.");
    TAU_UNIT_EQ(clockGate.GetClock(), false, "Clock state changed without input.");
    TAU_UNIT_EQ(clockGate.GetClock(), false, "Clock state changed without input.");
    TAU_UNIT_EQ(clockGate.GetClock(), false, "Clock state changed without input.");
}

static void CheckLowResetClock() noexcept
{
    TAU_UNIT_TEST();

    riscv::ClockGate clockGate;

    clockGate.SetClock(false);

    TAU_UNIT_EQ(clockGate.GetClock(), false, "Clock was active after setting clock low.");

    clockGate.SetClock(true);

    TAU_UNIT_EQ(clockGate.GetClock(), true, "Clock was not active after setting clock high while Reset_n was low.");

    clockGate.SetResetN(false);

    clockGate.SetClock(false);

    TAU_UNIT_EQ(clockGate.GetClock(), false, "Clock was active after setting clock low.");

    clockGate.SetClock(true);

    TAU_UNIT_EQ(clockGate.GetClock(), true, "Clock was active after setting clock high after setting Reset_n low.");
}

static void CheckActiveClock() noexcept
{
    TAU_UNIT_TEST();

    riscv::ClockGate clockGate;

    clockGate.SetResetN(true);

    clockGate.SetClock(false);

    TAU_UNIT_EQ(clockGate.GetClock(), false, "Clock was active after setting clock low.");

    clockGate.SetClock(true);

    TAU_UNIT_EQ(clockGate.GetClock(), true, "Clock was not active after setting clock high.");

    clockGate.SetClock(false);

    TAU_UNIT_EQ(clockGate.GetClock(), false, "Clock was active after setting clock low.");

    clockGate.SetClock(true);

    TAU_UNIT_EQ(clockGate.GetClock(), true, "Clock was not active after setting clock high.");
}

static void CheckActiveClockReset() noexcept
{
    TAU_UNIT_TEST();

    riscv::ClockGate clockGate;

    clockGate.SetResetN(true);

    clockGate.SetClock(false);

    TAU_UNIT_EQ(clockGate.GetClock(), false, "Clock was active after setting clock low.");

    clockGate.SetClock(true);

    TAU_UNIT_EQ(clockGate.GetClock(), true, "Clock was not active after setting clock high.");

    clockGate.SetClock(false);

    TAU_UNIT_EQ(clockGate.GetClock(), false, "Clock was active after setting clock low.");

    clockGate.SetClock(true);

    TAU_UNIT_EQ(clockGate.GetClock(), true, "Clock was not active after setting clock high.");

    clockGate.SetResetN(false);

    clockGate.SetClock(false);

    TAU_UNIT_EQ(clockGate.GetClock(), false, "Clock was active after setting clock low.");

    clockGate.SetClock(true);

    TAU_UNIT_EQ(clockGate.GetClock(), true, "Clock was not active after setting clock high after setting Reset_n low.");
}

static void CheckActiveClockHalt() noexcept
{
    TAU_UNIT_TEST();

    riscv::ClockGate clockGate;

    clockGate.SetResetN(true);

    clockGate.SetClock(false);

    TAU_UNIT_EQ(clockGate.GetClock(), false, "Clock was active after setting clock low.");

    clockGate.SetClock(true);

    TAU_UNIT_EQ(clockGate.GetClock(), true, "Clock was not active after setting clock high during halt.");

    clockGate.SetHalt(true);

    TAU_UNIT_EQ(clockGate.GetClock(), true, "Clock was not active after setting clock high before falling edge halt trigger.");

    clockGate.SetClock(false);

    TAU_UNIT_EQ(clockGate.GetClock(), false, "Clock was active after setting clock low.");

    clockGate.SetClock(true);

    TAU_UNIT_EQ(clockGate.GetClock(), false, "Clock was active after setting clock high during halt.");

    clockGate.SetResetN(false);

    clockGate.SetClock(false);

    TAU_UNIT_EQ(clockGate.GetClock(), false, "Clock was active after setting clock low.");

    clockGate.SetClock(true);

    TAU_UNIT_EQ(clockGate.GetClock(), true, "Clock was not active after setting clock high during halt & reset.");
}

void ClockGateTestBench() noexcept
{
    CheckDefaultClock();
    CheckLowResetClock();
    CheckActiveClock();
    CheckActiveClockReset();
    CheckActiveClockHalt();
}
