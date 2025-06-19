#pragma once

#include <Common.hpp>

namespace riscv::fifo {

class WritePointerReceiverSample
{
public:
    void ReceiveWritePointer_WriteFull(const u32 index, const bool writeFull) noexcept { }
    void ReceiveWritePointer_WritePointer(const u32 index, const u64 writePointer) noexcept { }
    void ReceiveWritePointer_WriteAddress(const u32 index, const u64 writeAddress) noexcept { }
};

template<typename Receiver = WritePointerReceiverSample, u32 ElementCountExponent = 2>
class WritePointer final
{
    DEFAULT_DESTRUCT(WritePointer);
    DELETE_CM(WritePointer);
private:
    SENSITIVITY_DECL(p_WriteReset_n, p_WriteClock);

    SIGNAL_ENTITIES();
public:
    WritePointer(Receiver* const parent, const u32 index = 0) noexcept
        : m_Parent(parent)
        , m_Index(index)
        , p_WriteReset_n(0)
        , p_WriteClock(0)
        , p_WriteIncoming(0)
        , m_WriteFullIntermediate(0)
        , m_Pad0(0)
        , p_WriteClockReadPointer(0)
        , m_WriteBin(0)
        , m_Pad1(0)
    { }

    void SetWriteResetN(const bool reset_n) noexcept
    {
        p_WriteReset_n = BOOL_TO_BIT(reset_n);

        TRIGGER_SENSITIVITY(p_WriteReset_n);
    }

    void SetWriteClock(const bool clock) noexcept
    {
        p_WriteClock = BOOL_TO_BIT(clock);

        TRIGGER_SENSITIVITY(p_WriteClock);
    }

    void SetWriteIncoming(const bool writeIncoming) noexcept
    {
        p_WriteIncoming = writeIncoming;
    }

    void SetWriteClockReadAddress(const u64 writeClockReadAddress) noexcept
    {
        p_WriteClockReadPointer = writeClockReadAddress;
    }
private:
    void SetWriteBin(const u64 writeBin) noexcept
    {
        m_WriteBin = writeBin;

        m_Parent->ReceiveWritePointer_WriteAddress(m_Index, m_WriteBin & ((1 << ElementCountExponent) - 1));
    }

    void SetWriteFullIntermediate(const bool writeFullIntermediate) noexcept
    {
        m_WriteFullIntermediate = BOOL_TO_BIT(writeFullIntermediate);
    
        m_Parent->ReceiveWritePointer_WriteFull(m_Index, writeFullIntermediate);
    }

    [[nodiscard]] bool WriteIncomingNotFull() const noexcept
    {
        return BIT_TO_BOOL(p_WriteIncoming) && !BIT_TO_BOOL(m_WriteFullIntermediate);
    }

    [[nodiscard]] u64 WriteBinNext() const noexcept
    {
        return (m_WriteBin + BOOL_TO_BIT(WriteIncomingNotFull())) & ((1 << (ElementCountExponent + 1)) - 1);
    }

    [[nodiscard]] u64 WriteGrayNext() const noexcept
    {
        const u64 writeBinNext = WriteBinNext();
        return (writeBinNext >> 1) ^ writeBinNext;
    }

    [[nodiscard]] bool GetWriteFullIntermediate0() const noexcept
    {
        const u64 upperBitsFlipped = (p_WriteClockReadPointer >> (ElementCountExponent - 1)) ^ 0x3;
        const u64 readPointer = p_WriteClockReadPointer & ~(0x3 << (ElementCountExponent - 1));
        const u64 grayPointer = (upperBitsFlipped << (ElementCountExponent - 1)) | readPointer;

        return WriteGrayNext() == grayPointer;
    }
private:
    PROCESSES_DECL()
    {
        PROCESS_ENTER(BinPortHandler, p_WriteClock, p_WriteReset_n);
    }

    PROCESS_DECL(BinPortHandler)
    {
        if(!BIT_TO_BOOL(p_WriteReset_n))
        {
            SetWriteBin(0);
            m_Parent->ReceiveWritePointer_WritePointer(m_Index, 0);
            SetWriteFullIntermediate(true);
        }
        else if(RISING_EDGE(p_WriteClock))
        {
            const u64 writeBinNext = WriteBinNext();
            const u64 writeGrayNext = WriteGrayNext();
            const bool full = GetWriteFullIntermediate0();

            SetWriteBin(writeBinNext);
            m_Parent->ReceiveWritePointer_WritePointer(m_Index, writeGrayNext);
            SetWriteFullIntermediate(full);
        }
    }
private:
    Receiver* m_Parent;
    u32 m_Index;

    u32 p_WriteReset_n : 1;
    u32 p_WriteClock : 1;
    u32 p_WriteIncoming : 1;
    u32 m_WriteFullIntermediate : 1;
    u32 m_Pad0 : 28;

    u64 p_WriteClockReadPointer : ElementCountExponent + 1;
    u64 m_WriteBin : ElementCountExponent + 1;
    u64 m_Pad1 : 64 - 2 * (ElementCountExponent + 1);
};

}
