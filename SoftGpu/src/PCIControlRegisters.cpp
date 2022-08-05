#include "PCIControlRegisters.hpp"
#include "Processor.hpp"

[[nodiscard]] u32 PCIControlRegisters::Read(const u32 address) noexcept
{
    switch(address)
    {
        case REGISTER_MAGIC: return REGISTER_MAGIC_VALUE;
        case REGISTER_REVISION: return REGISTER_REVISION_VALUE;
        case REGISTER_EMULATION: return 2;
        case REGISTER_RESET: m_Processor->Reset(); break;
        case REGISTER_CONTROL: return m_ControlRegister.Value;
        default: break;
    }

    return 0;
}

void PCIControlRegisters::Write(const u32 address, const u32 value) noexcept
{
    switch(address)
    {
        case REGISTER_CONTROL: m_ControlRegister.Value = value & CONTROL_REGISTER_VALID_MASK; break;
        default: return;
    }
}
