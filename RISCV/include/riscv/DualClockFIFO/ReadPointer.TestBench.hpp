/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include "ReadPointer.hpp"
#include <TauUnit.hpp>

namespace riscv::fifo::test {

struct ReadPointerReceiver
{
    bool Log = true;

    bool Empty = false;
    u64 Pointer = 0;
    u64 Address = 0;
    int ReceivedCount = 0;

    void ReceiveReadPointer_ReadEmpty(const u32 index, const bool readEmpty) noexcept
    {
        (void) index;
        Empty = readEmpty;
        ++ReceivedCount;
        if(Log)
        {
            ConPrinter::Print("Received Empty: {}\n", Empty);
        }
    }

    void ReceiveReadPointer_ReadPointer(const u32 index, const u64 readPointer) noexcept
    {
        (void) index;
        Pointer = readPointer;
        ++ReceivedCount;
        if(Log)
        {
            ConPrinter::Print("Received Pointer: {}\n", Pointer);
        }
    }

    void ReceiveReadPointer_ReadAddress(const u32 index, const u64 readAddress) noexcept
    {
        (void) index;
        Address = readAddress;
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

static void ReadPointerResetTest(const bool log = false)
{
    TAU_UNIT_TEST();

    using RecType = ReadPointerReceiver;
    using IpType = ReadPointer<RecType>;

    RecType receiver {};
    IpType ip(&receiver);

    receiver.Log = log;

    ip.SetReadIncoming(false);
    ip.SetReadClockWriteAddress(0);

    ip.SetReadResetN(false);

    TAU_UNIT_EQ(receiver.Empty, true, "Empty should be true during reset. {}");
    TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 during reset. {}");
    TAU_UNIT_EQ(receiver.Address, 0, "Address should be 0 during reset. {}");
    TAU_UNIT_EQ(receiver.ReceivedCount, 3, "Should have received 3 updates after reset. {}");

    receiver.ResetReceive();

    for(uSys i = 0; i < 4; ++i)
    {
        receiver.ResetReceive();

        ip.SetReadClock(true);

        TAU_UNIT_EQ(receiver.Empty, true, "Empty should be true during reset. {}");
        TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 during reset. {}");
        TAU_UNIT_EQ(receiver.Address, 0, "Address should be 0 during reset. {}");
        TAU_UNIT_EQ(receiver.ReceivedCount, 3, "Should have received {} updates after reset and clock raise. {}", 3);

        receiver.ResetReceive();

        ip.SetReadClock(false);

        TAU_UNIT_EQ(receiver.Empty, true, "Empty should be true during reset. {}");
        TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 during reset. {}");
        TAU_UNIT_EQ(receiver.Address, 0, "Address should be 0 during reset. {}");
        TAU_UNIT_EQ(receiver.ReceivedCount, 3, "Should have received {} updates after reset and clock raise and lower. {}", 3);
    }
}

static void ReadPointer1BitEmptyTest(const bool log = false)
{
    TAU_UNIT_TEST();

    using RecType = ReadPointerReceiver;
    using IpType = ReadPointer<RecType, 1>;

    RecType receiver {};
    IpType ip(&receiver);

    receiver.Log = log;

    ip.SetReadIncoming(true);
    ip.SetReadClockWriteAddress(0);

    ip.SetReadResetN(false);

    TAU_UNIT_EQ(receiver.Empty, true, "Empty should be true during reset. {}");
    TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 during reset. {}");
    TAU_UNIT_EQ(receiver.Address, 0, "Address should be 0 during reset. {}");
    TAU_UNIT_EQ(receiver.ReceivedCount, 3, "Should have received 3 updates after reset. {}");

    ip.SetReadResetN(true);

    for(uSys i = 0; i < 3; ++i)
    {
        receiver.ResetReceive();

        ip.SetReadClock(true);

        TAU_UNIT_EQ(receiver.Empty, true, "Empty should be true. {}");
        TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0. {}");
        TAU_UNIT_EQ(receiver.Address, 0, "Address should be 0. {}");
        TAU_UNIT_EQ(receiver.ReceivedCount, 3, "Should have received {} updates after reset and clock raise. {}", 3);

        receiver.ResetReceive();

        ip.SetReadClock(false);

        TAU_UNIT_EQ(receiver.Empty, true, "Empty should be true. {}");
        TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0. {}");
        TAU_UNIT_EQ(receiver.Address, 0, "Address should be 0. {}");
        TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Should have received {} updates after reset and clock raise and lower. {}", 0);
    }
}

static void ReadPointer1BitTest(const bool log = false)
{
    TAU_UNIT_TEST();

    using RecType = ReadPointerReceiver;
    using IpType = ReadPointer<RecType, 1>;

    RecType receiver {};
    IpType ip(&receiver);

    receiver.Log = false;

    ip.SetReadIncoming(true);
    ip.SetReadClockWriteAddress(0);

    ip.SetReadResetN(false);

    TAU_UNIT_EQ(receiver.Empty, true, "Empty should be true during reset. {}");
    TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 during reset. {}");
    TAU_UNIT_EQ(receiver.Address, 0, "Address should be 0 during reset. {}");
    TAU_UNIT_EQ(receiver.ReceivedCount, 3, "Should have received 3 updates after reset. {}");

    receiver.Log = log;

    ip.SetReadResetN(true);

    for(uSys i = 1; i < 8; ++i)
    {
        const uSys address = (i - 1) % 2;
        const uSys pointer = (i - 1) % 4;
        ip.SetReadClockWriteAddress(((i % 4) >> 1) ^ (i % 4));

        receiver.ResetReceive();

        ip.SetReadClock(true);

        TAU_UNIT_EQ(receiver.Empty, false, "Empty should be false. {}");
        TAU_UNIT_EQ(receiver.Pointer, (pointer >> 1) ^ pointer, "Pointer should be {}. {}", (pointer >> 1) ^ pointer);
        TAU_UNIT_EQ(receiver.Address, address, "Address should be {}. {}", address);
        TAU_UNIT_EQ(receiver.ReceivedCount, 3, "Should have received {} updates after reset and clock raise. {}", 3);

        receiver.ResetReceive();

        ip.SetReadClock(false);

        TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Should have received {} updates after reset and clock raise and lower. {}", 0);
    }
}

static void ReadPointer2BitEmptyTest(const bool log = false)
{
    TAU_UNIT_TEST();

    using RecType = ReadPointerReceiver;
    using IpType = ReadPointer<RecType, 2>;

    RecType receiver {};
    IpType ip(&receiver);

    receiver.Log = log;

    ip.SetReadIncoming(true);
    ip.SetReadClockWriteAddress(0);

    ip.SetReadResetN(false);

    TAU_UNIT_EQ(receiver.Empty, true, "Empty should be true during reset. {}");
    TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 during reset. {}");
    TAU_UNIT_EQ(receiver.Address, 0, "Address should be 0 during reset. {}");
    TAU_UNIT_EQ(receiver.ReceivedCount, 3, "Should have received 3 updates after reset. {}");

    ip.SetReadResetN(true);

    for(uSys i = 0; i < 3; ++i)
    {
        receiver.ResetReceive();

        ip.SetReadClock(true);

        TAU_UNIT_EQ(receiver.Empty, true, "Empty should be true. {}");
        TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0. {}");
        TAU_UNIT_EQ(receiver.Address, 0, "Address should be 0. {}");
        TAU_UNIT_EQ(receiver.ReceivedCount, 3, "Should have received {} updates after reset and clock raise. {}", 3);

        receiver.ResetReceive();

        ip.SetReadClock(false);

        TAU_UNIT_EQ(receiver.Empty, true, "Empty should be true. {}");
        TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0. {}");
        TAU_UNIT_EQ(receiver.Address, 0, "Address should be 0. {}");
        TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Should have received {} updates after reset and clock raise and lower. {}", 0);
    }
}

static void ReadPointer2BitTest(const bool log = false)
{
    TAU_UNIT_TEST();

    using RecType = ReadPointerReceiver;
    using IpType = ReadPointer<RecType, 2>;

    RecType receiver {};
    IpType ip(&receiver);

    receiver.Log = false;

    ip.SetReadIncoming(true);
    ip.SetReadClockWriteAddress(0);

    ip.SetReadResetN(false);

    TAU_UNIT_EQ(receiver.Empty, true, "Empty should be true during reset. {}");
    TAU_UNIT_EQ(receiver.Pointer, 0, "Pointer should be 0 during reset. {}");
    TAU_UNIT_EQ(receiver.Address, 0, "Address should be 0 during reset. {}");
    TAU_UNIT_EQ(receiver.ReceivedCount, 3, "Should have received 3 updates after reset. {}");

    receiver.Log = log;

    ip.SetReadResetN(true);

    for(uSys i = 1; i < 16; ++i)
    {
        const uSys address = (i - 1) % 4;
        const uSys pointer = (i - 1) % 8;
        ip.SetReadClockWriteAddress(((i % 8) >> 1) ^ (i % 8));

        receiver.ResetReceive();

        ip.SetReadClock(true);

        TAU_UNIT_EQ(receiver.Empty, false, "Empty should be false. {}");
        TAU_UNIT_EQ(receiver.Pointer, (pointer >> 1) ^ pointer, "Pointer should be {}. {}", (pointer >> 1) ^ pointer);
        TAU_UNIT_EQ(receiver.Address, address, "Address should be {}. {}", address);
        TAU_UNIT_EQ(receiver.ReceivedCount, 3, "Should have received {} updates after reset and clock raise. {}", 3);

        receiver.ResetReceive();

        ip.SetReadClock(false);

        TAU_UNIT_EQ(receiver.ReceivedCount, 0, "Should have received {} updates after reset and clock raise and lower. {}", 0);
    }
}

}
