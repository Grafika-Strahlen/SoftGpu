#pragma once

#include <NumTypes.hpp>
#include <Objects.hpp>

#include "DisplayManager.hpp"

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

class PciControlRegisters final
{
    DEFAULT_DESTRUCT(PciControlRegisters);
    DELETE_CM(PciControlRegisters);
public:
    static inline constexpr u32 REGISTER_MAGIC_VALUE = 0x4879666C;
    static inline constexpr u32 REGISTER_REVISION_VALUE = 0x00000001;

    static inline constexpr u16 REGISTER_MAGIC                  = 0x0000;
    static inline constexpr u16 REGISTER_REVISION               = 0x0004;
    static inline constexpr u16 REGISTER_EMULATION              = 0x0008;
    static inline constexpr u16 REGISTER_RESET                  = 0x000C;
    static inline constexpr u16 REGISTER_CONTROL                = 0x0010;
    static inline constexpr u16 REGISTER_VRAM_SIZE_LOW          = 0x0014;
    static inline constexpr u16 REGISTER_VRAM_SIZE_HIGH         = 0x0018;
    static inline constexpr u16 REGISTER_INTERRUPT_TYPE         = 0x001C;

    static inline constexpr u32 MSG_INTERRUPT_NONE              = 0x00000000;
    static inline constexpr u32 MSG_INTERRUPT_VSYNC_DISPLAY_0   = 0x00000010; // 0x10 - 0x17
                                                            
    static inline constexpr u16 REGISTER_VGA_WIDTH              = 0x1014;
    static inline constexpr u16 REGISTER_VGA_HEIGHT             = 0x1018;
    
    static inline constexpr u16 BASE_REGISTER_DI                = 0x2000;
    static inline constexpr u16 SIZE_REGISTER_DI                = 7 * 0x4;
    static inline constexpr u16 OFFSET_REGISTER_DI_WIDTH        = 0x00;
    static inline constexpr u16 OFFSET_REGISTER_DI_HEIGHT       = 0x04;
    static inline constexpr u16 OFFSET_REGISTER_DI_BPP          = 0x08;
    static inline constexpr u16 OFFSET_REGISTER_DI_ENABLE       = 0x0C;
    static inline constexpr u16 OFFSET_REGISTER_DI_REFRESH_RATE_NUMERATOR   = 0x10;
    static inline constexpr u16 OFFSET_REGISTER_DI_REFRESH_RATE_DENOMINATOR = 0x14;
    static inline constexpr u16 OFFSET_REGISTER_DI_VSYNC_ENABLE = 0x18;

    static inline constexpr u16 BASE_REGISTER_EDID              = 0x3000;
    static inline constexpr u16 SIZE_EDID                       = 128;

    static inline constexpr u16 REGISTER_DEBUG_PRINT            = 0x8000;

    static inline constexpr u32 CONTROL_REGISTER_VALID_MASK     = 0x00000001;

    static inline constexpr u16 DEFAULT_VGA_WIDTH  = 720;
    static inline constexpr u16 DEFAULT_VGA_HEIGHT = 480;
public:
    PciControlRegisters(Processor* const processor) noexcept
        : m_Processor(processor)
        , m_ControlRegister{.Value = 0}
        , m_VgaWidth(DEFAULT_VGA_WIDTH)
        , m_VgaHeight(DEFAULT_VGA_HEIGHT)
        , m_CurrentInterruptMessage(MSG_INTERRUPT_NONE)
        , m_Bus()
        , m_ReadState(0)
        , m_WriteState(0)
        , m_Pad0(0)
        , m_DisplayEdidStorage()
        , m_DisplayDataStorage()
        , m_DebugReadCallback(nullptr)
        , m_DebugWriteCallback(nullptr)
    {
        m_Bus.ReadBusLocked = 0;
        m_Bus.WriteBusLocked = 0;
    }

    void Reset()
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

        m_ReadState = 0;
    }

    void Clock(bool risingEdge = false)
    {
        if(risingEdge)
        {
            ExecuteRead();
        }
        else
        {
            ExecuteWrite();
        }
    }

    [[nodiscard]] PciControlRegistersBus& Bus() noexcept { return m_Bus; }

    void SetInterrupt(const u32 messageType) noexcept
    {
        m_CurrentInterruptMessage = messageType;
    }

    // [[nodiscard]] u32 Read(u32 address) noexcept;
    //
    // void Write(u32 address, u32 value) noexcept;

    void RegisterDebugCallbacks(const PciControlDebugReadCallback_f debugReadCallback, const PciControlDebugWriteCallback_f debugWriteCallback) noexcept
    {
        m_DebugReadCallback = debugReadCallback;
        m_DebugWriteCallback = debugWriteCallback;
    }
private:
    void ExecuteRead() noexcept;
    void ExecuteWrite() noexcept;
private:
    Processor* m_Processor;
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

    PciControlDebugReadCallback_f m_DebugReadCallback;
    PciControlDebugWriteCallback_f m_DebugWriteCallback;
};
