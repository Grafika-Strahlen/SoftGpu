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
    else if(m_Bus.WriteAddress >= BASE_REGISTER_DMA && m_Bus.WriteAddress < BASE_REGISTER_DMA + SIZE_REGISTER_DMA * DMAController::DMA_CHANNEL_COUNT)
    {
        const u32 offset = m_Bus.WriteAddress - BASE_REGISTER_DI;

        const u32 displayIndex = offset / SIZE_REGISTER_DI;
        const u32 registerOffset = (offset % SIZE_REGISTER_DI) / sizeof(u32);

        if(registerOffset == OFFSET_REGISTER_DMA_LOCK)
        {
            m_Bus.ReadResponse = m_DmaLock[displayIndex];
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

void PciControlRegisters::ExecuteWrite() noexcept
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
    else if(m_Bus.WriteAddress >= BASE_REGISTER_DMA && m_Bus.WriteAddress < BASE_REGISTER_DMA + SIZE_REGISTER_DMA * DMAController::DMA_CHANNEL_COUNT)
    {
        const u32 offset = m_Bus.WriteAddress - BASE_REGISTER_DMA;

        const u32 dmaIndex = offset / SIZE_REGISTER_DMA;
        const u32 registerOffset = (offset % SIZE_REGISTER_DMA) / sizeof(u32);

        DMAChannelBus& bus = m_DmaBuses[dmaIndex];
        
        switch(registerOffset)
        {
            case OFFSET_REGISTER_DMA_CPU_LOW:
            {
                u64 cpuAddress = bus.CPUPhysicalAddress & 0xFFFFFFFF'00000000ULL;
                cpuAddress |= m_Bus.WriteValue;
                bus.CPUPhysicalAddress = cpuAddress;
                break;
            }
            case OFFSET_REGISTER_DMA_CPU_HIGH:
            {
                u64 cpuAddress = bus.CPUPhysicalAddress & 0x00000000'FFFFFFFFULL;
                cpuAddress |= static_cast<u64>(m_Bus.WriteValue) << 32;
                bus.CPUPhysicalAddress = cpuAddress;
                break;
            }
            case OFFSET_REGISTER_DMA_GPU_LOW:
            {
                u64 gpuAddress = bus.GPUVirtualAddress & 0xFFFFFFFF'00000000ULL;
                gpuAddress |= m_Bus.WriteValue;
                bus.CPUPhysicalAddress = gpuAddress;
                break;
            }
            case OFFSET_REGISTER_DMA_GPU_HIGH:
            {
                u64 gpuAddress = bus.GPUVirtualAddress & 0x00000000'FFFFFFFFULL;
                gpuAddress |= static_cast<u64>(m_Bus.WriteValue) << 32;
                bus.GPUVirtualAddress = gpuAddress;
                break;
            }
            case OFFSET_REGISTER_DMA_SIZE_LOW:
            {
                u64 size = bus.WordCount & 0xFFFFFFFF'00000000ULL;
                size |= m_Bus.WriteValue;
                bus.WordCount = size;
                break;
            }
            case OFFSET_REGISTER_DMA_SIZE_HIGH:
            {
                u64 size = bus.WordCount & 0x00000000'FFFFFFFFULL;
                size |= static_cast<u64>(m_Bus.WriteValue) << 32;
                bus.WordCount = size;
                break;
            }
            case OFFSET_REGISTER_DMA_CONTROL:
            {
                bus.ReadWrite = m_Bus.WriteValue & 0x00000001;
                bus.Atomic = m_Bus.WriteValue & 0x00000002;
                bus.Active = 1;

                m_Processor->DmaSetReady(DMAController::PCI_CONTROLLER_BUS_INDEX_BASE + dmaIndex, true);
                break;
            }
            case OFFSET_REGISTER_DMA_LOCK:
            {
                if(m_DmaLock[dmaIndex] == VALUE_DMA_LOCK_UNLOCKED)
                {
                    m_DmaLock[dmaIndex] = m_Bus.WriteValue;
                }
                break;
            }
            default: break;
        }
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

void PciControlRegisters::HandleDmaBus() noexcept
{
    if(m_Processor->DmaGetBusState() == DMAController::ReadyForInput)
    {
        const u32 busSelect = m_Processor->DmaGetBusSelect();

        if(busSelect >= DMAController::PCI_CONTROLLER_BUS_INDEX_BASE && 
           busSelect < DMAController::PCI_CONTROLLER_BUS_INDEX_BASE + DMA_CHANNEL_COUNT)
        {
            const u32 dmaIndex = busSelect - DMAController::PCI_CONTROLLER_BUS_INDEX_BASE;
            const DMAChannelBus& bus = m_DmaBuses[dmaIndex];
            m_Processor->DmaSetCPUPhysicalAddress(bus.CPUPhysicalAddress);
            m_Processor->DmaSetGPUVirtualAddress(bus.GPUVirtualAddress);
            m_Processor->DmaSetWordCount(bus.WordCount);
            m_Processor->DmaSetReadWrite(bus.ReadWrite);
            m_Processor->DmaSetAtomic(bus.Atomic);
            m_Processor->DmaSetActive(bus.Active);
            m_Processor->DmaSetReady(busSelect, false);

            m_DmaLock[dmaIndex] = VALUE_DMA_LOCK_UNLOCKED;
        }
    }
    else if(m_Processor->DmaGetBusState() == DMAController::RespondingRequestNumber)
    {
        const u32 busSelect = m_Processor->DmaGetBusSelect();

        if(busSelect >= DMAController::PCI_CONTROLLER_BUS_INDEX_BASE &&
            busSelect < DMAController::PCI_CONTROLLER_BUS_INDEX_BASE + DMA_CHANNEL_COUNT)
        {
            const u32 dmaIndex = busSelect - DMAController::PCI_CONTROLLER_BUS_INDEX_BASE;
            m_DmaRequestNumbers[dmaIndex] = static_cast<u32>(m_Processor->DmaGetCPUPhysicalAddress());
        }
    }
    else if(m_Processor->DmaGetBusState() == DMAController::CompletedRequest)
    {
        const u32 completedRequest = static_cast<u32>(m_Processor->DmaGetCPUPhysicalAddress());

        for(u32 i = 0; i < DMA_CHANNEL_COUNT; ++i)
        {
            if(m_DmaRequestNumbers[i] == completedRequest)
            {
                m_Processor->SetInterrupt(MSG_INTERRUPT_DMA_COMPLETED);
                break;
            }
        }
    }
}
