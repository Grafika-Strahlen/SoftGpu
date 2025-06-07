#pragma once

#include <Common.hpp>
#include <BitSet.hpp>

namespace riscv::fifo {

template<u32 DataBits, u32 ElementCountExponent = 2>
class Memory final
{
    DEFAULT_DESTRUCT(Memory);
    DELETE_CM(Memory);
private:
    /* ReSharper disable once CppInconsistentNaming */
    SENSITIVITY_DECL(p_WriteClock);

    SIGNAL_ENTITIES();
public:
    static inline constexpr uSys DataBitsSys = DataBits;
    static inline constexpr u32 ElementCount = 1 << ElementCountExponent;
    using RawDataT = BitSetC<DataBitsSys>;
public:
    Memory() noexcept
        : p_WriteClock(0)
        , p_WriteClockEnable(0)
        , p_WriteFull(0)
        , m_Pad0(0)
        , p_WriteAddress(0)
        , p_ReadAddress(0)
        , m_Pad1(0)
        , p_WriteData(false)
        , p_out_ReadData(false)
        , m_RamBlock()
    { }

    void SetWriteClock(const bool clock) noexcept
    {
        p_WriteClock = BOOL_TO_BIT(clock);

        TRIGGER_SENSITIVITY(p_WriteClock);
    }

    void SetWriteClockEnable(const bool writeClockEnable) noexcept
    {
        p_WriteClockEnable = writeClockEnable;
    }

    void SetWriteFull(const bool writeFull) noexcept
    {
        p_WriteFull = writeFull;
    }

    void SetWriteAddress(const u64 writeAddress) noexcept
    {
        p_WriteAddress = writeAddress;
    }

    void SetReadAddress(const u64 readAddress) noexcept
    {
        p_ReadAddress = readAddress;
    }

    void SetWriteData(const BitSetC<DataBitsSys>& writeData) noexcept
    {
        p_WriteData = writeData;
    }

    [[nodiscard]] RawDataT GetReadData() const noexcept
    {
        return p_out_ReadData;
    }
private:
    [[nodiscard]] bool WriteEnable() const noexcept
    {
        return BIT_TO_BOOL(p_WriteClockEnable) && !BIT_TO_BOOL(p_WriteFull);
    }
private:
    PROCESSES_DECL()
    {
        PROCESS_ENTER(PortHandler, p_WriteClock);
    }

    PROCESS_DECL(PortHandler)
    {
        if(RISING_EDGE(p_WriteClock))
        {
            if(WriteEnable())
            {
                m_RamBlock[p_WriteAddress] = p_WriteData;
            }
        }

        p_out_ReadData = m_RamBlock[p_ReadAddress];
    }

private:
    u32 p_WriteClock : 1;
    u32 p_WriteClockEnable : 1;
    u32 p_WriteFull : 1;
    u32 m_Pad0 : 29;

    u64 p_WriteAddress : ElementCountExponent;
    u64 p_ReadAddress : ElementCountExponent;
    u64 m_Pad1 : 64 - 2 * (ElementCountExponent);
    RawDataT p_WriteData;
    RawDataT p_out_ReadData;
    RawDataT m_RamBlock[ElementCount];
};

}
