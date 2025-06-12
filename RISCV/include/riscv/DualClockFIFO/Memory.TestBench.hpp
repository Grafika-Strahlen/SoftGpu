#pragma once

#include "Memory.hpp"

namespace riscv::fifo::test {

struct TestStruct
{
    u64 A;
    u64 B;
};

}

namespace ConPrinter {

inline u32 Print(const riscv::fifo::test::TestStruct& test) noexcept;

}

#include <TauUnit.hpp>

namespace ConPrinter {

inline u32 Print(const riscv::fifo::test::TestStruct& test) noexcept
{
    return Print("({}, {})", test.A, test.B);
}

}


namespace riscv::fifo::test {

template<typename DataType>
struct MemoryReceiver
{
    DataType Data = {};
    int ReceivedCount = 0;

    void ReceiveMemory_ReadData(const u32 index, const DataType& data) noexcept
    {
        (void) index;
        Data = data;
        ++ReceivedCount;
        ConPrinter::PrintLn("Received Data: {}", data);
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

static void MemorySimpleSet()
{
    TAU_UNIT_TEST();

    using DataType = u16;
    using RecType = MemoryReceiver<DataType>;
    using IpType = Memory<RecType, DataType, 5>;

    RecType receiver {};
    IpType memory(&receiver);

    // Trigger an initial clock cycle to force a clear.
    memory.SetWriteClock(true);
    memory.SetWriteClock(false);

    TAU_UNIT_EQ(receiver.ReceivedCount, 2, "Expected to have received 2 memory updates. {}");
    receiver.ResetReceive();

    DataType data = receiver.Data;

    TAU_UNIT_EQ(data, 0, "Expected data to start at 0. {}");

    data = 0xA1B2;

    memory.SetWriteData(data);

    TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Expected to have received 0 memory updates. {}");
    receiver.ResetReceive();

    data = receiver.Data;

    TAU_UNIT_EQ(data, 0, "Data should not have changed without a clock cycle. {}");

    memory.SetWriteAddress(0);
    memory.SetReadAddress(0);
    memory.SetWriteClockEnable(true);

    TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Expected to have received 0 memory updates. {}");
    receiver.ResetReceive();

    data = receiver.Data;

    TAU_UNIT_EQ(data, 0, "Data should not have changed without a clock cycle. {}");

    // Trigger a memory cycle.
    memory.SetWriteClock(true);

    TAU_UNIT_EQ(receiver.ReceivedCount, 1, "Expected to have received 1 memory updates. {}");
    receiver.ResetReceive();

    data = receiver.Data;

    TAU_UNIT_EQ(data, 0xA1B2, "Expected data at address 0 to be returned. {}");

    memory.SetWriteClock(false);

    TAU_UNIT_EQ(receiver.ReceivedCount, 1, "Expected to have received 1 memory updates. {}");
    receiver.ResetReceive();

    data = receiver.Data;

    TAU_UNIT_EQ(data, 0xA1B2, "Expected data at address 0 to be returned. {}");
}

static void MemoryFullSet()
{
    TAU_UNIT_TEST();

    using DataType = u16;
    using RecType = MemoryReceiver<DataType>;
    using IpType = Memory<RecType, DataType, 5>;

    RecType receiver {};
    IpType memory(&receiver);

    // Trigger an initial clock cycle to force a clear.
    memory.SetWriteClock(true);
    memory.SetWriteClock(false);

    TAU_UNIT_EQ(receiver.ReceivedCount, 2, "Expected to have received 2 memory updates. {}");
    receiver.ResetReceive();

    memory.SetWriteFull(true);

    TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Expected to have received 0 memory updates. {}");
    receiver.ResetReceive();

    DataType data = receiver.Data;

    TAU_UNIT_EQ(data, 0, "Expected data to start at 0. {}");

    data = 0xA1B2;

    memory.SetWriteData(data);

    TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Expected to have received 0 memory updates. {}");
    receiver.ResetReceive();

    data = receiver.Data;

    TAU_UNIT_EQ(data, 0, "Data should not have changed without a clock cycle. {}");

    memory.SetWriteAddress(0);
    memory.SetReadAddress(0);
    memory.SetWriteClockEnable(true);

    data = receiver.Data;

    TAU_UNIT_EQ(data, 0, "Data should not have changed without a clock cycle. {}");

    // Trigger a memory cycle.
    memory.SetWriteClock(true);

    TAU_UNIT_EQ(receiver.ReceivedCount, 1, "Expected to have received 1 memory updates. {}");
    receiver.ResetReceive();

    data = receiver.Data;

    TAU_UNIT_EQ(data, 0, "Expected data at address 0 to be unchanged when full. {}");

    memory.SetWriteClock(false);

    TAU_UNIT_EQ(receiver.ReceivedCount, 1, "Expected to have received 1 memory updates. {}");
    receiver.ResetReceive();

    data = receiver.Data;

    TAU_UNIT_EQ(data, 0, "Expected data at address 0 to be unchanged when full. {}");
}

static void MemoryWrapSet()
{
    TAU_UNIT_TEST();

    using DataType = u16;
    using RecType = MemoryReceiver<DataType>;
    using IpType = Memory<RecType, DataType, 5>;

    RecType receiver {};
    IpType memory(&receiver);

    // Trigger an initial clock cycle to force a clear.
    memory.SetWriteClock(true);
    memory.SetWriteClock(false);

    DataType data = receiver.Data;

    TAU_UNIT_EQ(data, 0, "Expected data to start at 0. {}");

    data = 0xA1B2;

    memory.SetWriteData(data);

    data = receiver.Data;

    TAU_UNIT_EQ(data, 0, "Data should not have changed without a clock cycle. {}");

    memory.SetWriteAddress(63);
    memory.SetReadAddress(31);
    memory.SetWriteClockEnable(true);

    data = receiver.Data;

    TAU_UNIT_EQ(data, 0, "Data should not have changed without a clock cycle. {}");

    // Trigger a memory cycle.
    memory.SetWriteClock(true);

    data = receiver.Data;

    TAU_UNIT_EQ(data, 0xA1B2, "Expected data at address 31 to be returned. {}");

    memory.SetWriteClock(false);

    data = receiver.Data;

    TAU_UNIT_EQ(data, 0xA1B2, "Expected data at address 31 to be returned. {}");
}

static void MemoryLargeSet()
{
    TAU_UNIT_TEST();

    constexpr u64 TestA = 0x0123'4567'89AB'CDEF;
    constexpr u64 TestB = 0x3141'1592'6535'8979;

    using DataType = TestStruct;
    using RecType = MemoryReceiver<DataType>;
    using IpType = Memory<RecType, DataType, 5>;

    RecType receiver {};
    IpType memory(&receiver);

    // Trigger an initial clock cycle to force a clear.
    memory.SetWriteClock(true);
    memory.SetWriteClock(false);

    DataType data = receiver.Data;

    TAU_UNIT_EQ(data.A, 0, "Expected data to start at 0. {}");
    TAU_UNIT_EQ(data.B, 0, "Expected data to start at 0. {}");

    data.A = TestA;
    data.B = TestB;

    memory.SetWriteData(data);

    data = receiver.Data;

    TAU_UNIT_EQ(data.A, 0, "Data should not have changed without a clock cycle. {}");
    TAU_UNIT_EQ(data.B, 0, "Data should not have changed without a clock cycle. {}");

    memory.SetWriteAddress(7);
    memory.SetReadAddress(7);
    memory.SetWriteClockEnable(true);

    data = receiver.Data;

    TAU_UNIT_EQ(data.A, 0, "Data should not have changed without a clock cycle. {}");
    TAU_UNIT_EQ(data.B, 0, "Data should not have changed without a clock cycle. {}");

    // Trigger a memory cycle.
    memory.SetWriteClock(true);

    data = receiver.Data;

    TAU_UNIT_EQ(data.A, TestA, "Expected data at address 7 to be returned. {}");
    TAU_UNIT_EQ(data.B, TestB, "Expected data at address 7 to be returned. {}");

    memory.SetWriteClock(false);

    data = receiver.Data;

    TAU_UNIT_EQ(data.A, TestA, "Expected data at address 7 to be returned. {}");
    TAU_UNIT_EQ(data.B, TestB, "Expected data at address 7 to be returned. {}");
}

}
