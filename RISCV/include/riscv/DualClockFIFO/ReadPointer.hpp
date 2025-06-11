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
    /* ReSharper disable once CppInconsistentNaming */
    SENSITIVITY_DECL(p_ReadReset_n, p_ReadClock, ReadGrayNext, p_ReadClockWritePointer);

    SIGNAL_ENTITIES();
public:
    static inline constexpr u32 ElementCount = 1 << ElementCountExponent;
public:
    ReadPointer(Receiver* const parent, const u32 index = 0) noexcept
        : m_Parent(parent)
        , m_Index(0)
        , p_ReadReset_n(0)
        , p_ReadClock(0)
        , p_ReadIncoming(0)
        , m_ReadEmptyIntermediate0(0)
        , m_ReadEmptyIntermediate1(0)
        , m_Pad0(0)
        , p_out_ReadPointer(0)
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

        // p_WriteIncrement is one of the values that affects ReadGrayNext
        // TRIGGER_SENSITIVITY(ReadGrayNext);
    }

    void SetReadClockWriteAddress(const u64 readClockWriteAddress) noexcept
    {
        p_ReadClockWritePointer = readClockWriteAddress;

        // TRIGGER_SENSITIVITY(p_ReadClockWritePointer);
    }
private:
    void SetReadBin(const u64 readBin) noexcept
    {
        m_ReadBin = readBin;

        // m_WriteBin is one of the values that affects WriteGrayNext
        // TRIGGER_SENSITIVITY(ReadGrayNext);

        m_Parent->ReceiveReadPointer_ReadAddress(m_Index, m_ReadBin & ~(1 << ElementCountExponent));
    }

    void SetReadEmptyIntermediate1(const bool readEmptyIntermediate1) noexcept
    {
        m_ReadEmptyIntermediate1 = BOOL_TO_BIT(readEmptyIntermediate1);

        // m_WriteFullIntermediate1 is one of the values that affects WriteGrayNext
        // TRIGGER_SENSITIVITY(ReadGrayNext);

        m_Parent->ReceiveReadPointer_ReadEmpty(m_Index, readEmptyIntermediate1);
    }

    [[nodiscard]] bool ReadIncomingNotEmpty() const noexcept
    {
        return BIT_TO_BOOL(p_ReadIncoming) && !BIT_TO_BOOL(m_ReadEmptyIntermediate1);
    }

    [[nodiscard]] u64 ReadBinNext() const noexcept
    {
        return m_ReadBin + BOOL_TO_BIT(ReadIncomingNotEmpty());
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
        // PROCESS_ENTER(EmptyIntermediate0Handler, ReadGrayNext, p_ReadClockWritePointer);
        // PROCESS_ENTER(EmptyIntermediate1Handler, p_ReadClock, p_ReadReset_n);
    }

    PROCESS_DECL(BinPortHandler)
    {
        if(!BIT_TO_BOOL(p_ReadReset_n))
        {
            SetReadBin(0);
            m_Parent->ReceiveReadPointer_ReadPointer(m_Index, 0);
            SetReadEmptyIntermediate1(true);
        }
        else if(RISING_EDGE(p_ReadClock))
        {
            const u64 readBinNext = ReadBinNext();
            const u64 readGrayNext = ReadGrayNext();
            const bool empty = GetReadEmpty();

            SetReadBin(readBinNext);
            m_Parent->ReceiveReadPointer_ReadPointer(m_Index, readGrayNext);
            SetReadEmptyIntermediate1(empty);
        }
    }

    // PROCESS_DECL(EmptyIntermediate0Handler)
    // {
    //     m_ReadEmptyIntermediate0 = BOOL_TO_BIT(ReadGrayNext() == p_ReadClockWritePointer);
    // }
    //
    // PROCESS_DECL(EmptyIntermediate1Handler)
    // {
    //     if(!BIT_TO_BOOL(p_ReadReset_n))
    //     {
    //         SetReadEmptyIntermediate1(true);
    //     }
    //     else if(RISING_EDGE(p_ReadClock))
    //     {
    //         SetReadEmptyIntermediate1(m_ReadEmptyIntermediate0);
    //     }
    // }
private:
    Receiver* m_Parent;
    u32 m_Index;

    u32 p_ReadReset_n : 1;
    u32 p_ReadClock : 1;
    u32 p_ReadIncoming : 1;
    u32 m_ReadEmptyIntermediate0 : 1;
    u32 m_ReadEmptyIntermediate1 : 1;
    u32 m_Pad0 : 27;

    u64 p_out_ReadPointer : ElementCountExponent + 1;
    u64 p_ReadClockWritePointer : ElementCountExponent + 1;
    u64 m_ReadBin : ElementCountExponent + 1;
    u64 m_Pad1 : 64 - 3 * (ElementCountExponent + 1);
};

}
