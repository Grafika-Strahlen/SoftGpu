#include "PCIControlRegisters.hpp"
#include "Processor.hpp"

void PciControlRegisters::ExecuteRead() noexcept
{
    if(m_Bus.ReadBusLocked != 1)
    {
        return;
    }

    if(m_DebugReadCallback)
    {
        m_DebugReadCallback(m_Bus.ReadAddress);
    }

    if(m_Bus.ReadAddress >= BASE_REGISTER_EDID && m_Bus.ReadAddress < BASE_REGISTER_EDID + SIZE_EDID * DisplayManager::MaxDisplayCount)
    {
        const u32 offset = m_Bus.ReadAddress - BASE_REGISTER_EDID;

        const u32 displayIndex = offset / SIZE_EDID;
        const u32 registerOffset = offset % SIZE_EDID;

        if(m_ReadState == 0)
        {
            DisplayDataPacket packet;
            packet.BusActive = true;
            packet.PacketType = 0;
            packet.Read = true;
            packet.DisplayIndex = displayIndex;
            packet.EdidBusAssign = &m_DisplayEdidStorage;

            m_Processor->SetDisplayManagerBus(packet);
            m_ReadState = 1;
            return;
        }
        else if(m_ReadState == 1)
        {
            m_Processor->ResetDisplayManagerBus();

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

            // switch(registerOffset)
            // {
            //     case OFFSET_REGISTER_DI_WIDTH: m_Bus.ReadResponse = m_DisplayDataStorage.Width; break;
            //     case OFFSET_REGISTER_DI_HEIGHT: m_Bus.ReadResponse = m_DisplayDataStorage.Height; break;
            //     case OFFSET_REGISTER_DI_BPP: m_Bus.ReadResponse = m_DisplayDataStorage.BitsPerPixel; break;
            //     default: break;
            // }

            m_ReadState = 0;
        }
    }
    else if(m_Bus.ReadAddress >= BASE_REGISTER_DI && m_Bus.ReadAddress < BASE_REGISTER_DI + SIZE_REGISTER_DI * DisplayManager::MaxDisplayCount)
    {
        const u32 offset = m_Bus.ReadAddress - BASE_REGISTER_DI;

        const u32 displayIndex = offset / SIZE_REGISTER_DI;
        const u32 registerOffset = (offset % SIZE_REGISTER_DI) / sizeof(u32);

        if(m_ReadState == 0)
        {
            DisplayDataPacket packet;
            packet.BusActive = true;
            packet.PacketType = 1;
            packet.Read = true;
            packet.DisplayIndex = displayIndex;
            packet.Register = registerOffset;
            packet.Value = &m_DisplayDataStorage;

            m_Processor->SetDisplayManagerBus(packet);
            m_ReadState = 1;
            return;
        }
        else if(m_ReadState == 1)
        {
            m_Processor->ResetDisplayManagerBus();

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

    switch(m_Bus.ReadAddress)
    {
        case REGISTER_MAGIC: m_Bus.ReadResponse = REGISTER_MAGIC_VALUE; break;
        case REGISTER_REVISION: m_Bus.ReadResponse = REGISTER_REVISION_VALUE; break;
        case REGISTER_EMULATION: m_Bus.ReadResponse = VALUE_REGISTER_EMULATION_SIMULATION; break;
        case REGISTER_RESET: m_Processor->Reset(); break;
        case REGISTER_CONTROL: m_Bus.ReadResponse = m_ControlRegister.Value; break;
        case REGISTER_VRAM_SIZE_LOW: m_Bus.ReadResponse = 256 * 1024 * 1024; break;
        case REGISTER_VRAM_SIZE_HIGH: m_Bus.ReadResponse = 0; break;
        case REGISTER_VGA_WIDTH: m_Bus.ReadResponse = m_VgaWidth; break;
        case REGISTER_VGA_HEIGHT: m_Bus.ReadResponse = m_VgaHeight; break;
        case REGISTER_INTERRUPT_TYPE: m_Bus.ReadResponse = m_CurrentInterruptMessage; break;
        case REGISTER_DEBUG_PRINT: m_Bus.ReadResponse = 0; break;
        default: break;
    }

    m_Bus.ReadBusLocked = 2;
}

void PciControlRegisters::ExecuteWrite() noexcept
{
    if(m_Bus.WriteBusLocked != 1)
    {
        return;
    }

    if(m_DebugWriteCallback)
    {
        m_DebugWriteCallback(m_Bus.WriteAddress, m_Bus.WriteAddress);
    }

    if(m_Bus.WriteAddress >= BASE_REGISTER_DI && m_Bus.WriteAddress < BASE_REGISTER_DI + SIZE_REGISTER_DI * DisplayManager::MaxDisplayCount)
    {
        const u32 offset = m_Bus.WriteAddress - BASE_REGISTER_DI;

        const u32 displayIndex = offset / SIZE_REGISTER_DI;
        const u32 registerOffset = (offset % SIZE_REGISTER_DI) / sizeof(u32);

        if(m_WriteState == 0)
        {
            DisplayDataPacket packet;
            packet.BusActive = true;
            packet.PacketType = 1;
            packet.Read = false;
            packet.DisplayIndex = displayIndex;
            packet.Register = registerOffset;
            packet.Value = &m_Bus.WriteValue;

            m_Processor->SetDisplayManagerBus(packet);
            m_WriteState = 1;
            return;
        }
        else if(m_WriteState == 1)
        {
            m_Processor->ResetDisplayManagerBus();

            m_WriteState = 0;
        }
    }

    switch(m_Bus.WriteAddress)
    {
        case REGISTER_CONTROL: m_ControlRegister.Value = m_Bus.WriteValue & CONTROL_REGISTER_VALID_MASK; break;
        case REGISTER_VGA_WIDTH: m_VgaWidth = static_cast<u16>(m_Bus.WriteValue); break;
        case REGISTER_VGA_HEIGHT: m_VgaHeight = static_cast<u16>(m_Bus.WriteValue); break;
        case REGISTER_INTERRUPT_TYPE: m_CurrentInterruptMessage = 0; break; // The CPU can only clear the interrupt.
        default: break;
    }

    m_Bus.WriteBusLocked = 2;
}
