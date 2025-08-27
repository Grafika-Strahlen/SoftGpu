/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include "Common.hpp"

#include "DisplayManager.hpp"
#include "DMAController.hpp"

class Processor;

struct ControlRegister final
{
    union
    {
        struct
        {
            u32 InterruptEnable : 1;
            u32 Reserved : 31;
        };
        u32 Value;
    };
};

struct PciControlRegistersBus final
{
    DEFAULT_CONSTRUCT_PUC(PciControlRegistersBus);
    DEFAULT_DESTRUCT(PciControlRegistersBus);
    DEFAULT_CM_PUC(PciControlRegistersBus);

    /**
     * \brief Lock State
     *   - 0 - Unlocked
     *   - 1 - Locked
     *   - 2 - Operation Complete
     */
    u32 ReadBusLocked : 2;
    /**
     * \brief Lock State
     *   - 0 - Unlocked
     *   - 1 - Locked
     *   - 2 - Operation Complete
     */
    u32 WriteBusLocked : 2;

    u32 Pad0 : 28;
        
    u32 ReadAddress;
    u16 ReadSize;
    u32 ReadResponse;
        
    u32 WriteAddress;
    u16 WriteSize;
    u32 WriteValue;
};

typedef void (*PciControlDebugReadCallback_f)(u32 localAddress);
typedef void (*PciControlDebugWriteCallback_f)(u32 localAddress, u32 value);

class PciControlRegistersReceiverSample
{
public:
    void ReceivePciControlRegisters_TriggerResetN(const StdLogic reset_n) noexcept { }

    void ReceivePciControlRegisters_DisplayManagerRequestActive(const StdLogic active) noexcept { }
    void ReceivePciControlRegisters_DisplayManagerRequestPacketType(const DisplayRequestPacketType packetType) noexcept { }
    void ReceivePciControlRegisters_DisplayManagerRequestReadWrite(const EReadWrite readWrite) noexcept { }
    void ReceivePciControlRegisters_DisplayManagerRequestDisplayIndex(const u32 displayIndex) noexcept { }
    void ReceivePciControlRegisters_DisplayManagerRequestRegister(const u32 registerIndex) noexcept { }
    void ReceivePciControlRegisters_DisplayManagerRequestData(const u32 data) noexcept { }
};

class PciControlRegisters final
{
    DEFAULT_DESTRUCT(PciControlRegisters);
    DELETE_CM(PciControlRegisters);
public:
    static inline constexpr u32 REGISTER_MAGIC_VALUE    = 0x4879666C;
    static inline constexpr u32 REGISTER_REVISION_VALUE = 0x00000001;
    static inline constexpr u32 VALUE_REGISTER_EMULATION_MICROPROCESSOR = 0;
    static inline constexpr u32 VALUE_REGISTER_EMULATION_FPGA           = 1;
    static inline constexpr u32 VALUE_REGISTER_EMULATION_SIMULATION     = 2;

    static inline constexpr u16 REGISTER_MAGIC                  = 0x0000;
    static inline constexpr u16 REGISTER_REVISION               = 0x0004;
    static inline constexpr u16 REGISTER_EMULATION              = 0x0008;
    static inline constexpr u16 REGISTER_RESET                  = 0x000C;
    static inline constexpr u16 REGISTER_CONTROL                = 0x0010;
    static inline constexpr u16 REGISTER_VRAM_SIZE_LOW          = 0x0014;
    static inline constexpr u16 REGISTER_VRAM_SIZE_HIGH         = 0x0018;
    static inline constexpr u16 REGISTER_INTERRUPT_TYPE         = 0x001C;
    static inline constexpr u16 REGISTER_DMA_CHANNEL_COUNT      = 0x0020;

    static inline constexpr u32 MSG_INTERRUPT_NONE              = 0x00000000;
    static inline constexpr u32 MSG_INTERRUPT_VSYNC_DISPLAY_0   = 0x00000010; // 0x10 - 0x17
    static inline constexpr u32 MSG_INTERRUPT_DMA_COMPLETED     = 0x00000018;
                                                            
    static inline constexpr u16 REGISTER_VGA_WIDTH              = 0x1014;
    static inline constexpr u16 REGISTER_VGA_HEIGHT             = 0x1018;
    
    static inline constexpr u16 BASE_REGISTER_DI                = 0x2000;
    static inline constexpr u16 SIZE_REGISTER_DI                = 9 * 0x4;
    static inline constexpr u16 OFFSET_REGISTER_DI_WIDTH        = 0x00;
    static inline constexpr u16 OFFSET_REGISTER_DI_HEIGHT       = 0x04;
    static inline constexpr u16 OFFSET_REGISTER_DI_BPP          = 0x08;
    static inline constexpr u16 OFFSET_REGISTER_DI_ENABLE       = 0x0C;
    static inline constexpr u16 OFFSET_REGISTER_DI_REFRESH_RATE_NUMERATOR   = 0x10;
    static inline constexpr u16 OFFSET_REGISTER_DI_REFRESH_RATE_DENOMINATOR = 0x14;
    static inline constexpr u16 OFFSET_REGISTER_DI_VSYNC_ENABLE = 0x18;
    static inline constexpr u16 OFFSET_REGISTER_DI_FB_LOW       = 0x1C;
    static inline constexpr u16 OFFSET_REGISTER_DI_FB_HIGH      = 0x20;

    static inline constexpr u16 BASE_REGISTER_EDID              = 0x3000;
    static inline constexpr u16 SIZE_EDID                       = 128;

    static inline constexpr u16 REGISTER_DEBUG_PRINT            = 0x8000;
    static inline constexpr u16 REGISTER_DEBUG_LOG_LOCK         = 0x8004;
    static inline constexpr u32 VALUE_DEBUG_LOG_LOCK_UNLOCKED   = 0x00000000;
    static inline constexpr u16 REGISTER_DEBUG_LOG_MULTI        = 0x8008;

    static inline constexpr u16 BASE_REGISTER_DMA               = 0x4000;
    static inline constexpr u16 SIZE_REGISTER_DMA               = 8 * 0x4;
    static inline constexpr u16 OFFSET_REGISTER_DMA_CPU_LOW     = 0x00;
    static inline constexpr u16 OFFSET_REGISTER_DMA_CPU_HIGH    = 0x04;
    static inline constexpr u16 OFFSET_REGISTER_DMA_GPU_LOW     = 0x08;
    static inline constexpr u16 OFFSET_REGISTER_DMA_GPU_HIGH    = 0x0C;
    static inline constexpr u16 OFFSET_REGISTER_DMA_SIZE_LOW    = 0x10;
    static inline constexpr u16 OFFSET_REGISTER_DMA_SIZE_HIGH   = 0x14;
    static inline constexpr u16 OFFSET_REGISTER_DMA_CONTROL     = 0x18;
    static inline constexpr u16 OFFSET_REGISTER_DMA_LOCK        = 0x1C;
    static inline constexpr u32 VALUE_DMA_LOCK_UNLOCKED         = 0x00000000;

    static inline constexpr u32 CONTROL_REGISTER_VALID_MASK     = 0x00000001;

    static inline constexpr u16 DEFAULT_VGA_WIDTH  = 720;
    static inline constexpr u16 DEFAULT_VGA_HEIGHT = 480;

    static inline constexpr u32 DMA_CHANNEL_COUNT = 4;
private:
    using Receiver = PciControlRegistersReceiverSample;

    SENSITIVITY_DECL(p_Reset_n, p_Clock);

    SIGNAL_ENTITIES();
public:
    explicit PciControlRegisters(Receiver* const parent) noexcept
        : m_Parent(parent)
        , p_Reset_n(1)
        , p_Clock(0)
        , p_DisplayManagerAcknowledge(0)
        , p_Pad0(0)
        , p_DisplayManagerData(0)
        , m_ControlRegister{.Value = 0}
        , m_VgaWidth(DEFAULT_VGA_WIDTH)
        , m_VgaHeight(DEFAULT_VGA_HEIGHT)
        , m_CurrentInterruptMessage(MSG_INTERRUPT_NONE)
        , m_Bus()
        , m_ReadState(0)
        , m_WriteState(0)
        , m_Pad0(0)
        , m_DisplayEdidStorage()
        , m_DisplayDataStorage(0)
        , m_DebugLogLock(0)
        , m_DmaLock { VALUE_DMA_LOCK_UNLOCKED, VALUE_DMA_LOCK_UNLOCKED, VALUE_DMA_LOCK_UNLOCKED, VALUE_DMA_LOCK_UNLOCKED }
        , m_DmaBuses {  }
        , m_DmaRequestNumbers { }
        , m_DebugReadCallback(nullptr)
        , m_DebugWriteCallback(nullptr)
    {
        m_Bus.ReadBusLocked = 0;
        m_Bus.WriteBusLocked = 0;
    }

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

    void SetDisplayManagerAcknowledge(const bool acknowledge) noexcept
    {
        p_DisplayManagerAcknowledge = BOOL_TO_BIT(acknowledge);
    }

    [[nodiscard]] PciControlRegistersBus& Bus() noexcept { return m_Bus; }

    void SetInterrupt(const u32 messageType) noexcept
    {
        m_CurrentInterruptMessage = messageType;
    }

    void RegisterDebugCallbacks(const PciControlDebugReadCallback_f debugReadCallback, const PciControlDebugWriteCallback_f debugWriteCallback) noexcept
    {
        m_DebugReadCallback = debugReadCallback;
        m_DebugWriteCallback = debugWriteCallback;
    }
private:
    PROCESSES_DECL()
    {
        PROCESS_ENTER(ClockHandler, p_Reset_n, p_Clock);
    }

    PROCESS_DECL(ClockHandler)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_ControlRegister.Value = 0;
            m_VgaWidth = DEFAULT_VGA_WIDTH;
            m_VgaHeight = DEFAULT_VGA_HEIGHT;

            m_Bus.ReadBusLocked = 0;
            m_Bus.WriteBusLocked = 0;
            m_Bus.ReadAddress = 0;
            m_Bus.ReadResponse = 0;
            m_Bus.WriteAddress = 0;
            m_Bus.WriteValue = 0;

            m_DebugLogLock = 0;

            m_ReadState = 0;

            for(u32 i = 0; i < DMA_CHANNEL_COUNT; ++i)
            {
                m_DmaLock[i] = VALUE_DMA_LOCK_UNLOCKED;
                m_DmaBuses[i] = { };
                m_DmaRequestNumbers[i] = 0;
            }

            //   This needs to be done after a clock cycle to ensure we don't end
            // up in a cyclical situation where the reset is cleared, then set for
            // each of the entities. This is largely a side effect of C++, but
            // probably can't hurt on actual hardware.
            //   To be extra safe we'll only do it on the falling of the clock when
            // nothing is happening.
            if(FALLING_EDGE(p_Clock))
            {
                m_Parent->ReceivePciControlRegisters_TriggerResetN(StdLogic::Z);
            }
        }
        else if(RISING_EDGE(p_Clock))
        {
            ExecuteRead();
        }
        else if(FALLING_EDGE(p_Clock))
        {
            ExecuteWrite();
        }
    }

    void ExecuteRead() noexcept
    {
        if(m_Bus.ReadBusLocked != 1)
        {
            return;
        }

        if(m_DebugReadCallback)
        {
            m_DebugReadCallback(m_Bus.ReadAddress);
        }

        if(m_Bus.ReadAddress >= BASE_REGISTER_EDID && m_Bus.ReadAddress < BASE_REGISTER_EDID + SIZE_EDID * DisplayManager<>::MaxDisplayCount)
        {
            ExecuteReadEdid();
        }
        else if(m_Bus.ReadAddress >= BASE_REGISTER_DI && m_Bus.ReadAddress < BASE_REGISTER_DI + SIZE_REGISTER_DI * DisplayManager<>::MaxDisplayCount)
        {
            ExecuteReadDI();
        }
        else if(m_Bus.WriteAddress >= BASE_REGISTER_DMA && m_Bus.WriteAddress < BASE_REGISTER_DMA + SIZE_REGISTER_DMA * DMAController::DMA_CHANNEL_COUNT)
        {
            ExecuteReadDMA();
        }

        switch(m_Bus.ReadAddress)
        {
            case REGISTER_MAGIC: m_Bus.ReadResponse = REGISTER_MAGIC_VALUE; break;
            case REGISTER_REVISION: m_Bus.ReadResponse = REGISTER_REVISION_VALUE; break;
            case REGISTER_EMULATION: m_Bus.ReadResponse = VALUE_REGISTER_EMULATION_SIMULATION; break;
            case REGISTER_RESET: m_Parent->ReceivePciControlRegisters_TriggerResetN(StdLogic::H); break;
            case REGISTER_CONTROL: m_Bus.ReadResponse = m_ControlRegister.Value; break;
            case REGISTER_VRAM_SIZE_LOW: m_Bus.ReadResponse = 256 * 1024 * 1024; break;
            case REGISTER_VRAM_SIZE_HIGH: m_Bus.ReadResponse = 0; break;
            // case REGISTER_VRAM_SIZE_LOW: m_Bus.ReadResponse = 0; break;
            // case REGISTER_VRAM_SIZE_HIGH: m_Bus.ReadResponse = 1; break; // 8 GiB
            case REGISTER_VGA_WIDTH: m_Bus.ReadResponse = m_VgaWidth; break;
            case REGISTER_VGA_HEIGHT: m_Bus.ReadResponse = m_VgaHeight; break;
            case REGISTER_INTERRUPT_TYPE: m_Bus.ReadResponse = m_CurrentInterruptMessage; break;
            case REGISTER_DMA_CHANNEL_COUNT: m_Bus.ReadResponse = DMA_CHANNEL_COUNT; break;
            case REGISTER_DEBUG_PRINT: m_Bus.ReadResponse = 0; break;
            case REGISTER_DEBUG_LOG_LOCK: m_Bus.ReadResponse = m_DebugLogLock; break;
            case REGISTER_DEBUG_LOG_MULTI: m_Bus.ReadResponse = 0; break;
            default: break;
        }

        m_Bus.ReadBusLocked = 2;
    }

    void ExecuteReadEdid() noexcept
    {
        const u32 offset = m_Bus.ReadAddress - BASE_REGISTER_EDID;

        const u32 displayIndex = offset / SIZE_EDID;
        const u32 registerOffset = offset % SIZE_EDID;

        if(m_ReadState == 0)
        {
            m_Parent->ReceivePciControlRegisters_DisplayManagerRequestActive(StdLogic::One);
            m_Parent->ReceivePciControlRegisters_DisplayManagerRequestPacketType(DisplayRequestPacketType::Edid);
            m_Parent->ReceivePciControlRegisters_DisplayManagerRequestReadWrite(EReadWrite::Read);
            m_Parent->ReceivePciControlRegisters_DisplayManagerRequestDisplayIndex(displayIndex);
            m_Parent->ReceivePciControlRegisters_DisplayManagerRequestRegister(registerOffset);
            m_ReadState = 1;
            return;
        }
        else if(m_ReadState == 1)
        {
            if(!BIT_TO_BOOL(p_DisplayManagerAcknowledge))
            {
                return;
            }

            m_Parent->ReceivePciControlRegisters_DisplayManagerRequestActive(StdLogic::HighImpedance);

            if(m_Bus.ReadSize == 1)
            {
                const u8* rawData = reinterpret_cast<const u8*>(&m_DisplayEdidStorage);
                m_Bus.ReadResponse = rawData[registerOffset];
            }
            else if(m_Bus.ReadSize == 2)
            {
                const u16* rawData = reinterpret_cast<const u16*>(&m_DisplayEdidStorage);
                m_Bus.ReadResponse = rawData[registerOffset / sizeof(u16)];
            }
            else if(m_Bus.ReadSize == 4)
            {
                const u32* rawData = reinterpret_cast<const u32*>(&m_DisplayEdidStorage);
                m_Bus.ReadResponse = rawData[registerOffset / sizeof(u32)];
            }

            m_ReadState = 0;
        }
    }

    void ExecuteReadDI() noexcept
    {
        const u32 offset = m_Bus.ReadAddress - BASE_REGISTER_DI;

        const u32 displayIndex = offset / SIZE_REGISTER_DI;
        const u32 registerOffset = (offset % SIZE_REGISTER_DI) / sizeof(u32);

        if(m_ReadState == 0)
        {
            m_Parent->ReceivePciControlRegisters_DisplayManagerRequestActive(StdLogic::One);
            m_Parent->ReceivePciControlRegisters_DisplayManagerRequestPacketType(DisplayRequestPacketType::DisplayInfo);
            m_Parent->ReceivePciControlRegisters_DisplayManagerRequestReadWrite(EReadWrite::Read);
            m_Parent->ReceivePciControlRegisters_DisplayManagerRequestDisplayIndex(displayIndex);
            m_Parent->ReceivePciControlRegisters_DisplayManagerRequestRegister(registerOffset);
            m_ReadState = 1;
            return;
        }
        else if(m_ReadState == 1)
        {
            if(!BIT_TO_BOOL(p_DisplayManagerAcknowledge))
            {
                return;
            }

            m_Parent->ReceivePciControlRegisters_DisplayManagerRequestActive(StdLogic::HighImpedance);

            if(m_Bus.ReadSize == 1)
            {
                const u8* rawData = reinterpret_cast<const u8*>(&m_DisplayDataStorage);
                m_Bus.ReadResponse = *rawData;
            }
            else if(m_Bus.ReadSize == 2)
            {
                const u16* rawData = reinterpret_cast<const u16*>(&m_DisplayDataStorage);
                m_Bus.ReadResponse = *rawData;
            }
            else if(m_Bus.ReadSize == 4)
            {
                const u32* rawData = &m_DisplayDataStorage;
                m_Bus.ReadResponse = *rawData;
            }

            m_ReadState = 0;
        }
    }

    void ExecuteReadDMA() noexcept
    {
        const u32 offset = m_Bus.WriteAddress - BASE_REGISTER_DI;

        const u32 displayIndex = offset / SIZE_REGISTER_DI;
        const u32 registerOffset = (offset % SIZE_REGISTER_DI) / sizeof(u32);

        if(registerOffset == OFFSET_REGISTER_DMA_LOCK)
        {
            m_Bus.ReadResponse = m_DmaLock[displayIndex];
        }
    }

    void ExecuteWrite() noexcept
    {
        HandleDmaBus();

        if(m_Bus.WriteBusLocked != 1)
        {
            return;
        }

        if(m_DebugWriteCallback)
        {
            m_DebugWriteCallback(m_Bus.WriteAddress, m_Bus.WriteAddress);
        }

        if(m_Bus.WriteAddress >= BASE_REGISTER_DI && m_Bus.WriteAddress < BASE_REGISTER_DI + SIZE_REGISTER_DI * DisplayManager<>::MaxDisplayCount)
        {
            ExecuteWriteDI();
        }
        else if(m_Bus.WriteAddress >= BASE_REGISTER_DMA && m_Bus.WriteAddress < BASE_REGISTER_DMA + SIZE_REGISTER_DMA * DMAController::DMA_CHANNEL_COUNT)
        {
            ExecuteWriteDMA();
        }

        switch(m_Bus.WriteAddress)
        {
            case REGISTER_CONTROL: m_ControlRegister.Value = m_Bus.WriteValue & CONTROL_REGISTER_VALID_MASK; break;
            case REGISTER_VGA_WIDTH: m_VgaWidth = static_cast<u16>(m_Bus.WriteValue); break;
            case REGISTER_VGA_HEIGHT: m_VgaHeight = static_cast<u16>(m_Bus.WriteValue); break;
            case REGISTER_INTERRUPT_TYPE: m_CurrentInterruptMessage = 0; break; // The CPU can only clear the interrupt.
            case REGISTER_DEBUG_LOG_LOCK:
                if(m_DebugLogLock == VALUE_DEBUG_LOG_LOCK_UNLOCKED)
                {
                    m_DebugLogLock = m_Bus.WriteValue;
                }
                else if(m_Bus.WriteValue == VALUE_DEBUG_LOG_LOCK_UNLOCKED)
                {
                    m_DebugLogLock = VALUE_DEBUG_LOG_LOCK_UNLOCKED;
                }
                break;
            default: break;
        }

        m_Bus.WriteBusLocked = 2;
    }

    void ExecuteWriteDI() noexcept
    {
        const u32 offset = m_Bus.WriteAddress - BASE_REGISTER_DI;

        const u32 displayIndex = offset / SIZE_REGISTER_DI;
        const u32 registerOffset = (offset % SIZE_REGISTER_DI) / sizeof(u32);

        if(m_WriteState == 0)
        {
            m_Parent->ReceivePciControlRegisters_DisplayManagerRequestActive(StdLogic::One);
            m_Parent->ReceivePciControlRegisters_DisplayManagerRequestPacketType(DisplayRequestPacketType::DisplayInfo);
            m_Parent->ReceivePciControlRegisters_DisplayManagerRequestReadWrite(EReadWrite::Write);
            m_Parent->ReceivePciControlRegisters_DisplayManagerRequestDisplayIndex(displayIndex);
            m_Parent->ReceivePciControlRegisters_DisplayManagerRequestRegister(registerOffset);
            m_Parent->ReceivePciControlRegisters_DisplayManagerRequestData(m_Bus.WriteValue);
            m_WriteState = 1;
            return;
        }
        else if(m_WriteState == 1)
        {
            if(!BIT_TO_BOOL(p_DisplayManagerAcknowledge))
            {
                return;
            }

            m_Parent->ReceivePciControlRegisters_DisplayManagerRequestActive(StdLogic::HighImpedance);

            m_WriteState = 0;
        }
    }

    void ExecuteWriteDMA() noexcept;

    void HandleDmaBus() noexcept;
private:
    Receiver* m_Parent;

    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    u32 p_DisplayManagerAcknowledge : 1;
    u32 p_Pad0 : 29;

    u32 p_DisplayManagerData;

    ControlRegister m_ControlRegister;
    u16 m_VgaWidth;
    u16 m_VgaHeight;
    u32 m_CurrentInterruptMessage;

    PciControlRegistersBus m_Bus;
    u32 m_ReadState : 2;
    u32 m_WriteState : 2;
    u32 m_Pad0 : 28;
    EdidBlock m_DisplayEdidStorage;
    u32 m_DisplayDataStorage;

    u32 m_DebugLogLock;
    u32 m_DmaLock[DMA_CHANNEL_COUNT];
    DMAChannelBus m_DmaBuses[DMA_CHANNEL_COUNT];
    u32 m_DmaRequestNumbers[DMA_CHANNEL_COUNT];

    PciControlDebugReadCallback_f m_DebugReadCallback;
    PciControlDebugWriteCallback_f m_DebugWriteCallback;
};
