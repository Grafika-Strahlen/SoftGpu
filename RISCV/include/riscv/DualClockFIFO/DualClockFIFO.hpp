#pragma once

#include <Common.hpp>

#include "ReadPointer.hpp"
#include "WritePointer.hpp"
#include "Memory.hpp"
#include "Synchronizer.hpp"

namespace riscv::fifo {

template<typename DataType>
class DualClockFIFOReceiverSample
{
public:
    void ReceiveDualClockFIFO_WriteFull(const u32 index, const bool writeFull) noexcept { }
    void ReceiveDualClockFIFO_ReadData(const u32 index, const DataType& data) noexcept { }
    void ReceiveDualClockFIFO_ReadEmpty(const u32 index, const bool readEmpty) noexcept { }
    void ReceiveDualClockFIFO_WriteAddress(const u32 index, const u64 writeAddress) noexcept { }
    void ReceiveDualClockFIFO_ReadAddress(const u32 index, const u64 readAddress) noexcept { }
};

// Mostly just implemented from http://www.sunburst-design.com/papers/CummingsSNUG2002SJ_FIFO1.pdf
template<typename Receiver = MemoryReceiverSample<u32>, typename DataType = u32, u32 ElementCountExponent = 2>
class DualClockFIFO final
{
    DEFAULT_DESTRUCT(DualClockFIFO);
    DELETE_CM(DualClockFIFO);
public:
    DualClockFIFO(Receiver* const parent, const u32 index = 0) noexcept
        : m_Parent(parent)
        , m_Index(index)
        , m_WritePointer(this)
        , m_ReadPointer(this)
        , m_Memory(this)
        , m_SyncReadToWrite(this, 0)
        , m_SyncWriteToRead(this, 1)
    { }

    void SetWriteResetN(const bool reset_n) noexcept
    {
        m_WritePointer.SetWriteResetN(reset_n);
        m_SyncReadToWrite.SetResetN(reset_n);
    }

    void SetWriteClock(const bool clock) noexcept
    {
        // Memory has to tick before WritePointer to ensure the propagation order of the parameters is correct.
        m_Memory.SetWriteClock(clock);
        m_WritePointer.SetWriteClock(clock);
        m_SyncReadToWrite.SetClock(clock);
    }

    void SetWriteData(const DataType& writeData) noexcept
    {
        m_Memory.SetWriteData(writeData);
    }

    void SetWriteIncoming(const bool writeIncoming) noexcept
    {
        m_WritePointer.SetWriteIncoming(writeIncoming);
        m_Memory.SetWriteClockEnable(writeIncoming);
    }

    void SetReadResetN(const bool reset_n) noexcept
    {
        m_ReadPointer.SetReadResetN(reset_n);
        m_SyncWriteToRead.SetResetN(reset_n);
    }

    void SetReadClock(const bool clock) noexcept
    {
        m_ReadPointer.SetReadClock(clock);
        m_SyncWriteToRead.SetClock(clock);
    }

    void SetReadIncoming(const bool readIncoming) noexcept
    {
        m_ReadPointer.SetReadIncoming(readIncoming);
    }
public:
    void ReceiveWritePointer_WriteFull(const u32 index, const bool writeFull) noexcept
    {
        (void) index;
        m_Memory.SetWriteFull(writeFull);
        m_Parent->ReceiveDualClockFIFO_WriteFull(m_Index, writeFull);
    }

    void ReceiveWritePointer_WritePointer(const u32 index, const u64 writePointer) noexcept
    {
        (void) index;
        m_SyncWriteToRead.SetPointer(writePointer);
    }

    void ReceiveWritePointer_WriteAddress(const u32 index, const u64 writeAddress) noexcept
    {
        (void) index;
        m_Memory.SetWriteAddress(writeAddress);
        m_Parent->ReceiveDualClockFIFO_WriteAddress(m_Index, writeAddress);
    }

    void ReceiveReadPointer_ReadEmpty(const u32 index, const bool readEmpty) noexcept
    {
        (void) index;
        m_Parent->ReceiveDualClockFIFO_ReadEmpty(m_Index, readEmpty);
    }

    void ReceiveReadPointer_ReadPointer(const u32 index, const u64 readPointer) noexcept
    {
        (void) index;
        m_SyncReadToWrite.SetPointer(readPointer);
    }

    void ReceiveReadPointer_ReadAddress(const u32 index, const u64 readAddress) noexcept
    {
        (void) index;
        m_Memory.SetReadAddress(readAddress);
        m_Parent->ReceiveDualClockFIFO_ReadAddress(m_Index, readAddress);
    }

    void ReceiveMemory_ReadData(const u32 index, const DataType& data) noexcept
    {
        (void) index;
        m_Parent->ReceiveDualClockFIFO_ReadData(m_Index, data);
    }

    void ReceiveSynchronizer_Pointer(const u32 index, const u64 pointer) noexcept
    {
        if(index == 0)
        {
            m_WritePointer.SetWriteClockReadAddress(pointer);
        }
        else if(index == 1)
        {
            m_ReadPointer.SetReadClockWriteAddress(pointer);
        }
    }
private:
    Receiver* m_Parent;
    u32 m_Index;

    WritePointer<DualClockFIFO, ElementCountExponent> m_WritePointer;
    ReadPointer<DualClockFIFO, ElementCountExponent> m_ReadPointer;
    Memory<DualClockFIFO, DataType, ElementCountExponent> m_Memory;
    Synchronizer<DualClockFIFO, ElementCountExponent> m_SyncReadToWrite;
    Synchronizer<DualClockFIFO, ElementCountExponent> m_SyncWriteToRead;
};

}
