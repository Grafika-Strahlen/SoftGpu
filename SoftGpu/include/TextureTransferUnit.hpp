/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include "Common.hpp"
#include "DMAController.hpp"

class TextureTransferUnit final
{
    DEFAULT_DESTRUCT(TextureTransferUnit);
    DELETE_CM(TextureTransferUnit);
public:
    enum class TransferMode : u32
    {
        GpuToGpu = 0,
        GpuToCpu = 1,
        CpuToGpu = 2
    };
private:
    SENSITIVITY_DECL(p_Reset_n, p_Clock, p_Active);

    SIGNAL_ENTITIES();
public:
    TextureTransferUnit(Processor* const processor, const u32 index) noexcept
        : m_Processor(processor)
        , p_Reset_n(1)
        , p_Clock(0)
        , p_TransferMode(TransferMode::GpuToGpu)
        , p_Active(0)
        , p_out_Finished(0)
        , p_Pad0(0)
        , p_SourceBaseAddress(0)
        , p_DestBaseAddress(0)
        , p_SourceWidth(0)
        , p_SourceHeight(0)
        , p_SourceDepth(0)
        , p_DestWidth(0)
        , p_DestHeight(0)
        , p_DestDepth(0)
        , p_SourceX(0)
        , p_SourceY(0)
        , p_SourceZ(0)
        , p_DestX(0)
        , p_DestY(0)
        , p_DestZ(0)
        , p_CopyWidth(0)
        , p_CopyHeight(0)
        , p_CopyDepth(0)
        , m_TransferUnitIndex(index)
        , m_Pad0(0)
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

    void SetTransferMode(const TransferMode transferMode) noexcept
    {
        p_TransferMode = transferMode;
    }

    void SetActive(const bool active) noexcept
    {
        p_Active = BOOL_TO_BIT(active);

        TRIGGER_SENSITIVITY(p_Active);
    }

    [[nodiscard]] bool GetFinished() const noexcept { return BIT_TO_BOOL(p_out_Finished); }

    void SetSourceBaseAddress(const u64 sourceBaseAddress) noexcept
    {
        p_SourceBaseAddress = sourceBaseAddress;
    }

    void SetDestBaseAddress(const u64 destBaseAddress) noexcept
    {
        p_DestBaseAddress = destBaseAddress;
    }

    void SetSourceWidth(const u32 sourceWidth) noexcept
    {
        p_SourceWidth = sourceWidth;
    }

    void SetSourceHeight(const u32 sourceHeight) noexcept
    {
        p_SourceHeight = sourceHeight;
    }

    void SetSourceDepth(const u16 sourceDepth) noexcept
    {
        p_SourceDepth = sourceDepth;
    }

    void SetDestWidth(const u32 destWidth) noexcept
    {
        p_DestWidth = destWidth;
    }

    void SetDestHeight(const u32 destHeight) noexcept
    {
        p_DestHeight = destHeight;
    }

    void SetDestDepth(const u16 destDepth) noexcept
    {
        p_DestDepth = destDepth;
    }

    void SetSourceX(const u32 sourceX) noexcept
    {
        p_SourceX = sourceX;
    }

    void SetSourceY(const u32 sourceY) noexcept
    {
        p_SourceY = sourceY;
    }

    void SetSourceZ(const u16 sourceZ) noexcept
    {
        p_SourceZ = sourceZ;
    }

    void SetDestX(const u32 destX) noexcept
    {
        p_DestX = destX;
    }

    void SetDestY(const u32 destY) noexcept
    {
        p_DestY = destY;
    }

    void SetDestZ(const u16 destZ) noexcept
    {
        p_DestZ = destZ;
    }

    void SetCopyWidth(const u32 copyWidth) noexcept
    {
        p_CopyWidth = copyWidth;
    }

    void SetCopyHeight(const u32 copyHeight) noexcept
    {
        p_CopyHeight = copyHeight;
    }

    void SetCopyDepth(const u16 copyDepth) noexcept
    {
        p_CopyDepth = copyDepth;
    }
private:
    PROCESSES_DECL()
    {
        PROCESS_ENTER(ClockHandler, p_Reset_n, p_Clock);
        PROCESS_ENTER(ActiveHandler, p_Active);
    }

    PROCESS_DECL(ClockHandler)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            p_TransferMode = TransferMode::GpuToGpu;
            p_Active = 0;
            p_out_Finished = 0;
            p_SourceBaseAddress = 0;
            p_DestBaseAddress = 0;
            p_SourceWidth = 0;
            p_SourceHeight = 0;
            p_SourceDepth = 0;
            p_DestWidth = 0;
            p_DestHeight = 0;
            p_DestDepth = 0;
            p_SourceX = 0;
            p_SourceY = 0;
            p_SourceZ = 0;
            p_DestX = 0;
            p_DestY = 0;
            p_DestZ = 0;
            p_CopyWidth = 0;
            p_CopyHeight = 0;
            p_CopyDepth = 0;
        }
        else if(RISING_EDGE(p_Clock))
        {
        }
    }

    void ActiveHandler(const Sensitivity trigger) noexcept
    {
        if(RISING_EDGE(p_Active))
        {
            p_out_Finished = 0;
        }
    }
private:
    Processor* m_Processor;

    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    TransferMode p_TransferMode : 2;
    u32 p_Active : 1;
    u32 p_out_Finished : 1;
    u32 p_Pad0 : 26;
    u64 p_SourceBaseAddress;
    u64 p_DestBaseAddress;
    u32 p_SourceWidth;
    u32 p_SourceHeight;
    u16 p_SourceDepth;
    u32 p_DestWidth;
    u32 p_DestHeight;
    u16 p_DestDepth;
    u32 p_SourceX;
    u32 p_SourceY;
    u16 p_SourceZ;
    u32 p_DestX;
    u32 p_DestY;
    u16 p_DestZ;
    u32 p_CopyWidth;
    u32 p_CopyHeight;
    u16 p_CopyDepth;

    u32 m_TransferUnitIndex : 1;
    u32 m_Pad0 : 21;
};

class TextureTransferController final
{
    DEFAULT_DESTRUCT(TextureTransferController);
    DELETE_CM(TextureTransferController);
public:
    using TransferMode = TextureTransferUnit::TransferMode;
public:
    static inline constexpr u16 TRANSFER_CHANNEL_COUNT = 2;
    static inline constexpr u16 DMA_CONTROLLER_BUS_INDEX_BASE = 4;
private:
    enum class Sensitivity
    {
        Reset = 0,
        Clock
    };

    SIGNAL_ENTITIES(Sensitivity);
public:
    TextureTransferController(Processor* const processor) noexcept
        : m_Processor(processor)
        , p_Reset_n(1)
        , p_Clock(0)
        , p_TransferMode(TransferMode::GpuToGpu)
        , p_Active(0)
        , p_Pad0(0)
        , p_SourceBaseAddress(0)
        , p_DestBaseAddress(0)
        , p_SourceWidth(0)
        , p_SourceHeight(0)
        , p_SourceDepth(0)
        , p_DestWidth(0)
        , p_DestHeight(0)
        , p_DestDepth(0)
        , p_SourceX(0)
        , p_SourceY(0)
        , p_SourceZ(0)
        , p_DestX(0)
        , p_DestY(0)
        , p_DestZ(0)
        , p_CopyWidth(0)
        , p_CopyHeight(0)
        , p_CopyDepth(0)
        , m_BusArbiter()
        , m_Channels { { processor, 0 }, { processor, 1 } }
    { }

    void SetResetN(const bool reset_n) noexcept
    {
        p_Reset_n = BOOL_TO_BIT(reset_n);

        for(u32 i = 0; i < TRANSFER_CHANNEL_COUNT; ++i)
        {
            m_Channels->SetResetN(reset_n);
        }

        m_BusArbiter.SetResetN(reset_n);

        Processes(Sensitivity::Reset);
    }

    void SetClock(const bool clock) noexcept
    {
        p_Clock = BOOL_TO_BIT(clock);

        for(u32 i = 0; i < TRANSFER_CHANNEL_COUNT; ++i)
        {
            m_Channels->SetClock(clock);
        }

        m_BusArbiter.SetClock(clock);

        Processes(Sensitivity::Clock);
    }

    void SetTransferMode(const TransferMode transferMode) noexcept
    {
        p_TransferMode = transferMode;
    }

    void SetActive(const bool active) noexcept
    {
        p_Active = BOOL_TO_BIT(active);
    }

    void SetSourceBaseAddress(const u64 sourceBaseAddress) noexcept
    {
        p_SourceBaseAddress = sourceBaseAddress;
    }

    void SetDestBaseAddress(const u64 destBaseAddress) noexcept
    {
        p_DestBaseAddress = destBaseAddress;
    }

    void SetSourceWidth(const u32 sourceWidth) noexcept
    {
        p_SourceWidth = sourceWidth;
    }

    void SetSourceHeight(const u32 sourceHeight) noexcept
    {
        p_SourceHeight = sourceHeight;
    }

    void SetSourceDepth(const u16 sourceDepth) noexcept
    {
        p_SourceDepth = sourceDepth;
    }

    void SetDestWidth(const u32 destWidth) noexcept
    {
        p_DestWidth = destWidth;
    }

    void SetDestHeight(const u32 destHeight) noexcept
    {
        p_DestHeight = destHeight;
    }

    void SetDestDepth(const u16 destDepth) noexcept
    {
        p_DestDepth = destDepth;
    }

    void SetSourceX(const u32 sourceX) noexcept
    {
        p_SourceX = sourceX;
    }

    void SetSourceY(const u32 sourceY) noexcept
    {
        p_SourceY = sourceY;
    }

    void SetSourceZ(const u16 sourceZ) noexcept
    {
        p_SourceZ = sourceZ;
    }

    void SetDestX(const u32 destX) noexcept
    {
        p_DestX = destX;
    }

    void SetDestY(const u32 destY) noexcept
    {
        p_DestY = destY;
    }

    void SetDestZ(const u16 destZ) noexcept
    {
        p_DestZ = destZ;
    }

    void SetCopyWidth(const u32 copyWidth) noexcept
    {
        p_CopyWidth = copyWidth;
    }

    void SetCopyHeight(const u32 copyHeight) noexcept
    {
        p_CopyHeight = copyHeight;
    }

    void SetCopyDepth(const u16 copyDepth) noexcept
    {
        p_CopyDepth = copyDepth;
    }

    void SetReady(const u32 readyLineNumber, const bool ready) noexcept
    {
        m_BusArbiter.SetReady(readyLineNumber, ready);
    }

    [[nodiscard]] u32 GetBusSelect() const noexcept { return m_BusArbiter.GetBusSelect(); }
private:
    void Processes(const Sensitivity trigger) noexcept
    {
        PROCESS_BEGIN(Sensitivity::Clock, Sensitivity::Reset)
        {
            ClockHandler(trigger);
        }
    }

    void ClockHandler(const Sensitivity trigger) noexcept
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            p_TransferMode = TransferMode::GpuToGpu;
            p_Active = 0;
            p_SourceBaseAddress = 0;
            p_DestBaseAddress = 0;
            p_SourceWidth = 0;
            p_SourceHeight = 0;
            p_SourceDepth = 0;
            p_DestWidth = 0;
            p_DestHeight = 0;
            p_DestDepth = 0;
            p_SourceX = 0;
            p_SourceY = 0;
            p_SourceZ = 0;
            p_DestX = 0;
            p_DestY = 0;
            p_DestZ = 0;
            p_CopyWidth = 0;
            p_CopyHeight = 0;
            p_CopyDepth = 0;
        }
        else if(RisingEdge<Sensitivity::Clock>(p_Clock, trigger))
        {
            if(m_BusArbiter.GetFinished())
            {
                for(u32 i = 0; i < TRANSFER_CHANNEL_COUNT; ++i)
                {
                    if(m_Channels[i].GetFinished())
                    {
                        m_Channels[i].SetTransferMode(p_TransferMode);
                        m_Channels[i].SetActive(p_Active);
                        m_Channels[i].SetSourceBaseAddress(p_SourceBaseAddress);
                        m_Channels[i].SetDestBaseAddress(p_DestBaseAddress);
                        m_Channels[i].SetSourceWidth(p_SourceWidth);
                        m_Channels[i].SetSourceHeight(p_SourceHeight);
                        m_Channels[i].SetSourceDepth(p_SourceDepth);
                        m_Channels[i].SetDestWidth(p_DestWidth);
                        m_Channels[i].SetDestHeight(p_DestHeight);
                        m_Channels[i].SetDestDepth(p_DestDepth);
                        m_Channels[i].SetSourceX(p_SourceX);
                        m_Channels[i].SetSourceY(p_SourceY);
                        m_Channels[i].SetSourceZ(p_SourceZ);
                        m_Channels[i].SetDestX(p_DestX);
                        m_Channels[i].SetDestY(p_DestY);
                        m_Channels[i].SetDestZ(p_DestZ);
                        m_Channels[i].SetCopyWidth(p_CopyWidth);
                        m_Channels[i].SetCopyHeight(p_CopyHeight);
                        m_Channels[i].SetCopyDepth(p_CopyDepth);

                        break;
                    }
                }

                m_BusArbiter.SetMasterReady(true);
            }
        }
    }
private:
    Processor* m_Processor;

    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    TransferMode p_TransferMode : 2;
    u32 p_Active : 1;
    u32 p_Pad0 : 27;
    u64 p_SourceBaseAddress;
    u64 p_DestBaseAddress;
    u32 p_SourceWidth;
    u32 p_SourceHeight;
    u16 p_SourceDepth;
    u32 p_DestWidth;
    u32 p_DestHeight;
    u16 p_DestDepth;
    u32 p_SourceX;
    u32 p_SourceY;
    u16 p_SourceZ;
    u32 p_DestX;
    u32 p_DestY;
    u16 p_DestZ;
    u32 p_CopyWidth;
    u32 p_CopyHeight;
    u16 p_CopyDepth;

    BusArbiter<2> m_BusArbiter;
    TextureTransferUnit m_Channels[TRANSFER_CHANNEL_COUNT];
};
