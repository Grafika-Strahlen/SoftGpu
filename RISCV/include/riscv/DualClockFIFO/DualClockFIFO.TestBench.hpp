/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include "DualClockFIFO.hpp"
#include <TauUnit.hpp>
#include <thread>
#include <vector>
#include <atomic>

namespace riscv::fifo::test {

template<typename DataType>
struct FifoReceiver
{
    bool Full = false;
    bool Empty = false;
    DataType Data = {};
    u64 WriteAddress = 0;
    u64 ReadAddress = 0;

    void ReceiveDualClockFIFO_WriteFull(const u32 index, const bool writeFull) noexcept
    {
        Full = writeFull;
        // auto message = Format<char>("Full: {}\n", Full);
        // ConPrinter::Print(message);
    }

    void ReceiveDualClockFIFO_ReadData(const u32 index, const DataType& data) noexcept
    {
        Data = data;
        // auto message = Format<char>("Data: {}\n", Data);
        // ConPrinter::Print(message);
    }

    void ReceiveDualClockFIFO_ReadEmpty(const u32 index, const bool readEmpty) noexcept
    {
        Empty = readEmpty;
        // auto message = Format<char>("Empty: {}\n", Empty);
        // ConPrinter::Print(message);
    }

    void ReceiveDualClockFIFO_WriteAddress(const u32 index, const u64 writeAddress) noexcept
    {
        WriteAddress = writeAddress;
        // auto message = Format<char>("WriteAddress: {}\n", WriteAddress);
        // ConPrinter::Print(message);
    }

    void ReceiveDualClockFIFO_ReadAddress(const u32 index, const u64 readAddress) noexcept
    {
        ReadAddress = readAddress;
        // auto message = Format<char>("ReadAddress: {}\n", ReadAddress);
        // ConPrinter::Print(message);
    }
};

static u32 TargetDataSet[]
{
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29,
    31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
    73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
    127, 131, 137, 139, 149, 151, 157, 163, 167, 173,
    179, 181, 191, 193, 197, 199, 211, 223, 227, 229,
    233, 239, 241, 251, 257, 263, 269, 271, 277, 281,
    283, 293, 307, 311, 313, 317, 331, 337, 347, 349,
    353, 359, 367, 373, 379, 383, 389, 397, 401, 409,
    419, 421, 431, 433, 439, 443, 449, 457, 461, 463,
    467, 479, 487, 491, 499, 503, 509, 521, 523, 541,
    547, 557, 563, 569, 571, 577, 587, 593, 599, 601,
    607, 613, 617, 619, 631, 641, 643, 647, 653, 659,
    661, 673, 677, 683, 691, 701, 709, 719, 727, 733,
    739, 743, 751, 757, 761, 769, 773, 787, 797, 809,
    811, 821, 823, 827, 829, 839, 853, 857, 859, 863,
    877, 881, 883, 887, 907, 911, 919, 929, 937, 941,
    947, 953, 967, 971, 977, 983, 991, 997, 1009, 1013,
    1019, 1021, 1031, 1033, 1039, 1049, 1051, 1061, 1063, 1069,
    1087, 1091, 1093, 1097, 1103, 1109, 1117, 1123, 1129, 1151,
    1153, 1163, 1171, 1181, 1187, 1193, 1201, 1213, 1217, 1223
};

template<u32 WriterDelay, u32 ReaderDelay, u32 BitCount>
static void FifoTestRunner()
{
    using DataType = u32;
    using RecType = FifoReceiver<DataType>;
    using IpType = DualClockFIFO<RecType, DataType, BitCount>;

    RecType receiver {};
    IpType fifo(&receiver);

    ::std::vector<u32> receivedValues;

    ::std::atomic_bool shouldStop = false;

    ::std::thread writerThread(
        [&]()
        {
            fifo.SetWriteResetN(true);

            fifo.SetWriteClock(true);
            fifo.SetWriteClock(false);

            uSys i = 0;

            while(!shouldStop)
            {
                if(i >= ::std::size(TargetDataSet))
                {
                    break;
                }

                bool triggered = false;

                if(!receiver.Full)
                {
                    triggered = true;
                    fifo.SetWriteIncoming(true);
                    fifo.SetWriteData(TargetDataSet[i++]);
                }

                const u64 writeAddress = receiver.WriteAddress;

                fifo.SetWriteClock(true);

                if(triggered)
                {
                    auto message = Format<char>("\x1B[0;35mWrote {p} at address {p}\x1B[0m\n", TargetDataSet[i - 1], writeAddress);
                    ConPrinter::Print(message);
                }

                ::std::this_thread::sleep_for(::std::chrono::milliseconds(WriterDelay));
                fifo.SetWriteClock(false);
                ::std::this_thread::sleep_for(::std::chrono::milliseconds(WriterDelay));

                fifo.SetWriteIncoming(false);
            }
        }
    );

    ::std::thread readerThread(
        [&]()
        {
            fifo.SetReadResetN(true);

            fifo.SetReadClock(true);
            fifo.SetReadClock(false);

            while(!shouldStop)
            {
                if(receivedValues.size() >= ::std::size(TargetDataSet))
                {
                    shouldStop = true;
                    break;
                }

                bool triggered = false;

                if(!receiver.Empty)
                {
                    triggered = true;
                    fifo.SetReadIncoming(true);
                }

                const u64 readAddress = receiver.ReadAddress;

                if(triggered)
                {
                    receivedValues.push_back(receiver.Data);
                    auto message = Format<char>("\x1B[1;30mRead  {p} at address {p}\x1B[0m\n", receiver.Data, readAddress);
                    ConPrinter::Print(message);

                    if(receiver.Data != TargetDataSet[receivedValues.size() - 1])
                    {
                        shouldStop = true;
                        message = Format<char>("Sequence did not match at index {}\n", receivedValues.size() - 1);
                        ConPrinter::Print(message);
                    }

                    if(receivedValues.size() > 2)
                    {
                        int x = 0;
                        // break;
                    }
                }


                fifo.SetReadClock(true);
                ::std::this_thread::sleep_for(::std::chrono::milliseconds(ReaderDelay));
                fifo.SetReadClock(false);
                ::std::this_thread::sleep_for(::std::chrono::milliseconds(ReaderDelay));

                fifo.SetReadIncoming(false);
            }
        }
    );

    writerThread.join();
    readerThread.join();

    TAU_UNIT_EQ(receivedValues.size(), ::std::size(TargetDataSet), "Expected received counts to match. {}");

    for(uSys i = 0; i < receivedValues.size() && i < ::std::size(TargetDataSet); ++i)
    {
        TAU_UNIT_EQ(receivedValues[i], TargetDataSet[i], "Expected values at {} to match. {}", i);
    }
}

static void FifoTestFastRead1Bit()
{
    TAU_UNIT_TEST();

    FifoTestRunner<31, 7, 1>();
}

static void FifoTestFastWrite1Bit()
{
    TAU_UNIT_TEST();

    FifoTestRunner<7, 31, 1>();
}

static void FifoTestFastRead2Bit()
{
    TAU_UNIT_TEST();

    FifoTestRunner<31, 7, 2>();
}

static void FifoTestFastWrite2Bit()
{
    TAU_UNIT_TEST();

    FifoTestRunner<7, 31, 2>();
}

static void FifoTestFastRead3Bit()
{
    TAU_UNIT_TEST();

    FifoTestRunner<31, 7, 3>();
}

static void FifoTestFastWrite3Bit()
{
    TAU_UNIT_TEST();

    FifoTestRunner<7, 31, 3>();
}

static void FifoTestFastRead4Bit()
{
    TAU_UNIT_TEST();

    FifoTestRunner<31, 7, 3>();
}

static void FifoTestFastWrite4Bit()
{
    TAU_UNIT_TEST();

    FifoTestRunner<7, 31, 3>();
}

static void FifoTestFastRead5Bit()
{
    TAU_UNIT_TEST();

    FifoTestRunner<31, 7, 5>();
}

static void FifoTestFastWrite5Bit()
{
    TAU_UNIT_TEST();

    FifoTestRunner<7, 31, 5>();
}

}
