#pragma once

#include <Common.hpp>

namespace riscv {

// Mostly just implemented from http://www.sunburst-design.com/papers/CummingsSNUG2002SJ_FIFO1.pdf
template<typename DataType = u32, u32 ElementCountExponent = 2, bool SyncRead = false, bool Safe = false>
class FIFO final
{
    DEFAULT_DESTRUCT(FIFO);
    DELETE_CM(FIFO);
private:
    SENSITIVITY_DECL(p_Reset_n, p_Clock, Level);

    SIGNAL_ENTITIES();
public:
    static inline constexpr u32 ElementCount = 1 << ElementCountExponent;
public:
    FIFO() noexcept
        : p_Reset_n(0)
        , p_Clock(0)
        , p_Clear(0)
        , p_WriteEnable(0)
        , p_ReadEnable(0)
        , p_out_HalfFull(0)
        , p_out_Free(0)
        , p_out_Available(0)
        , m_Pad0(0)
        , p_out_Level(0)
        , p_WriteData{ }
        , p_out_ReadData{ }
        , m_Memory{ }
        , m_WritePointer(0)
        , m_ReadPointer(0)
        , m_ReadPointer0(0)
        , m_Pad1(0)
    { }

    void SetResetN(const bool reset_n) noexcept
    {
        p_Reset_n = BOOL_TO_BIT(reset_n);

        TRIGGER_SENSITIVITY(p_Reset_n);
    }

    void SetClock(const bool clock) noexcept
    {
        p_Clock = BOOL_TO_BIT(clock);

        TRIGGER_SENSITIVITY(p_Clock);
    }

    void SetClear(const bool clear) noexcept
    {
        p_Clear = clear;
    }

    void SetWriteEnable(const bool writeEnable) noexcept
    {
        p_WriteEnable = writeEnable;
    }

    void SetReadEnable(const bool readEnable) noexcept
    {
        p_ReadEnable = readEnable;
    }

    void SetWriteData(const DataType writeData) noexcept
    {
        p_WriteData = writeData;
    }

    [[nodiscard]] DataType GetReadData() const noexcept
    {
        if constexpr(ElementCount == 1)
        {
            return m_Memory[0];
        }
        else
        {
            if constexpr(!SyncRead)
            {
                return m_Memory[m_ReadPointer0];
            }
            else
            {
                return p_out_ReadData;
            }
        }
    }

    [[nodiscard]] bool GetHalfFull() const noexcept
    {
        if constexpr(!SyncRead)
        {
            return Half();
        }
        else
        {
            return p_out_HalfFull;
        }
    }

    [[nodiscard]] bool GetFree() const noexcept
    {
        if constexpr(!SyncRead)
        {
            return Free();
        }
        else
        {
            return p_out_Free;
        }
    }

    [[nodiscard]] bool GetAvailable() const noexcept
    {
        if constexpr(!SyncRead)
        {
            return Available();
        }
        else
        {
            return p_out_Available;
        }
    }
private:
    // Muxes

    [[nodiscard]] bool ReadEnable() const noexcept
    {
        if constexpr(Safe)
        {
            return BIT_TO_BOOL(p_ReadEnable);
        }
        else
        {
            return BIT_TO_BOOL(p_ReadEnable) && Available();
        }
    }

    [[nodiscard]] bool WriteEnable() const noexcept
    {
        if constexpr(Safe)
        {
            return BIT_TO_BOOL(p_WriteEnable);
        }
        else
        {
            return BIT_TO_BOOL(p_WriteEnable) && Free();
        }
    }

    [[nodiscard]] u32 NextWrite() const noexcept
    {
        if(BIT_TO_BOOL(p_Clear))
        {
            return 0;
        }
        else if(WriteEnable())
        {
            return m_WritePointer + 1;
        }
        else
        {
            return m_WritePointer;
        }
    }

    [[nodiscard]] u32 NextRead() const noexcept
    {
        if(BIT_TO_BOOL(p_Clear))
        {
            return 0;
        }
        else if(ReadEnable())
        {
            return m_ReadPointer + 1;
        }
        else
        {
            return m_ReadPointer;
        }
    }

    [[nodiscard]] bool Match() const noexcept
    {
        if constexpr(ElementCount == 1)
        {
            return (m_WritePointer & 1) == (m_ReadPointer & 1);
        }
        else
        {
            constexpr u32 BitMask = (1 << (ElementCountExponent - 1)) - 1;

            return (m_WritePointer & BitMask) == (m_ReadPointer & BitMask);
        }
    }

    [[nodiscard]] bool Full() const noexcept
    {
        if constexpr(ElementCount == 1)
        {
            return !Match();
        }
        else
        {
            constexpr u32 BitMask = (1 << (ElementCountExponent - 1));

            return Match() && ((m_WritePointer & BitMask) != (m_ReadPointer & BitMask));
        }
    }

    [[nodiscard]] bool Free() const noexcept
    {
        return !Full();
    }

    [[nodiscard]] bool Empty() const noexcept
    {
        if constexpr(ElementCount == 1)
        {
            return Match();
        }
        else
        {
            constexpr u32 BitMask = (1 << (ElementCountExponent - 1));

            return Match() && ((m_WritePointer & BitMask) == (m_ReadPointer & BitMask));
        }
    }

    [[nodiscard]] bool Available() const noexcept
    {
        return !Empty();
    }

    [[nodiscard]] u32 Level() const noexcept
    {
        if constexpr(ElementCount == 1)
        {
            return BOOL_TO_BIT(Full());
        }
        else
        {
            return m_WritePointer - m_ReadPointer;
        }
    }

    [[nodiscard]] bool Half() const noexcept
    {
        if constexpr(ElementCount == 1)
        {
            return Full();
        }
        else
        {
            constexpr u32 BitShift = (ElementCountExponent - 1);
            constexpr u32 TargetBit = 1 << BitShift;

            return ((Level() & TargetBit) >> BitShift) || Full();
        }
    }
private:
    PROCESSES_DECL()
    {
        PROCESS_ENTER(PointersHandler, p_Reset_n, p_Clock);
        PROCESS_ENTER(LevelHandler, Level);
        PROCESS_ENTER(WriteHandler, p_Reset_n, p_Clock);
        PROCESS_ENTER(ReadHandler, p_Reset_n, p_Clock);

        if constexpr(SyncRead)
        {
            PROCESS_ENTER(SyncStatusHandler, p_Reset_n, p_Clock);
        }
    }

    PROCESS_DECL(PointersHandler)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_WritePointer = 0;
            m_ReadPointer = 0;

            TRIGGER_SENSITIVITY(Level);
        }
        else if(RISING_EDGE(p_Clock))
        {
            m_WritePointer = NextWrite();
            m_ReadPointer = NextRead();

            TRIGGER_SENSITIVITY(Level);
        }
    }

    PROCESS_DECL(LevelHandler)
    {
        p_out_Level = 0;
        p_out_Level = Level();
    }

    PROCESS_DECL(WriteHandler)
    {
        if constexpr(ElementCount == 1)
        {
            if(!BIT_TO_BOOL(p_Reset_n))
            {
                m_Memory[0] = DataType{};
            }
            else if(RISING_EDGE(p_Clock))
            {
                if(WriteEnable())
                {
                    m_Memory[0] = p_WriteData;
                }
            }
        }
        else
        {
            if(!BIT_TO_BOOL(p_Reset_n))
            {
                for(u32 i = 0; i < ElementCount; ++i)
                {
                    m_Memory[i] = DataType {};
                }
            }
            else if(RISING_EDGE(p_Clock))
            {
                if(WriteEnable())
                {
                    m_Memory[m_WritePointer] = p_WriteData;
                }
            }
        }
    }

    PROCESS_DECL(ReadHandler)
    {
        if constexpr(SyncRead)
        {
            if constexpr(ElementCount > 1)
            {
                if(RISING_EDGE(p_Clock))
                {
                    m_ReadPointer0 = NextRead();
                }
            }
        }
        else
        {
            if constexpr(ElementCount > 1)
            {
                if(RISING_EDGE(p_Clock))
                {
                    p_out_ReadData = m_Memory[m_ReadPointer];
                }
            }
        }
    }

    PROCESS_DECL(SyncStatusHandler)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            p_out_HalfFull = 0;
            p_out_Free = 0;
            p_out_Available = 0;
        }
        else if(RISING_EDGE(p_Clock))
        {
            p_out_HalfFull = Half();
            p_out_Free = Free();
            p_out_Available = Available();
        }
    }
private:
    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    u32 p_Clear : 1;
    u32 p_WriteEnable : 1;
    u32 p_ReadEnable : 1;
    u32 p_out_HalfFull : 1;
    u32 p_out_Free : 1;
    u32 p_out_Available : 1;
    u32 m_Pad0 : 24;

    u32 p_out_Level;
    DataType p_WriteData;
    DataType p_out_ReadData;

    DataType m_Memory[ElementCount];
    u32 m_WritePointer : ElementCountExponent + 1;
    u32 m_ReadPointer : ElementCountExponent + 1;
    u32 m_ReadPointer0 : ElementCountExponent + 1;
    u32 m_Pad1 : 32 - 3 * (ElementCountExponent + 1);
};

}
