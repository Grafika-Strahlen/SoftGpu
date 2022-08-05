#pragma once

#include <NumTypes.hpp>
#include <Objects.hpp>

class Processor;

#define REGISTER_MAGIC_VALUE (0x4879666C)
#define REGISTER_REVISION_VALUE (0x00000001)

#define REGISTER_MAGIC     (0x0000)
#define REGISTER_REVISION  (0x0004)
#define REGISTER_EMULATION (0x0008)
#define REGISTER_RESET     (0x000C)
#define REGISTER_CONTROL   (0x0010)

#define CONTROL_REGISTER_VALID_MASK (0x00000001)

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

class PCIControlRegisters final
{
    DEFAULT_DESTRUCT(PCIControlRegisters);
    DELETE_CM(PCIControlRegisters);
public:
    PCIControlRegisters(Processor* const processor) noexcept
        : m_Processor(processor)
        , m_ControlRegister{.Value = 0}
    { }

    void Reset()
    {
        m_ControlRegister.Value = 0;
    }

    [[nodiscard]] u32 Read(u32 address) noexcept;

    void Write(u32 address, u32 value) noexcept;
private:
    Processor* m_Processor;
    ControlRegister m_ControlRegister;
};
