#pragma once

#include <Common.hpp>

#include "ReadPointer.hpp"
#include "WritePointer.hpp"
#include "Memory.hpp"
#include "Synchronizer.hpp"


namespace riscv::fifo {

template<u32 DataBits, u32 ElementCountExponent = 2>
class DualClockFIFO final
{
    DEFAULT_DESTRUCT(DualClockFIFO);
    DELETE_CM(DualClockFIFO);
public:
    DualClockFIFO() noexcept
        : m_WritePointer()
        , m_ReadPointer()
        , m_Memory()
        , m_SyncReadToWrite()
        , m_SyncWriteToRead()
    { }

    void SetWriteResetN(const bool reset_n) noexcept
    {
        m_WritePointer.SetWriteResetN(reset_n);
        m_SyncReadToWrite.SetResetN(reset_n);
    }

    void SetWriteClock(const bool clock) noexcept
    {
        m_WritePointer.SetWriteClock(clock);

        // WriteFull is set on a clock process, so we can propagate it here.
        m_Memory.SetWriteFull(m_WritePointer.GetWriteFull());

        m_Memory.SetWriteClock(clock);
        m_SyncReadToWrite.SetClock(clock);
    }

    void SetWriteData(const BitSetC<DataBits>& writeData) noexcept
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
        m_ReadPointer.SetWriteResetN(reset_n);
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

    [[nodiscard]] BitSetC<DataBits> GetReadData() const noexcept
    {
        return m_Memory.GetReadData();
    }

    [[nodiscard]] bool GetWriteFull() const noexcept
    {
        return m_WritePointer.GetWriteFull();
    }

    [[nodiscard]] bool GetReadEmpty() const noexcept
    {
        return m_ReadPointer.GetEmpty();
    }

    [[nodiscard]] u64 GetWriteAddress() const noexcept
    {
        return m_WritePointer.GetWriteAddress();
    }

    [[nodiscard]] u64 GetReadAddress() const noexcept
    {
        return m_ReadPointer.GetReadAddress();
    }
private:
    WritePointer<ElementCountExponent> m_WritePointer;
    ReadPointer<ElementCountExponent> m_ReadPointer;
    Memory<DataBits, ElementCountExponent> m_Memory;
    Synchronizer<ElementCountExponent> m_SyncReadToWrite;
    Synchronizer<ElementCountExponent> m_SyncWriteToRead;
};

}
