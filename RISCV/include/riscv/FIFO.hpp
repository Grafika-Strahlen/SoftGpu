/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <Common.hpp>

namespace riscv {

template<typename DataType>
class FIFOReceiverSample
{
public:
    void ReceiveFIFO_HalfFull(const u32 index, const bool halfFull) noexcept { }
    void ReceiveFIFO_Level(const u32 index, const u32 level) noexcept { }
    void ReceiveFIFO_Free(const u32 index, const bool free) noexcept { }
    void ReceiveFIFO_ReadData(const u32 index, const DataType& data) noexcept { }
    void ReceiveFIFO_Available(const u32 index, const bool available) noexcept { }
};

template<typename Receiver = FIFOReceiverSample<u32>, typename DataType = u32, u32 ElementCountExponent = 2, bool SyncRead = false, bool Safe = false, bool FullReset = false, bool ZeroOut = false>
class FIFO final
{
    DEFAULT_DESTRUCT(FIFO);
    DELETE_CM(FIFO);
private:
    SENSITIVITY_DECL(p_Reset_n, p_Clock, Level);

    SIGNAL_ENTITIES();
public:
    static inline constexpr u32 ElementCount = 1 << ElementCountExponent;
    static inline constexpr u32 PointerMask = (1 << ElementCountExponent) - 1;
public:
    FIFO(Receiver* const parent, const u32 index = 0) noexcept
        : m_Parent(parent)
        , m_Index(index)
        , p_Reset_n(0)
        , p_Clock(0)
        , p_Clear(0)
        , p_WriteEnable(0)
        , p_ReadEnable(0)
        , m_Pad0(0)
        , p_WriteData{ }
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
private:
    // Wires

    [[nodiscard]] bool ReadEnable() const noexcept
    {
        if constexpr(!Safe)
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
        if constexpr(!Safe)
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
            return (m_WritePointer & PointerMask) == (m_ReadPointer & PointerMask);
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
            constexpr u32 BitMask = (1 << ElementCountExponent);

            return Match() && ((m_WritePointer & BitMask) != (m_ReadPointer & BitMask));
        }
    }

    [[nodiscard]] bool Empty() const noexcept
    {
        if constexpr(ElementCount == 1)
        {
            return Match();
        }
        else
        {
            constexpr u32 BitMask = (1 << ElementCountExponent);

            return Match() && ((m_WritePointer & BitMask) == (m_ReadPointer & BitMask));
        }
    }

    [[nodiscard]] bool Free() const noexcept
    {
        return !Full();
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

            return BIT_TO_BOOL((Level() >> BitShift) & 0x1) || Full();
        }
    }
private:
    PROCESSES_DECL()
    {
        PROCESS_ENTER(PointersHandler, p_Reset_n, p_Clock);
        PROCESS_ENTER(LevelHandler, Level);

        if constexpr(FullReset)
        {
            PROCESS_ENTER(FullResetWriteHandler, p_Reset_n, p_Clock);
        }
        else
        {
            PROCESS_ENTER(NoResetWriteHandler, p_Clock);
        }

        if constexpr(SyncRead)
        {
            PROCESS_ENTER(SyncReadHandler, p_Clock);
        }
        else
        {
            PROCESS_ENTER(AsyncReadHandler, p_Clock);
        }

        if constexpr(SyncRead)
        {
            PROCESS_ENTER(SyncStatusHandler, p_Clock);
        }
    }

    PROCESS_DECL(PointersHandler)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_WritePointer = 0;
            m_ReadPointer = 0;

            if constexpr(!SyncRead)
            {
                // HalfFull Free, and Available are muxes in this case.
                m_Parent->ReceiveFIFO_HalfFull(m_Index, Half());
                m_Parent->ReceiveFIFO_Free(m_Index, Free());
                m_Parent->ReceiveFIFO_Available(m_Index, Available());
            }

            TRIGGER_SENSITIVITY(Level);
        }
        else if(RISING_EDGE(p_Clock))
        {
            m_WritePointer = NextWrite();
            m_ReadPointer = NextRead();

            if constexpr(!SyncRead)
            {
                // HalfFull Free, and Available are muxes in this case.
                m_Parent->ReceiveFIFO_HalfFull(m_Index, Half());
                m_Parent->ReceiveFIFO_Free(m_Index, Free());
                m_Parent->ReceiveFIFO_Available(m_Index, Available());
            }

            TRIGGER_SENSITIVITY(Level);
        }
    }

    PROCESS_DECL(LevelHandler)
    {
        m_Parent->ReceiveFIFO_Level(m_Index, Level());
    }

    PROCESS_DECL(FullResetWriteHandler)
    {
        if constexpr(FullReset)
        {
            if constexpr(ElementCount == 1)
            {
                if(!BIT_TO_BOOL(p_Reset_n))
                {
                    m_Memory[0] = DataType {};

                    // ReadData is a mux in this case.
                    m_Parent->ReceiveFIFO_ReadData(m_Index, m_Memory[0]);
                }
                else if(RISING_EDGE(p_Clock))
                {
                    if(WriteEnable())
                    {
                        m_Memory[0] = p_WriteData;

                        // ReadData is a wire in this case.
                        if constexpr(ZeroOut)
                        {
                            if(Available())
                            {
                                m_Parent->ReceiveFIFO_ReadData(m_Index, m_Memory[0]);
                            }
                            else
                            {
                                m_Parent->ReceiveFIFO_ReadData(m_Index, DataType {});
                            }
                        }
                        else
                        {
                            m_Parent->ReceiveFIFO_ReadData(m_Index, m_Memory[0]);
                        }
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
                        m_Memory[m_WritePointer & PointerMask] = p_WriteData;
                    }
                }
            }
        }
    }

    PROCESS_DECL(NoResetWriteHandler)
    {
        if constexpr(!FullReset)
        {
            if constexpr(ElementCount == 1)
            {
                if(RISING_EDGE(p_Clock))
                {
                    if(WriteEnable())
                    {
                        m_Memory[0] = p_WriteData;

                        // ReadData is a wire in this case.
                        if constexpr(ZeroOut)
                        {
                            if(Available())
                            {
                                m_Parent->ReceiveFIFO_ReadData(m_Index, m_Memory[0]);
                            }
                            else
                            {
                                m_Parent->ReceiveFIFO_ReadData(m_Index, DataType {});
                            }
                        }
                        else
                        {
                            m_Parent->ReceiveFIFO_ReadData(m_Index, m_Memory[0]);
                        }
                    }
                }
            }
            else
            {
                if(RISING_EDGE(p_Clock))
                {
                    if(WriteEnable())
                    {
                        m_Memory[m_WritePointer & PointerMask] = p_WriteData;
                    }
                }
            }
        }
    }

    PROCESS_DECL(SyncReadHandler)
    {
        if constexpr(SyncRead)
        {
            if constexpr(ElementCount > 1)
            {
                if(RISING_EDGE(p_Clock))
                {
                    if constexpr(ZeroOut)
                    {
                        if(Available())
                        {
                            m_Parent->ReceiveFIFO_ReadData(m_Index, m_Memory[m_ReadPointer & PointerMask]);
                        }
                        else
                        {
                            m_Parent->ReceiveFIFO_ReadData(m_Index, DataType {});
                        }
                    }
                    else
                    {
                        m_Parent->ReceiveFIFO_ReadData(m_Index, m_Memory[m_ReadPointer & PointerMask]);
                    }
                }
            }
        }
    }

    PROCESS_DECL(AsyncReadHandler)
    {
        if constexpr(!SyncRead)
        {
            if constexpr(ElementCount > 1)
            {
                if(RISING_EDGE(p_Clock))
                {
                    m_ReadPointer0 = NextRead();

                    // ReadData is a wire in this case.
                    if constexpr(ZeroOut)
                    {
                        if(Available())
                        {
                            m_Parent->ReceiveFIFO_ReadData(m_Index, m_Memory[m_ReadPointer0 & PointerMask]);
                        }
                        else
                        {
                            m_Parent->ReceiveFIFO_ReadData(m_Index, DataType {});
                        }
                    }
                    else
                    {
                        m_Parent->ReceiveFIFO_ReadData(m_Index, m_Memory[m_ReadPointer0 & PointerMask]);
                    }
                }
            }
        }
    }

    PROCESS_DECL(SyncStatusHandler)
    {
        if constexpr(SyncRead)
        {
            if(!BIT_TO_BOOL(p_Reset_n))
            {
                m_Parent->ReceiveFIFO_HalfFull(m_Index, 0);
                m_Parent->ReceiveFIFO_Free(m_Index, 0);
                m_Parent->ReceiveFIFO_Available(m_Index, 0);
            }
            else if(RISING_EDGE(p_Clock))
            {
                m_Parent->ReceiveFIFO_HalfFull(m_Index, Half());
                m_Parent->ReceiveFIFO_Free(m_Index, Free());
                m_Parent->ReceiveFIFO_Available(m_Index, Available());
            }
        }
    }
private:
    Receiver* m_Parent;
    u32 m_Index;

    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    u32 p_Clear : 1;
    u32 p_WriteEnable : 1;
    u32 p_ReadEnable : 1;
    u32 m_Pad0 : 27;

    DataType p_WriteData;

    DataType m_Memory[ElementCount];
    u64 m_WritePointer : ElementCountExponent + 1;
    u64 m_ReadPointer : ElementCountExponent + 1;
    u64 m_ReadPointer0 : ElementCountExponent + 1;
    u64 m_Pad1 : 64 - 3 * (ElementCountExponent + 1);
};

}
