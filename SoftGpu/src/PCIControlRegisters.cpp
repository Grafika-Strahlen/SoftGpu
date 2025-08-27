#include "PCIControlRegisters.hpp"
#include "Processor.hpp"

void PciControlRegisters::ExecuteWriteDMA() noexcept
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

            // m_Parent->DmaSetReady(DMAController::PCI_CONTROLLER_BUS_INDEX_BASE + dmaIndex, true);
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

void PciControlRegisters::HandleDmaBus() noexcept
{
    // if(m_Parent->DmaGetBusState() == DMAController::ReadyForInput)
    // {
    //     const u32 busSelect = m_Parent->DmaGetBusSelect();
    //
    //     if(busSelect >= DMAController::PCI_CONTROLLER_BUS_INDEX_BASE &&
    //        busSelect < DMAController::PCI_CONTROLLER_BUS_INDEX_BASE + DMA_CHANNEL_COUNT)
    //     {
    //         const u32 dmaIndex = busSelect - DMAController::PCI_CONTROLLER_BUS_INDEX_BASE;
    //         const DMAChannelBus& bus = m_DmaBuses[dmaIndex];
    //         m_Parent->DmaSetCPUPhysicalAddress(bus.CPUPhysicalAddress);
    //         m_Parent->DmaSetGPUVirtualAddress(bus.GPUVirtualAddress);
    //         m_Parent->DmaSetWordCount(bus.WordCount);
    //         m_Parent->DmaSetReadWrite(bus.ReadWrite);
    //         m_Parent->DmaSetAtomic(bus.Atomic);
    //         m_Parent->DmaSetActive(bus.Active);
    //         m_Parent->DmaSetReady(busSelect, false);
    //
    //         m_DmaLock[dmaIndex] = VALUE_DMA_LOCK_UNLOCKED;
    //     }
    // }
    // else if(m_Parent->DmaGetBusState() == DMAController::RespondingRequestNumber)
    // {
    //     const u32 busSelect = m_Parent->DmaGetBusSelect();
    //
    //     if(busSelect >= DMAController::PCI_CONTROLLER_BUS_INDEX_BASE &&
    //         busSelect < DMAController::PCI_CONTROLLER_BUS_INDEX_BASE + DMA_CHANNEL_COUNT)
    //     {
    //         const u32 dmaIndex = busSelect - DMAController::PCI_CONTROLLER_BUS_INDEX_BASE;
    //         m_DmaRequestNumbers[dmaIndex] = static_cast<u32>(m_Parent->DmaGetCPUPhysicalAddress());
    //     }
    // }
    // else if(m_Parent->DmaGetBusState() == DMAController::CompletedRequest)
    // {
    //     const u32 completedRequest = static_cast<u32>(m_Parent->DmaGetCPUPhysicalAddress());
    //
    //     for(u32 i = 0; i < DMA_CHANNEL_COUNT; ++i)
    //     {
    //         if(m_DmaRequestNumbers[i] == completedRequest)
    //         {
    //             m_Parent->SetInterrupt(MSG_INTERRUPT_DMA_COMPLETED);
    //             break;
    //         }
    //     }
    // }
}
