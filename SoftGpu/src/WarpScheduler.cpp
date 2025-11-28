/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#include "WarpScheduler.hpp"
#include "StreamingMultiprocessor.hpp"

void WarpScheduler::Clock() noexcept
{
}

void WarpScheduler::NextWarp(const u64 instructionPointer, const u8 threadEnabledMask, const u8 threadCompletedMask) noexcept
{
    {
        m_Warps[m_CurrentWarp].InstructionPointer = instructionPointer;
        m_Warps[m_CurrentWarp].ThreadEnabledMask = threadEnabledMask;
        m_Warps[m_CurrentWarp].ThreadCompletedMask = threadCompletedMask;
        
        const u64 storageAddress = m_Warps[m_CurrentWarp].RegisterFilePointer;
        u32* const storagePointer = reinterpret_cast<u32*>(storageAddress);

        for(u32 i = 0; i <= m_Warps[m_CurrentWarp].TotalRequiredRegisterCount; ++i)
        {
            (void) storagePointer;
            // TODO: FIX
            // storagePointer[i] = m_SM->GetRegister(m_Warps[m_CurrentWarp].RegisterFileBase + i);
        }

        m_SM->FreeRegisters(m_Warps[m_CurrentWarp].RegisterFileBase, m_Warps[m_CurrentWarp].TotalRequiredRegisterCount);
    }

    // (m_CurrentWarp + 1) % WARP_COUNT
    const u8 nextIndex = (m_CurrentWarp + 1) & (WARP_COUNT - 1);

    if(!m_Warps[nextIndex].RegisterFileResident)
    {
        const u64 storageAddress = m_Warps[nextIndex].RegisterFilePointer;
        const u32* const storagePointer = reinterpret_cast<u32*>(storageAddress);

        m_Warps[nextIndex].RegisterFileBase = m_SM->AllocateRegisters(m_Warps[nextIndex].TotalRequiredRegisterCount);

        for(u32 i = 0; i <= m_Warps[m_CurrentWarp].TotalRequiredRegisterCount; ++i)
        {
            (void) storagePointer;
            // TODO: FIX
            // m_SM->SetRegister(m_Warps[nextIndex].RegisterFileBase + i, storagePointer[i]);
        }
    }

    m_CurrentWarp = nextIndex;

    u16 baseRegisters[8];

    (void) ::std::memset(baseRegisters, 0xFF, sizeof(baseRegisters));

    // Iterate through all enabled threads and assign base registers.
    u8 threadMask = m_Warps[nextIndex].ThreadEnabledMask;
    u32 registerIndex = 0; // Only needs to be 3 bits
    u16 currentBaseRegister = m_Warps[nextIndex].RegisterFileBase;
    // This would be better done by checking all bits of the mask simultaneously.
    while(threadMask)
    {
        if((threadMask & 0x1) != 0)
        {
            baseRegisters[registerIndex] = currentBaseRegister;
            currentBaseRegister += m_Warps[nextIndex].RequiredRegisterCount;
        }

        threadMask >>= 1;
        ++registerIndex;
    }

    m_SM->LoadWarp(m_Index, m_Warps[nextIndex].ThreadEnabledMask, m_Warps[nextIndex].ThreadCompletedMask, baseRegisters, m_Warps[nextIndex].InstructionPointer);
}
