/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include "Synchronizer.hpp"
#include <TauUnit.hpp>

namespace riscv::fifo::test {

struct SynchronizerReceiver
{
    bool Log = true;

    u64 Pointer = 0;
    int ReceivedCount = 0;

    void ReceiveSynchronizer_Pointer(const u32 index, const u64 pointer) noexcept
    {
        (void) index;
        Pointer = pointer;
        ++ReceivedCount;
        if(Log)
        {
            ConPrinter::Print("Received Pointer: {}\n", Pointer);
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

[[maybe_unused]] static void SynchronizerResetTest()
{
    TAU_UNIT_TEST();

    using RecType = SynchronizerReceiver;
    using IpType = Synchronizer<RecType>;

    RecType receiver {};
    IpType sync(&receiver);

    sync.SetPointer(1);

    sync.SetResetN(false);

    TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 during reset. {}");
    TAU_UNIT_EQ(receiver.ReceivedCount, 1, "Should have received 1 pointer update after reset. {}");

    receiver.ResetReceive();

    for(int i = 0; i < 4; ++i)
    {
        sync.SetClock(true);

        TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 during reset.");
        TAU_UNIT_EQ(receiver.ReceivedCount, 2 * i + 1, "Should have received {} pointer update after reset and clock raise. {}", 2 * i + 1);

        sync.SetClock(false);

        TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 during reset.");
        TAU_UNIT_EQ(receiver.ReceivedCount, 2 * i + 2, "Should have received {} pointer update after reset and clock raise and lower. {}", 2 * i + 2);
    }
}

[[maybe_unused]] static void SynchronizerSingleTest()
{
    TAU_UNIT_TEST();

    using RecType = SynchronizerReceiver;
    using IpType = Synchronizer<RecType>;

    RecType receiver {};
    IpType sync(&receiver);

    sync.SetPointer(1);

    sync.SetResetN(true);

    TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Should not have received pointer update after clearing reset. {}");

    receiver.ResetReceive();

    sync.SetClock(true);

    TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 after first clock raise.");
    TAU_UNIT_EQ(receiver.ReceivedCount, 1, "Should have received 1 pointer update after clock raise. {}");

    receiver.ResetReceive();

    sync.SetClock(false);

    TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 after first clock lower.");
    TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Should not have received pointer update after clock lower. {}");

    receiver.ResetReceive();

    sync.SetClock(true);

    TAU_UNIT_EQ(receiver.Pointer, 1, "Pointer should be 1 after second clock raise.");
    TAU_UNIT_EQ(receiver.ReceivedCount, 1, "Should have received 1 pointer update after clock raise. {}");

    receiver.ResetReceive();

    sync.SetClock(false);

    TAU_UNIT_EQ(receiver.Pointer, 1, "Pointer should be 1 after second clock lower.");
    TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Should not have received pointer update after clock lower. {}");

    receiver.ResetReceive();

    sync.SetClock(true);

    TAU_UNIT_EQ(receiver.Pointer, 1, "Pointer should be 1 after third clock raise.");
    TAU_UNIT_EQ(receiver.ReceivedCount, 1, "Should have received 1 pointer update after clock raise. {}");

    receiver.ResetReceive();

    sync.SetClock(false);

    TAU_UNIT_EQ(receiver.Pointer, 1, "Pointer should be 1 after third clock lower.");
    TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Should not have received pointer update after clock lower. {}");
}

[[maybe_unused]] static void SynchronizerIncrementSingleBitTest()
{
    TAU_UNIT_TEST();

    using RecType = SynchronizerReceiver;
    using IpType = Synchronizer<RecType, 0>;

    RecType receiver {};
    IpType sync(&receiver);

    sync.SetResetN(true);

    TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Should not have received pointer update after clearing reset. {}");

    receiver.ResetReceive();

    sync.SetClock(true);

    TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 after first clock raise.");
    TAU_UNIT_EQ(receiver.ReceivedCount, 1, "Should have received 1 pointer update after clock raise. {}");
    receiver.ResetReceive();

    sync.SetClock(false);

    TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 after first clock lower.");
    TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Should not have received pointer update after clock lower. {}");

    for(uSys i = 1; i < 8; ++i)
    {
        sync.SetPointer(i);

        receiver.ResetReceive();

        sync.SetClock(true);

        TAU_UNIT_EQ(receiver.Pointer, (i - 1) % 2, "Pointer should be {} after second clock raise.", (i - 1) % 2);
        TAU_UNIT_EQ(receiver.ReceivedCount, 1, "Should have received 1 pointer update after clock raise. {}");

        receiver.ResetReceive();

        sync.SetClock(false);

        TAU_UNIT_EQ(receiver.Pointer, (i - 1) % 2, "Pointer should be {} after second clock lower.", (i - 1) % 2);
        TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Should not have received pointer update after clock lower. {}");
    }
}

[[maybe_unused]] static void SynchronizerIncrementDualBitTest()
{
    TAU_UNIT_TEST();

    using RecType = SynchronizerReceiver;
    using IpType = Synchronizer<RecType, 1>;

    RecType receiver {};
    IpType sync(&receiver);

    sync.SetResetN(true);

    TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Should not have received pointer update after clearing reset. {}");

    receiver.ResetReceive();

    sync.SetClock(true);

    TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 after first clock raise.");
    TAU_UNIT_EQ(receiver.ReceivedCount, 1, "Should have received 1 pointer update after clock raise. {}");
    receiver.ResetReceive();

    sync.SetClock(false);

    TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 after first clock lower.");
    TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Should not have received pointer update after clock lower. {}");

    for(uSys i = 1; i < 12; ++i)
    {
        sync.SetPointer(i);

        receiver.ResetReceive();

        sync.SetClock(true);

        TAU_UNIT_EQ(receiver.Pointer, (i - 1) % 4, "Pointer should be {} after second clock raise.", (i - 1) % 4);
        TAU_UNIT_EQ(receiver.ReceivedCount, 1, "Should have received 1 pointer update after clock raise. {}");

        receiver.ResetReceive();

        sync.SetClock(false);

        TAU_UNIT_EQ(receiver.Pointer, (i - 1) % 4, "Pointer should be {} after second clock lower.", (i - 1) % 4);
        TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Should not have received pointer update after clock lower. {}");
    }
}

[[maybe_unused]] static void SynchronizerIncrement6BitTest()
{
    TAU_UNIT_TEST();

    using RecType = SynchronizerReceiver;
    using IpType = Synchronizer<RecType, 6>;

    RecType receiver {};
    IpType sync(&receiver);
    receiver.Log = false;

    sync.SetResetN(true);

    TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Should not have received pointer update after clearing reset. {}");

    receiver.ResetReceive();

    sync.SetClock(true);

    TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 after first clock raise.");
    TAU_UNIT_EQ(receiver.ReceivedCount, 1, "Should have received 1 pointer update after clock raise. {}");
    receiver.ResetReceive();

    sync.SetClock(false);

    TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 after first clock lower.");
    TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Should not have received pointer update after clock lower. {}");

    for(uSys i = 1; i < 64; ++i)
    {
        sync.SetPointer(i);

        receiver.ResetReceive();

        sync.SetClock(true);

        TAU_UNIT_EQ(receiver.Pointer, (i - 1) % 64, "Pointer should be {} after second clock raise.", (i - 1) % 64);
        TAU_UNIT_EQ(receiver.ReceivedCount, 1, "Should have received 1 pointer update after clock raise. {}");

        receiver.ResetReceive();

        sync.SetClock(false);

        TAU_UNIT_EQ(receiver.Pointer, (i - 1) % 64, "Pointer should be {} after second clock lower.", (i - 1) % 64);
        TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Should not have received pointer update after clock lower. {}");
    }
}

}
