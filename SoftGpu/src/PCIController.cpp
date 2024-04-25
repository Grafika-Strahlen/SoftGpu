#include "PCIController.hpp"
#include "Processor.hpp"

void PciController::ExecuteMemRead() noexcept
{
    ::std::lock_guard lock(m_ReadDataMutex);

    if(!m_ReadRequestActive)
    {
        return;
    }

    // ConPrinter::PrintLn(u8"ExecuteMemRead");

    if(!(CommandRegister() & PciController::COMMAND_REGISTER_MEMORY_SPACE_BIT))
    {
        ConPrinter::PrintLn("Attempted to Read over PCI while the Memory Space bit was not set.");
        *m_ReadCountResponse = 0;

        m_ReadRequestActive = false;
        SetEvent(m_SimulationSyncEvent);
        return;
    }

    const u8 bar = GetBARFromAddress(m_ReadRequestAddress);

    if constexpr(false)
    {
        if(bar == PciController::EXPANSION_ROM_BAR_ID)
        {
            ConPrinter::PrintLn("Reading from Expansion ROM {} bytes at 0x{XP0}.", m_ReadRequestSize, m_ReadRequestAddress);
        }
        else
        {
            ConPrinter::PrintLn("Reading from BAR{} {} bytes at 0x{XP0}.", bar, m_ReadRequestSize, m_ReadRequestAddress);
        }
    }

    if(bar == 0xFF)
    {
        *m_ReadCountResponse = 0;

        m_ReadRequestActive = false;
        SetEvent(m_SimulationSyncEvent);
        return;
    }

    const u64 addressOffset = GetBAROffset(m_ReadRequestAddress, bar);

    if(bar == 0)
    {
        if(m_ReadState == 0)
        {
            if(m_Processor->PciControlRegistersBus().ReadBusLocked == 0)
            {
                m_Processor->PciControlRegistersBus().ReadBusLocked = 1;
                m_Processor->PciControlRegistersBus().ReadAddress = static_cast<u32>(addressOffset);
                m_Processor->PciControlRegistersBus().ReadSize = m_ReadRequestSize;
                m_ReadState = 1;
            }
        }
        else if(m_ReadState == 1)
        {
            if(m_Processor->PciControlRegistersBus().ReadBusLocked == 2)
            {
                *m_ReadRequestResponseData = m_Processor->PciControlRegistersBus().ReadResponse;
                *m_ReadCountResponse = 1;
                m_Processor->PciControlRegistersBus().ReadBusLocked = 0;
                m_ReadState = 0;

                m_ReadRequestActive = false;
                SetEvent(m_SimulationSyncEvent);
            }
        }

        // m_ReadRequestData[0] = m_PciRegisters.Read(static_cast<u32>(addressOffset));
        // *m_ReadResponse = 1;

        // SetEvent(m_SimulationSyncEvent);
        return;
    }

    if(bar == 1)
    {
        // Shift right 2 to match the MMU granularity of 4 bytes.
        const u64 realAddress4 = (addressOffset + m_Processor->RamBaseAddress()) >> 2;

        const u16 sizeWords = m_ReadRequestSize / 4;

        for(u16 i = 0; i < sizeWords; ++i)
        {
            // ~Use the real address as that is mapped into the system virtual page.~
            m_ReadRequestResponseData[i] = m_Processor->MemReadPhy(realAddress4 + i);
        }

        *m_ReadCountResponse = m_ReadRequestSize;

        m_ReadRequestActive = false;
        SetEvent(m_SimulationSyncEvent);
        return;
    }

    if(bar == PciController::EXPANSION_ROM_BAR_ID)
    {
        // ConPrinter::PrintLn("Reading from Expansion ROM");

        *m_ReadCountResponse = m_Processor->PciReadExpansionRom(addressOffset, m_ReadRequestSize, m_ReadRequestResponseData);

        m_ReadRequestActive = false;
        SetEvent(m_SimulationSyncEvent);
        return;
    }

    *m_ReadCountResponse = 0;

    m_ReadRequestActive = false;
    SetEvent(m_SimulationSyncEvent);
}

void PciController::ExecuteMemWrite() noexcept
{
    ::std::lock_guard lock(m_WriteDataMutex);

    if(!m_WriteRequestActive)
    {
        // SetEvent(m_SimulationSyncEvent);
        return;
    }

    if(!(CommandRegister() & PciController::COMMAND_REGISTER_MEMORY_SPACE_BIT))
    {
        ConPrinter::PrintLn("Attempted to Write over PCI while the Memory Space bit was not set.");
        m_WriteRequestActive = false;
        SetEvent(m_SimulationSyncEvent);
        return;
    }

    const u8 bar = GetBARFromAddress(m_WriteRequestAddress);

    // ConPrinter::PrintLn("Writing to BAR{} {} bytes at 0x{XP0}.", bar, m_WriteRequestSize, m_WriteRequestAddress);

    if(bar == 0xFF)
    {
        m_WriteRequestActive = false;
        SetEvent(m_SimulationSyncEvent);
        return;
    }

    const u64 addressOffset = GetBAROffset(m_WriteRequestAddress, bar);

    if(bar == 0)
    {
        if(m_WriteState == 0)
        {
            if(m_Processor->PciControlRegistersBus().WriteBusLocked == 0)
            {
                m_Processor->PciControlRegistersBus().WriteBusLocked = 1;
                m_Processor->PciControlRegistersBus().WriteAddress = static_cast<u32>(addressOffset);
                m_Processor->PciControlRegistersBus().WriteSize = m_WriteRequestSize;
                m_Processor->PciControlRegistersBus().WriteValue = static_cast<u32>(*m_WriteRequestData);
                
                m_WriteState = 1;
            }
        }
        else if(m_WriteState == 1)
        {
            if(m_Processor->PciControlRegistersBus().WriteBusLocked == 2)
            {
                m_Processor->PciControlRegistersBus().WriteBusLocked = 0;
                m_WriteState = 0;

                m_WriteRequestActive = false;
                SetEvent(m_SimulationSyncEvent);
            }
        }

        // m_PciRegisters.Write(static_cast<u32>(addressOffset), m_WriteRequestData[0]);
        return;
    }
    else if(bar == 1)
    {
        // Shift right 2 to match the MMU granularity of 4 bytes.
        const u64 realAddress4 = (addressOffset + m_Processor->RamBaseAddress()) >> 2;

        const u16 sizeWords = m_WriteRequestSize / 4;

        for(u16 i = 0; i < sizeWords; ++i)
        {
            // ~Use the real address as that is mapped into the system virtual page.~
            m_Processor->MemWritePhy(realAddress4 + i, m_WriteRequestData[i]);
        }
    }

    m_WriteRequestActive = false;
    SetEvent(m_SimulationSyncEvent);
}
