#pragma once

#include <Common.hpp>

namespace riscv::fifo {

template<u32 ElementCountExponent = 2>
class WritePointer final
{
    DEFAULT_DESTRUCT(WritePointer);
    DELETE_CM(WritePointer);
private:
    /* ReSharper disable once CppInconsistentNaming */
    SENSITIVITY_DECL(p_WriteReset_n, p_WriteClock, WriteGrayNext, p_WriteClockReadPointer);

    SIGNAL_ENTITIES();
public:
    static inline constexpr u32 ElementCount = 1 << ElementCountExponent;
public:
    WritePointer() noexcept
        : p_WriteReset_n(0)
        , p_WriteClock(0)
        , p_WriteIncoming(0)
        , m_WriteFullIntermediate0(0)
        , m_WriteFullIntermediate1(0)
        , m_Pad0(0)
        , p_out_WritePointer(0)
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

        // p_WriteIncrement is one of the values that affects WriteGrayNext
        TRIGGER_SENSITIVITY(WriteGrayNext);
    }

    void SetWriteClockReadAddress(const u64 writeClockReadAddress) noexcept
    {
        p_WriteClockReadPointer = writeClockReadAddress;

        TRIGGER_SENSITIVITY(p_WriteClockReadPointer);
    }

    [[nodiscard]] bool GetWriteFull() const noexcept
    {
        return m_WriteFullIntermediate1;
    }

    [[nodiscard]] u64 GetWriteAddress() const noexcept
    {
        return m_WriteBin & ~(1 << ElementCountExponent);
    }
private:
    void SetWriteBin(const u64 writeBin) noexcept
    {
        m_WriteBin = writeBin;

        // m_WriteBin is one of the values that affects WriteGrayNext
        TRIGGER_SENSITIVITY(WriteGrayNext);
    }

    void SetWriteFullIntermediate1(const bool writeFullIntermediate1) noexcept
    {
        m_WriteFullIntermediate1 = BOOL_TO_BIT(writeFullIntermediate1);

        // m_WriteFullIntermediate1 is one of the values that affects WriteGrayNext
        TRIGGER_SENSITIVITY(WriteGrayNext);
    }

    [[nodiscard]] bool WriteIncrementNotFull() const noexcept
    {
        return BIT_TO_BOOL(p_WriteIncoming) && !BIT_TO_BOOL(m_WriteFullIntermediate1);
    }

    [[nodiscard]] u64 WriteBinNext() const noexcept
    {
        return m_WriteBin + BOOL_TO_BIT(WriteIncrementNotFull());
    }

    [[nodiscard]] u64 WriteGrayNext() const noexcept
    {
        const u64 writeBinNext = WriteBinNext();
        return (writeBinNext >> 1) ^ writeBinNext;
    }
private:
    PROCESSES_DECL()
    {
        PROCESS_ENTER(BinPortHandler, p_WriteClock, p_WriteReset_n);
        PROCESS_ENTER(FullIntermediate0Handler, WriteGrayNext, p_WriteClockReadPointer);
        PROCESS_ENTER(FullIntermediate1Handler, p_WriteClock, p_WriteReset_n);
    }

    PROCESS_DECL(BinPortHandler)
    {
        if(!BIT_TO_BOOL(p_WriteReset_n))
        {
            SetWriteBin(0);
            p_out_WritePointer = 0;
        }
        else if(RISING_EDGE(p_WriteClock))
        {
            SetWriteBin(WriteBinNext());
            p_out_WritePointer = WriteGrayNext();
        }
    }

    PROCESS_DECL(FullIntermediate0Handler)
    {
        const u64 upperBitsFlipped = (p_WriteClockReadPointer >> (ElementCountExponent - 1)) ^ 0x3;
        const u64 readPointer = p_WriteClockReadPointer & ~(0x3 << (ElementCountExponent - 1));
        const u64 grayPointer = (upperBitsFlipped << (ElementCountExponent - 1)) | readPointer;

        m_WriteFullIntermediate0 = BOOL_TO_BIT(WriteGrayNext() == grayPointer);
    }

    PROCESS_DECL(FullIntermediate1Handler)
    {
        if(!BIT_TO_BOOL(p_WriteReset_n))
        {
            SetWriteFullIntermediate1(false);
        }
        else if(RISING_EDGE(p_WriteClock))
        {
            SetWriteFullIntermediate1(m_WriteFullIntermediate0);
        }
    }
private:
    u32 p_WriteReset_n : 1;
    u32 p_WriteClock : 1;
    u32 p_WriteIncoming : 1;
    u32 m_WriteFullIntermediate0 : 1;
    u32 m_WriteFullIntermediate1 : 1;
    u32 m_Pad0 : 27;

    u64 p_out_WritePointer : ElementCountExponent + 1;
    u64 p_WriteClockReadPointer : ElementCountExponent + 1;
    u64 m_WriteBin : ElementCountExponent + 1;
    u64 m_Pad1 : 64 - 3 * (ElementCountExponent + 1);
};

}
