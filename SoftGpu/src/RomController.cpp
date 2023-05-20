#include "RomController.hpp"
#include "Processor.hpp"

u16 RomController::PciReadExpansionRom(const u64 address, const u16 size, u32* const data) noexcept
{
    if constexpr(false)
    {
        ConPrinter::PrintLn(u8"Attempting to read from ROM. Address = 0x{XP0}, Size = {}", address, size);
    }

    if(!m_Processor->ExpansionRomEnable())
    {
        ConPrinter::PrintLn(u8"Attempting to read Expansion ROM while it is disabled.");
        return 0;
    }

    u64 trueSize = size;

    if(address + size > sizeof(m_ExpansionRom))
    {
        trueSize = sizeof(m_ExpansionRom) - address;
    }

    ::std::memcpy(data, reinterpret_cast<u8*>(m_ExpansionRom) + address, trueSize);

    if constexpr(false)
    {
        ConPrinter::PrintLn(u8"First Byte of Block: 0x{XP0}", m_ExpansionRom[address]);
    }

    if constexpr(false)
    {
        ConPrinter::PrintLn(u8"Read {} bytes from ROM.", trueSize);
    }
    return static_cast<u16>(trueSize);
}

