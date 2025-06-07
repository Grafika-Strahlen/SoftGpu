#pragma once

#include "Memory.hpp"
#include "BitSetToData.hpp"
#include <TauUnit.hpp>

namespace riscv::fifo::test {

static void MemorySimpleSet()
{
    TAU_UNIT_TEST();

    using MemoryTest = Memory<16, 5>;

    MemoryTest memory;

    // Trigger an initial clock cycle to force a clear.
    memory.SetWriteClock(true);
    memory.SetWriteClock(false);

    u16 data = CopyFromBitSet<u16>(memory.GetReadData());

    TAU_UNIT_EQ(data, 0, "Expected data to start at 0. {}");

    data = 0xA1B2;

    memory.SetWriteData(CopyToBitSet(data));

    data = CopyFromBitSet<u16>(memory.GetReadData());

    TAU_UNIT_EQ(data, 0, "Data should not have changed without a clock cycle. {}");

    memory.SetWriteAddress(0);
    memory.SetReadAddress(0);
    memory.SetWriteClockEnable(true);

    data = CopyFromBitSet<u16>(memory.GetReadData());

    TAU_UNIT_EQ(data, 0, "Data should not have changed without a clock cycle. {}");

    // Trigger a memory cycle.
    memory.SetWriteClock(true);

    data = CopyFromBitSet<u16>(memory.GetReadData());

    TAU_UNIT_EQ(data, 0xA1B2, "Expected data at address 0 to be returned. {}");

    memory.SetWriteClock(false);

    data = CopyFromBitSet<u16>(memory.GetReadData());

    TAU_UNIT_EQ(data, 0xA1B2, "Expected data at address 0 to be returned. {}");
}

static void MemoryFullSet()
{
    TAU_UNIT_TEST();

    using MemoryTest = Memory<16, 5>;

    MemoryTest memory;

    // Trigger an initial clock cycle to force a clear.
    memory.SetWriteClock(true);
    memory.SetWriteClock(false);

    memory.SetWriteFull(true);

    u16 data = CopyFromBitSet<u16>(memory.GetReadData());

    TAU_UNIT_EQ(data, 0, "Expected data to start at 0. {}");

    data = 0xA1B2;

    memory.SetWriteData(CopyToBitSet(data));

    data = CopyFromBitSet<u16>(memory.GetReadData());

    TAU_UNIT_EQ(data, 0, "Data should not have changed without a clock cycle. {}");

    memory.SetWriteAddress(0);
    memory.SetReadAddress(0);
    memory.SetWriteClockEnable(true);

    data = CopyFromBitSet<u16>(memory.GetReadData());

    TAU_UNIT_EQ(data, 0, "Data should not have changed without a clock cycle. {}");

    // Trigger a memory cycle.
    memory.SetWriteClock(true);

    data = CopyFromBitSet<u16>(memory.GetReadData());

    TAU_UNIT_EQ(data, 0, "Expected data at address 0 to be unchanged when full. {}");

    memory.SetWriteClock(false);

    data = CopyFromBitSet<u16>(memory.GetReadData());

    TAU_UNIT_EQ(data, 0, "Expected data at address 0 to be unchanged when full. {}");
}

static void MemoryWrapSet()
{
    TAU_UNIT_TEST();

    using MemoryTest = Memory<16, 5>;

    MemoryTest memory;

    // Trigger an initial clock cycle to force a clear.
    memory.SetWriteClock(true);
    memory.SetWriteClock(false);

    u16 data = CopyFromBitSet<u16>(memory.GetReadData());

    TAU_UNIT_EQ(data, 0, "Expected data to start at 0. {}");

    data = 0xA1B2;

    memory.SetWriteData(CopyToBitSet(data));

    data = CopyFromBitSet<u16>(memory.GetReadData());

    TAU_UNIT_EQ(data, 0, "Data should not have changed without a clock cycle. {}");

    memory.SetWriteAddress(63);
    memory.SetReadAddress(31);
    memory.SetWriteClockEnable(true);

    data = CopyFromBitSet<u16>(memory.GetReadData());

    TAU_UNIT_EQ(data, 0, "Data should not have changed without a clock cycle. {}");

    // Trigger a memory cycle.
    memory.SetWriteClock(true);

    data = CopyFromBitSet<u16>(memory.GetReadData());

    TAU_UNIT_EQ(data, 0xA1B2, "Expected data at address 31 to be returned. {}");

    memory.SetWriteClock(false);

    data = CopyFromBitSet<u16>(memory.GetReadData());

    TAU_UNIT_EQ(data, 0xA1B2, "Expected data at address 31 to be returned. {}");
}

struct TestStruct
{
    u64 A;
    u64 B;
};

static void MemoryLargeSet()
{
    TAU_UNIT_TEST();

    constexpr u64 TestA = 0x0123'4567'89AB'CDEF;
    constexpr u64 TestB = 0x3141'1592'6535'8979;

    using MemoryTest = Memory<sizeof(TestStruct) * CHAR_BIT, 5>;

    MemoryTest memory;

    // Trigger an initial clock cycle to force a clear.
    memory.SetWriteClock(true);
    memory.SetWriteClock(false);

    TestStruct data = CopyFromBitSet<TestStruct>(memory.GetReadData());

    TAU_UNIT_EQ(data.A, 0, "Expected data to start at 0. {}");
    TAU_UNIT_EQ(data.B, 0, "Expected data to start at 0. {}");

    data.A = TestA;
    data.B = TestB;

    memory.SetWriteData(CopyToBitSet(data));

    data = CopyFromBitSet<TestStruct>(memory.GetReadData());

    TAU_UNIT_EQ(data.A, 0, "Data should not have changed without a clock cycle. {}");
    TAU_UNIT_EQ(data.B, 0, "Data should not have changed without a clock cycle. {}");

    memory.SetWriteAddress(7);
    memory.SetReadAddress(7);
    memory.SetWriteClockEnable(true);

    data = CopyFromBitSet<TestStruct>(memory.GetReadData());

    TAU_UNIT_EQ(data.A, 0, "Data should not have changed without a clock cycle. {}");
    TAU_UNIT_EQ(data.B, 0, "Data should not have changed without a clock cycle. {}");

    // Trigger a memory cycle.
    memory.SetWriteClock(true);

    data = CopyFromBitSet<TestStruct>(memory.GetReadData());

    TAU_UNIT_EQ(data.A, TestA, "Expected data at address 7 to be returned. {}");
    TAU_UNIT_EQ(data.B, TestB, "Expected data at address 7 to be returned. {}");

    memory.SetWriteClock(false);

    data = CopyFromBitSet<TestStruct>(memory.GetReadData());

    TAU_UNIT_EQ(data.A, TestA, "Expected data at address 7 to be returned. {}");
    TAU_UNIT_EQ(data.B, TestB, "Expected data at address 7 to be returned. {}");
}

}
