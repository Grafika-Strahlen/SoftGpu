/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include "WritePointer.hpp"
#include <TauUnit.hpp>

namespace riscv::fifo::test {

struct WritePointerReceiver
{
    bool Log = true;

    bool Full = false;
    u64 Pointer = 0;
    u64 Address = 0;
    int ReceivedCount = 0;

    void ReceiveWritePointer_WriteFull(const u32 index, const bool writeFull) noexcept
    {
        (void) index;
        Full = writeFull;
        ++ReceivedCount;
        if(Log)
        {
            ConPrinter::Print("Received Full: {}\n", Full);
        }
    }

    void ReceiveWritePointer_WritePointer(const u32 index, const u64 writePointer) noexcept
    {
        (void) index;
        Pointer = writePointer;
        ++ReceivedCount;
        if(Log)
        {
            ConPrinter::Print("Received Pointer: {}\n", Pointer);
        }
    }

    void ReceiveWritePointer_WriteAddress(const u32 index, const u64 writeAddress) noexcept
    {
        (void) index;
        Address = writeAddress;
        ++ReceivedCount;
        if(Log)
        {
            ConPrinter::Print("Received Address: {}\n", Address);
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

[[maybe_unused]] static void WritePointerResetTest(const bool log = false)
{
    TAU_UNIT_TEST();

    using RecType = WritePointerReceiver;
    using IpType = WritePointer<RecType>;

    RecType receiver {};
    IpType ip(&receiver);

    receiver.Log = log;

    ip.SetWriteIncoming(false);
    ip.SetWriteClockReadAddress(0);

    ip.SetWriteResetN(false);

    TAU_UNIT_EQ(receiver.Full, true, "Full should be true during reset. {}");
    TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 during reset. {}");
    TAU_UNIT_EQ(receiver.Address, 0, "Address should be 0 during reset. {}");
    TAU_UNIT_EQ(receiver.ReceivedCount, 3, "Should have received 3 updates after reset. {}");

    receiver.ResetReceive();

    for(uSys i = 0; i < 4; ++i)
    {
        receiver.ResetReceive();

        ip.SetWriteClock(true);

        TAU_UNIT_EQ(receiver.Full, true, "Full should be true during reset. {}");
        TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 during reset. {}");
        TAU_UNIT_EQ(receiver.Address, 0, "Address should be 0 during reset. {}");
        TAU_UNIT_EQ(receiver.ReceivedCount, 3, "Should have received {} updates after reset and clock raise. {}", 3);

        receiver.ResetReceive();

        ip.SetWriteClock(false);

        TAU_UNIT_EQ(receiver.Full, true, "Full should be true during reset. {}");
        TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 during reset. {}");
        TAU_UNIT_EQ(receiver.Address, 0, "Address should be 0 during reset. {}");
        TAU_UNIT_EQ(receiver.ReceivedCount, 3, "Should have received {} updates after reset and clock raise and lower. {}", 3);
    }
}

[[maybe_unused]] static void WritePointer1BitFullTest(const bool log = false)
{
    TAU_UNIT_TEST();

    using RecType = WritePointerReceiver;
    using IpType = WritePointer<RecType, 1>;

    RecType receiver {};
    IpType ip(&receiver);

    receiver.Log = log;

    ip.SetWriteIncoming(true);
    // This is 0x3, since the address is actually 2 bits.
    ip.SetWriteClockReadAddress(3);

    ip.SetWriteResetN(false);

    TAU_UNIT_EQ(receiver.Full, true, "Full should be true during reset. {}");
    TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 during reset. {}");
    TAU_UNIT_EQ(receiver.Address, 0, "Address should be 0 during reset. {}");
    TAU_UNIT_EQ(receiver.ReceivedCount, 3, "Should have received 3 updates after reset. {}");

    ip.SetWriteResetN(true);

    for(uSys i = 0; i < 3; ++i)
    {
        receiver.ResetReceive();

        ip.SetWriteClock(true);

        TAU_UNIT_EQ(receiver.Full, true, "Full should be true. {}");
        TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0. {}");
        TAU_UNIT_EQ(receiver.Address, 0, "Address should be 0. {}");
        TAU_UNIT_EQ(receiver.ReceivedCount, 3, "Should have received {} updates after reset and clock raise. {}", 3);

        receiver.ResetReceive();

        ip.SetWriteClock(false);

        TAU_UNIT_EQ(receiver.Full, true, "Full should be true. {}");
        TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0. {}");
        TAU_UNIT_EQ(receiver.Address, 0, "Address should be 0. {}");
        TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Should have received {} updates after reset and clock raise and lower. {}", 0);
    }
}

[[maybe_unused]] static void WritePointer1BitTest(const bool log = false)
{
    TAU_UNIT_TEST();

    using RecType = WritePointerReceiver;
    using IpType = WritePointer<RecType, 1>;

    RecType receiver {};
    IpType ip(&receiver);

    receiver.Log = false;

    ip.SetWriteIncoming(true);
    ip.SetWriteClockReadAddress(0);

    ip.SetWriteResetN(false);

    TAU_UNIT_EQ(receiver.Full, true, "Full should be true during reset. {}");
    TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 during reset. {}");
    TAU_UNIT_EQ(receiver.Address, 0, "Address should be 0 during reset. {}");
    TAU_UNIT_EQ(receiver.ReceivedCount, 3, "Should have received 3 updates after reset. {}");

    receiver.Log = log;

    ip.SetWriteResetN(true);

    for(uSys i = 1; i < 8; ++i)
    {
        const uSys address = (i - 1) % 2;
        const uSys pointer = (i - 1) % 4;
        ip.SetWriteClockReadAddress(((i % 4) >> 1) ^ (i % 4));

        receiver.ResetReceive();

        ip.SetWriteClock(true);

        TAU_UNIT_EQ(receiver.Full, false, "Empty should be false. {}");
        TAU_UNIT_EQ(receiver.Pointer, (pointer >> 1) ^ pointer, "Pointer should be {}. {}", (pointer >> 1) ^ pointer);
        TAU_UNIT_EQ(receiver.Address, address, "Address should be {}. {}", address);
        TAU_UNIT_EQ(receiver.ReceivedCount, 3, "Should have received {} updates after reset and clock raise. {}", 3);

        receiver.ResetReceive();

        ip.SetWriteClock(false);

        TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Should have received {} updates after reset and clock raise and lower. {}", 0);
    }
}

[[maybe_unused]] static void WritePointer2BitFullTest(const bool log = false)
{
    TAU_UNIT_TEST();

    using RecType = WritePointerReceiver;
    using IpType = WritePointer<RecType, 2>;

    RecType receiver {};
    IpType ip(&receiver);

    receiver.Log = log;

    ip.SetWriteIncoming(true);
    // This is 0x6, since the address is actually 3 bits, and only the upper 2 bits matter.
    ip.SetWriteClockReadAddress(0x6);

    ip.SetWriteResetN(false);

    TAU_UNIT_EQ(receiver.Full, true, "Full should be true during reset. {}");
    TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 during reset. {}");
    TAU_UNIT_EQ(receiver.Address, 0, "Address should be 0 during reset. {}");
    TAU_UNIT_EQ(receiver.ReceivedCount, 3, "Should have received 3 updates after reset. {}");

    ip.SetWriteResetN(true);

    for(uSys i = 0; i < 3; ++i)
    {
        receiver.ResetReceive();

        ip.SetWriteClock(true);

        TAU_UNIT_EQ(receiver.Full, true, "Full should be true. {}");
        TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0. {}");
        TAU_UNIT_EQ(receiver.Address, 0, "Address should be 0. {}");
        TAU_UNIT_EQ(receiver.ReceivedCount, 3, "Should have received {} updates after reset and clock raise. {}", 3);

        receiver.ResetReceive();

        ip.SetWriteClock(false);

        TAU_UNIT_EQ(receiver.Full, true, "Full should be true. {}");
        TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0. {}");
        TAU_UNIT_EQ(receiver.Address, 0, "Address should be 0. {}");
        TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Should have received {} updates after reset and clock raise and lower. {}", 0);
    }
}

[[maybe_unused]] static void WritePointer2BitTest(const bool log = false)
{
    TAU_UNIT_TEST();

    using RecType = WritePointerReceiver;
    using IpType = WritePointer<RecType, 2>;

    RecType receiver {};
    IpType ip(&receiver);

    receiver.Log = false;

    ip.SetWriteIncoming(true);
    ip.SetWriteClockReadAddress(0);

    ip.SetWriteResetN(false);

    TAU_UNIT_EQ(receiver.Full, true, "Full should be true during reset. {}");
    TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 during reset. {}");
    TAU_UNIT_EQ(receiver.Address, 0, "Address should be 0 during reset. {}");
    TAU_UNIT_EQ(receiver.ReceivedCount, 3, "Should have received 3 updates after reset. {}");

    receiver.Log = log;

    ip.SetWriteResetN(true);

    for(uSys i = 1; i < 16; ++i)
    {
        const uSys address = (i - 1) % 4;
        const uSys pointer = (i - 1) % 8;
        ip.SetWriteClockReadAddress(((i % 8) >> 1) ^ (i % 8));

        receiver.ResetReceive();

        ip.SetWriteClock(true);

        TAU_UNIT_EQ(receiver.Full, false, "Empty should be false. {}");
        TAU_UNIT_EQ(receiver.Pointer, (pointer >> 1) ^ pointer, "Pointer should be {}. {}", (pointer >> 1) ^ pointer);
        TAU_UNIT_EQ(receiver.Address, address, "Address should be {}. {}", address);
        TAU_UNIT_EQ(receiver.ReceivedCount, 3, "Should have received {} updates after reset and clock raise. {}", 3);

        receiver.ResetReceive();

        ip.SetWriteClock(false);

        TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Should have received {} updates after reset and clock raise and lower. {}", 0);
    }
}

}
