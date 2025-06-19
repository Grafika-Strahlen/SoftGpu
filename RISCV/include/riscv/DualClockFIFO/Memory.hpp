#pragma once

#include <Common.hpp>

namespace riscv::fifo {

template<typename DataType>
class MemoryReceiverSample
{
public:
    void ReceiveMemory_ReadData(const u32 index, const DataType& data) noexcept { }
};

template<typename Receiver = MemoryReceiverSample<u32>, typename DataType = u32, u32 ElementCountExponent = 2>
class Memory final
{
    DEFAULT_DESTRUCT(Memory);
    DELETE_CM(Memory);
private:
    SENSITIVITY_DECL(p_WriteClock);

    SIGNAL_ENTITIES();
public:
    static inline constexpr u32 ElementCount = 1 << ElementCountExponent;
public:
    Memory(Receiver* const parent, const u32 index = 0) noexcept
        : m_Parent(parent)
        , m_Index(index)
        , p_WriteClock(0)
        , p_WriteClockEnable(0)
        , p_WriteFull(0)
        , m_Pad0(0)
        , p_WriteAddress(0)
        , p_ReadAddress(0)
        , m_Pad1(0)
        , p_WriteData{}
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

        m_Parent->ReceiveMemory_ReadData(m_Index, m_RamBlock[p_ReadAddress]);
    }

    void SetWriteData(const DataType& writeData) noexcept
    {
        p_WriteData = writeData;
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

                // This is triggered outside of a process anytime the memory changes,
                // Thus we know that it only changes when the write address and read address
                // are equal. Or when read address changes.
                if(p_WriteAddress == p_ReadAddress)
                {
                    m_Parent->ReceiveMemory_ReadData(m_Index, m_RamBlock[p_ReadAddress]);
                }
            }
        }
    }

private:
    Receiver* m_Parent;
    u32 m_Index;

    u32 p_WriteClock : 1;
    u32 p_WriteClockEnable : 1;
    u32 p_WriteFull : 1;
    u32 m_Pad0 : 29;

    u64 p_WriteAddress : ElementCountExponent;
    u64 p_ReadAddress : ElementCountExponent;
    u64 m_Pad1 : 64 - 2 * (ElementCountExponent);
    DataType p_WriteData;
    DataType m_RamBlock[ElementCount];
    u8 OverFlow0 = 0xCC;
    u8 OverFlow1 = 0xCC;
    u8 OverFlow2 = 0xCC;
    u8 OverFlow3 = 0xCC;
    u8 OverFlow4 = 0xCC;
    u8 OverFlow5 = 0xCC;
    u8 OverFlow6 = 0xCC;
    u8 OverFlow7 = 0xCC;
    u8 OverFlow8 = 0xCC;
    u8 OverFlow9 = 0xCC;
    u8 OverFlowA = 0xCC;
    u8 OverFlowB = 0xCC;
    u8 OverFlowC = 0xCC;
    u8 OverFlowD = 0xCC;
    u8 OverFlowE = 0xCC;
    u8 OverFlowF = 0xCC;
};

}
