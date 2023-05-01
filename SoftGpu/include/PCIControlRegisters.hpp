#pragma once

#include <NumTypes.hpp>
#include <Objects.hpp>

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

class PciControlRegisters final
{
    DEFAULT_DESTRUCT(PciControlRegisters);
    DELETE_CM(PciControlRegisters);
public:
    static inline constexpr u32 REGISTER_MAGIC_VALUE = 0x4879666C;
    static inline constexpr u32 REGISTER_REVISION_VALUE =0x00000001;

    static inline constexpr u16 REGISTER_MAGIC      = 0x0000;
    static inline constexpr u16 REGISTER_REVISION   = 0x0004;
    static inline constexpr u16 REGISTER_EMULATION  = 0x0008;
    static inline constexpr u16 REGISTER_RESET      = 0x000C;
    static inline constexpr u16 REGISTER_CONTROL    = 0x0010;
    static inline constexpr u16 REGISTER_VGA_WIDTH  = 0x1014;
    static inline constexpr u16 REGISTER_VGA_HEIGHT = 0x1018;

    static inline constexpr u32 CONTROL_REGISTER_VALID_MASK = 0x00000001;

    static inline constexpr u16 DEFAULT_VGA_WIDTH = 720;
    static inline constexpr u16 DEFAULT_VGA_HEIGHT = 480;
public:
    PciControlRegisters(Processor* const processor) noexcept
        : m_Processor(processor)
        , m_ControlRegister{.Value = 0}
        , m_VgaWidth(DEFAULT_VGA_WIDTH)
        , m_VgaHeight(DEFAULT_VGA_HEIGHT)
    { }

    void Reset()
    {
        m_ControlRegister.Value = 0;
        m_VgaWidth = DEFAULT_VGA_WIDTH;
        m_VgaHeight = DEFAULT_VGA_HEIGHT;
    }

    [[nodiscard]] u32 Read(u32 address) noexcept;

    void Write(u32 address, u32 value) noexcept;
private:
    Processor* m_Processor;
    ControlRegister m_ControlRegister;
    u16 m_VgaWidth;
    u16 m_VgaHeight;
};
