/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <Common.hpp>

namespace riscv::fifo {

class SynchronizerReceiverSample
{
public:
    void ReceiveSynchronizer_Pointer(const u32 index, const u64 pointer) noexcept { }
};

template<typename Receiver = SynchronizerReceiverSample, u32 ElementCountExponent = 2>
class Synchronizer final
{
    DEFAULT_DESTRUCT(Synchronizer);
    DELETE_CM(Synchronizer);
private:
    SENSITIVITY_DECL(p_Reset_n, p_Clock);

    SIGNAL_ENTITIES();
public:
    Synchronizer(Receiver* const parent, const u32 index = 0) noexcept
        : m_Parent(parent)
        , m_Index(index)
        , p_Reset_n(0)
        , p_Clock(0)
        , m_Pad0(0)
        , p_Pointer(0)
        , m_PointerStaging(0)
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

    void SetPointer(const u64 pointer) noexcept
    {
        p_Pointer = pointer;
    }
private:
    PROCESSES_DECL()
    {
        PROCESS_ENTER(PointersHandler, p_Reset_n, p_Clock);
    }

    PROCESS_DECL(PointersHandler)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_Parent->ReceiveSynchronizer_Pointer(m_Index, 0);
            m_PointerStaging = 0;
        }
        else if(RISING_EDGE(p_Clock))
        {
            m_Parent->ReceiveSynchronizer_Pointer(m_Index, m_PointerStaging);
            m_PointerStaging = p_Pointer;
        }
    }

private:
    Receiver* m_Parent;
    u32 m_Index;

    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    u32 m_Pad0 : 30;

    u64 p_Pointer : ElementCountExponent + 1;
    u64 m_PointerStaging : ElementCountExponent + 1;
    u64 m_Pad1 : 64 - 2 * (ElementCountExponent + 1);
};

}
