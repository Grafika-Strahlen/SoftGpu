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

        // Iterate through all enabled threads and store register blocks if necessary.
        u8 threadMask = m_Warps[m_CurrentWarp].ThreadEnabledMask;
        u32 registerIndex = 0;
        while(threadMask)
        {
            if((threadMask & 0x1) != 0)
            {
                const u64 storageAddress = (m_Warps[m_CurrentWarp].RegisterFilePointer & 0x7FFFFFFFFFFFFFFF) + static_cast<u64>(m_Warps[m_CurrentWarp].RequiredRegisterCount) * registerIndex;
                u32* const storagePointer = reinterpret_cast<u32*>(static_cast<uPtr>(storageAddress));

                for(u32 i = 0; i < m_Warps[m_CurrentWarp].RequiredRegisterCount; ++i)
                {
                    storagePointer[i] = m_SM->GetRegister(m_Warps[m_CurrentWarp].RegisterFileBase[registerIndex] + i);
                }

                m_SM->FreeRegisters(m_Warps[m_CurrentWarp].RequiredRegisterCount, m_Warps[m_CurrentWarp].RegisterFileBase[registerIndex]);
            }

            threadMask >>= 1;
            ++registerIndex;
        }
    }

    const u32 nextIndex = (++m_Index) % WARP_COUNT;

    if((m_Warps[nextIndex].RegisterFilePointer & 0x8000000000000000) == 0)
    {
        // Iterate through all enabled threads and allocate register blocks if necessary.
        u8 threadMask = m_Warps[nextIndex].ThreadEnabledMask;
        u32 registerIndex = 0;
        while(threadMask)
        {
            if((threadMask & 0x1) != 0)
            {
                m_Warps[nextIndex].RegisterFileBase[0] = m_SM->AllocateRegisters(m_Warps[nextIndex].RequiredRegisterCount);

                const u64 storageAddress = (m_Warps[nextIndex].RegisterFilePointer & 0x7FFFFFFFFFFFFFFF) + static_cast<u64>(m_Warps[nextIndex].RequiredRegisterCount) * registerIndex;
                const u32* const storagePointer = reinterpret_cast<const u32*>(static_cast<uPtr>(storageAddress));

                for(u32 i = 0; i < m_Warps[nextIndex].RequiredRegisterCount; ++i)
                {
                    m_SM->SetRegister(m_Warps[nextIndex].RegisterFileBase[registerIndex] + i, storagePointer[i]);
                }
            }

            threadMask >>= 1;
            ++registerIndex;
        }
    }

    m_CurrentWarp = nextIndex;

    m_SM->LoadWarp(m_Index, m_Warps[nextIndex].ThreadEnabledMask, m_Warps[nextIndex].ThreadCompletedMask, m_Warps[nextIndex].RegisterFileBase, m_Warps[nextIndex].InstructionPointer);
}
