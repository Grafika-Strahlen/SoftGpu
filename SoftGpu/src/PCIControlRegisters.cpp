#include "PCIControlRegisters.hpp"
#include "Processor.hpp"

[[nodiscard]] u32 PciControlRegisters::Read(const u32 address) noexcept
{
    switch(address)
    {
        case REGISTER_MAGIC: return REGISTER_MAGIC_VALUE;
        case REGISTER_REVISION: return REGISTER_REVISION_VALUE;
        case REGISTER_EMULATION: return 2;
        case REGISTER_RESET: m_Processor->Reset(); break;
        case REGISTER_CONTROL: return m_ControlRegister.Value;
        case REGISTER_VGA_WIDTH: return m_VgaWidth;
        case REGISTER_VGA_HEIGHT: return m_VgaHeight;
        default: break;
    }

    return 0;
}

void PciControlRegisters::Write(const u32 address, const u32 value) noexcept
{
    switch(address)
    {
        case REGISTER_CONTROL: m_ControlRegister.Value = value & CONTROL_REGISTER_VALID_MASK; break;
        case REGISTER_VGA_WIDTH: m_VgaWidth = static_cast<u16>(value); break;
        case REGISTER_VGA_HEIGHT: m_VgaHeight = static_cast<u16>(value); break;
        default: break;
    }
}
