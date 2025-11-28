/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
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

    if(!(m_ConfigData.ConfigHeader.Command & PciController::COMMAND_REGISTER_MEMORY_SPACE_BIT))
    {
        ConPrinter::PrintLn("Attempted to Read over PCI while the Memory Space bit was not set.");
        *m_ReadCountResponse = 0;

        m_ReadRequestActive = false;
#ifdef _WIN32
        SetEvent(m_SimulationSyncEvent);
#endif
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
#ifdef _WIN32
        SetEvent(m_SimulationSyncEvent);
#endif
        return;
    }

    [[maybe_unused]] const u64 addressOffset = GetBAROffset(m_ReadRequestAddress, bar);

    if(bar == 0)
    {
        if(m_ReadState == 0)
        {
            // if(m_Parent->PciControlRegistersBus().ReadBusLocked == 0)
            // {
            //     m_Parent->PciControlRegistersBus().ReadBusLocked = 1;
            //     m_Parent->PciControlRegistersBus().ReadAddress = static_cast<u32>(addressOffset);
            //     m_Parent->PciControlRegistersBus().ReadSize = m_ReadRequestSize;
            //     m_ReadState = 1;
            // }
        }
        else if(m_ReadState == 1)
        {
//             if(m_Parent->PciControlRegistersBus().ReadBusLocked == 2)
//             {
//                 *m_ReadRequestResponseData = m_Parent->PciControlRegistersBus().ReadResponse;
//                 *m_ReadCountResponse = 1;
//                 m_Parent->PciControlRegistersBus().ReadBusLocked = 0;
//                 m_ReadState = 0;
//
//                 m_ReadRequestActive = false;
// #ifdef _WIN32
//                 SetEvent(m_SimulationSyncEvent);
// #endif
//             }
        }

        // m_ReadRequestData[0] = m_PciRegisters.Read(static_cast<u32>(addressOffset));
        // *m_ReadResponse = 1;

        // SetEvent(m_SimulationSyncEvent);
        return;
    }

    if(bar == 1)
    {
        // Shift right 2 to match the MMU granularity of 4 bytes.
        // const u64 realAddress4 = (addressOffset + m_Parent->RamBaseAddress()) >> 2;

        const u16 sizeWords = m_ReadRequestSize / 4;

        for(u16 i = 0; i < sizeWords; ++i)
        {
            // ~Use the real address as that is mapped into the system virtual page.~
            // m_ReadRequestResponseData[i] = m_Parent->MemReadPhy(realAddress4 + i);
        }

        *m_ReadCountResponse = m_ReadRequestSize;

        m_ReadRequestActive = false;
#ifdef _WIN32
        SetEvent(m_SimulationSyncEvent);
#endif
        return;
    }

    if(bar == PciController::EXPANSION_ROM_BAR_ID)
    {
        // ConPrinter::PrintLn("Reading from Expansion ROM");

        // *m_ReadCountResponse = m_Parent->PciReadExpansionRom(addressOffset, m_ReadRequestSize, m_ReadRequestResponseData);

        m_ReadRequestActive = false;
#ifdef _WIN32
        SetEvent(m_SimulationSyncEvent);
#endif
        return;
    }

    *m_ReadCountResponse = 0;

    m_ReadRequestActive = false;
#ifdef _WIN32
    SetEvent(m_SimulationSyncEvent);
#endif
}

void PciController::ExecuteMemWrite() noexcept
{
    ::std::lock_guard lock(m_WriteDataMutex);

    if(!m_WriteRequestActive)
    {
        // SetEvent(m_SimulationSyncEvent);
        return;
    }

    if(!(m_ConfigData.ConfigHeader.Command & PciController::COMMAND_REGISTER_MEMORY_SPACE_BIT))
    {
        ConPrinter::PrintLn("Attempted to Write over PCI while the Memory Space bit was not set.");
        m_WriteRequestActive = false;
#ifdef _WIN32
        SetEvent(m_SimulationSyncEvent);
#endif
        return;
    }

    const u8 bar = GetBARFromAddress(m_WriteRequestAddress);

    // ConPrinter::PrintLn("Writing to BAR{} {} bytes at 0x{XP0}.", bar, m_WriteRequestSize, m_WriteRequestAddress);

    if(bar == 0xFF)
    {
        m_WriteRequestActive = false;
#ifdef _WIN32
        SetEvent(m_SimulationSyncEvent);
#endif
        return;
    }

    [[maybe_unused]] const u64 addressOffset = GetBAROffset(m_WriteRequestAddress, bar);

    if(bar == 0)
    {
        if(m_WriteState == 0)
        {
            // if(m_Parent->PciControlRegistersBus().WriteBusLocked == 0)
            // {
            //     m_Parent->PciControlRegistersBus().WriteBusLocked = 1;
            //     m_Parent->PciControlRegistersBus().WriteAddress = static_cast<u32>(addressOffset);
            //     m_Parent->PciControlRegistersBus().WriteSize = m_WriteRequestSize;
            //     m_Parent->PciControlRegistersBus().WriteValue = static_cast<u32>(*m_WriteRequestData);
            //
            //     m_WriteState = 1;
            // }
        }
        else if(m_WriteState == 1)
        {
//             if(m_Parent->PciControlRegistersBus().WriteBusLocked == 2)
//             {
//                 m_Parent->PciControlRegistersBus().WriteBusLocked = 0;
//                 m_WriteState = 0;
//
//                 m_WriteRequestActive = false;
// #ifdef _WIN32
//                 SetEvent(m_SimulationSyncEvent);
// #endif
//             }
        }

        // m_PciRegisters.Write(static_cast<u32>(addressOffset), m_WriteRequestData[0]);
        return;
    }
    else if(bar == 1)
    {
        // Shift right 2 to match the MMU granularity of 4 bytes.
        // const u64 realAddress4 = (addressOffset + m_Parent->RamBaseAddress()) >> 2;

        const u16 sizeWords = m_WriteRequestSize / 4;

        for(u16 i = 0; i < sizeWords; ++i)
        {
            // ~Use the real address as that is mapped into the system virtual page.~
            // m_Parent->MemWritePhy(realAddress4 + i, m_WriteRequestData[i]);
        }
    }

    m_WriteRequestActive = false;
#ifdef _WIN32
    SetEvent(m_SimulationSyncEvent);
#endif
}
