#pragma once

#include <Common.hpp>

namespace riscv::fifo {

class ReadPointerReceiverSample
{
    void ReceiveReadPointer_ReadEmpty(const u32 index, const bool readEmpty) noexcept { }
    void ReceiveReadPointer_ReadPointer(const u32 index, const u64 readPointer) noexcept { }
    void ReceiveReadPointer_ReadAddress(const u32 index, const u64 readAddress) noexcept { }
};

template<typename Receiver = ReadPointerReceiverSample, u32 ElementCountExponent = 2>
class ReadPointer final
{
    DEFAULT_DESTRUCT(ReadPointer);
    DELETE_CM(ReadPointer);
private:
    SENSITIVITY_DECL(p_ReadReset_n, p_ReadClock);

    SIGNAL_ENTITIES();
public:
    ReadPointer(Receiver* const parent, const u32 index = 0) noexcept
        : m_Parent(parent)
        , m_Index(index)
        , p_ReadReset_n(0)
        , p_ReadClock(0)
        , p_ReadIncoming(0)
        , m_ReadEmptyIntermediate(0)
        , m_Pad0(0)
        , p_ReadClockWritePointer(0)
        , m_ReadBin(0)
        , m_Pad1(0)
    { }

    void SetReadResetN(const bool reset_n) noexcept
    {
        p_ReadReset_n = BOOL_TO_BIT(reset_n);

        TRIGGER_SENSITIVITY(p_ReadReset_n);
    }

    void SetReadClock(const bool clock) noexcept
    {
        p_ReadClock = BOOL_TO_BIT(clock);

        TRIGGER_SENSITIVITY(p_ReadClock);
    }

    void SetReadIncoming(const bool readIncoming) noexcept
    {
        p_ReadIncoming = readIncoming;
    }

    void SetReadClockWriteAddress(const u64 readClockWriteAddress) noexcept
    {
        p_ReadClockWritePointer = readClockWriteAddress;
    }
private:
    void SetReadBin(const u64 readBin) noexcept
    {
        m_ReadBin = readBin;

        m_Parent->ReceiveReadPointer_ReadAddress(m_Index, m_ReadBin & ((1 << ElementCountExponent) - 1));
    }

    void SetReadEmptyIntermediate(const bool readEmpty) noexcept
    {
        m_ReadEmptyIntermediate = BOOL_TO_BIT(readEmpty);

        m_Parent->ReceiveReadPointer_ReadEmpty(m_Index, readEmpty);
    }

    [[nodiscard]] bool ReadIncomingNotEmpty() const noexcept
    {
        return BIT_TO_BOOL(p_ReadIncoming) && !BIT_TO_BOOL(m_ReadEmptyIntermediate);
    }

    [[nodiscard]] u64 ReadBinNext() const noexcept
    {
        return (m_ReadBin + BOOL_TO_BIT(ReadIncomingNotEmpty())) & ((1 << (ElementCountExponent + 1)) - 1);
    }

    [[nodiscard]] u64 ReadGrayNext() const noexcept
    {
        const u64 readBinNext = ReadBinNext();
        return (readBinNext >> 1) ^ readBinNext;
    }

    [[nodiscard]] bool GetReadEmpty() const noexcept
    {
        return ReadGrayNext() == p_ReadClockWritePointer;
    }
private:
    PROCESSES_DECL()
    {
        PROCESS_ENTER(BinPortHandler, p_ReadClock, p_ReadReset_n);
    }

    PROCESS_DECL(BinPortHandler)
    {
        if(!BIT_TO_BOOL(p_ReadReset_n))
        {
            SetReadBin(0);
            m_Parent->ReceiveReadPointer_ReadPointer(m_Index, 0);
            SetReadEmptyIntermediate(true);
        }
        else if(RISING_EDGE(p_ReadClock))
        {
            const u64 readBinNext = ReadBinNext();
            const u64 readGrayNext = ReadGrayNext();
            const bool empty = GetReadEmpty();

            SetReadBin(readBinNext);
            m_Parent->ReceiveReadPointer_ReadPointer(m_Index, readGrayNext);
            SetReadEmptyIntermediate(empty);
        }
    }
private:
    Receiver* m_Parent;
    u32 m_Index;

    u32 p_ReadReset_n : 1;
    u32 p_ReadClock : 1;
    u32 p_ReadIncoming : 1;
    u32 m_ReadEmptyIntermediate : 1;
    u32 m_Pad0 : 28;

    u64 p_ReadClockWritePointer : ElementCountExponent + 1;
    u64 m_ReadBin : ElementCountExponent + 1;
    u64 m_Pad1 : 64 - 2 * (ElementCountExponent + 1);
};

}
