/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include "Common.hpp"
#include <BitSet.hpp>

template<u32 NumEndpoints>
class BusArbiter final
{
    DEFAULT_DESTRUCT(BusArbiter);
    DELETE_CM(BusArbiter);
private:
    enum class Sensitivity
    {
        Reset = 0,
        Clock,
        MasterReady
    };

    SIGNAL_ENTITIES();
public:
    BusArbiter() noexcept
        : p_Reset_n(0)
        , p_Clock(0)
        , p_MasterReady(0)
        , p_out_Finished(0)
        , p_Pad0(0)
        , p_Ready(NumEndpoints)
        , p_out_BusSelect(0)
        , m_EndpointPriority { }
        , m_MasterReady(0)
        , m_Pad0(0)
    { }

    void SetResetN(const u32 reset_n) noexcept
    {
        p_Reset_n = reset_n;

        Processes(Sensitivity::Reset);
    }

    void SetClock(const bool clock) noexcept
    {
        p_Clock = BOOL_TO_BIT(clock);

        Processes(Sensitivity::Clock);
    }

    void SetMasterReady(const bool masterReady) noexcept
    {
        p_MasterReady = BOOL_TO_BIT(masterReady);

        Processes(Sensitivity::MasterReady);
    }

    // This is just a bit vector, which the controller will assign each bit to each entity on the bus.
    void SetReady(const u32 readyLineNumber, const bool ready) noexcept
    {
        p_Ready[readyLineNumber] = ready;
    }

    [[nodiscard]] bool GetFinished() const noexcept { return p_out_Finished; }
    [[nodiscard]] u32 GetBusSelect() const noexcept { return p_out_BusSelect; }
private:
    void Processes(const Sensitivity trigger) noexcept
    {
        ResetHandler(trigger);
        ClockHandler(trigger);
        MasterReadyHandler(trigger);
    }

    void ResetHandler(const Sensitivity trigger) noexcept
    {
        if(FallingEdge<Sensitivity::Reset>(p_Reset_n, trigger))
        {
            p_out_BusSelect = 0;
            for(u32 i = 0; i < NumEndpoints; ++i)
            {
                m_EndpointPriority[i] = 0;
            }
        }
    }

    void ClockHandler(const Sensitivity trigger) noexcept
    {
        if(RisingEdge<Sensitivity::Clock>(p_Clock, trigger))
        {
            if(p_out_Finished)
            {
                if(m_MasterReady)
                {
                    SelectNextEndpoint();
                }
            }
            else if(p_Ready[p_out_BusSelect] == 0)
            {
                m_EndpointPriority[p_out_BusSelect] = 0;
                p_out_Finished = 1;
            }
        }
    }

    void MasterReadyHandler(const Sensitivity trigger) noexcept
    {
        if(RisingEdge<Sensitivity::MasterReady>(p_MasterReady, trigger))
        {
            m_MasterReady = 1;
        }
    }

    void SelectNextEndpoint() noexcept
    {
        u32 activeCount = 0;

        for(u32 i = 0; i < NumEndpoints; ++i)
        {
            if(p_Ready[i])
            {
                ++m_EndpointPriority[i];
                ++activeCount;
            }
        }

        if(activeCount == 0)
        {
            return;
        }

        u32 maxIndex = 0;
        u32 maxValue = 0;

        for(u32 i = 0; i < NumEndpoints; ++i)
        {
            if(m_EndpointPriority[i] > maxValue)
            {
                maxIndex = i;
                maxValue = m_EndpointPriority[i];
            }
        }

        p_out_Finished = 0;
        p_out_BusSelect = maxIndex;
        m_MasterReady = 0;
    }
private:
    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    u32 p_MasterReady : 1;
    u32 p_out_Finished : 1;
    u32 p_Pad0 : 28;
    BitSet p_Ready;
    u32 p_out_BusSelect;

    u32 m_EndpointPriority[NumEndpoints];
    u32 m_MasterReady : 1;
    u32 m_Pad0 : 31;
};
