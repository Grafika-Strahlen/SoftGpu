/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#include "DMAController.hpp"
#include "Processor.hpp"
#include <algorithm>

#ifdef min
#undef min
#endif

void DMAChannel::ExecuteRead() noexcept
{
    if(!p_Active)
    {
        return;
    }

    if(p_WordCount == 0)
    {
        return;
    }

    if(m_WordsTransferred >= p_WordCount)
    {
        p_WordCount = 0;
    }

    const u64 wordsRemaining = p_WordCount - m_WordsTransferred;

    const u64 wordsToTransfer = ::std::min(wordsRemaining, static_cast<u64>(m_Processor->PCITransferLimit()));

    m_WordsInTransferBlock = static_cast<u16>(wordsToTransfer);

    if(p_ReadWrite)
    {
        m_Processor->PciBusRead(p_CPUPhysicalAddress, static_cast<u16>(wordsToTransfer), m_TransferBlock);
    }
    else
    {
        for(u64 i = 0; i < wordsToTransfer; ++i)
        {
            m_TransferBlock[i] = m_Processor->MemReadPhy(p_GPUVirtualAddress + i);
        }
    }
}

void DMAChannel::ExecuteWrite() noexcept
{
    if(!p_Active)
    {
        return;
    }

    if(p_WordCount == 0)
    {
        return;
    }

    if(m_WordsTransferred >= p_WordCount)
    {
        p_WordCount = 0;
    }

    if(p_ReadWrite)
    {
        for(u64 i = 0; i < static_cast<u64>(m_WordsInTransferBlock); ++i)
        {
            m_Processor->MemWritePhy(p_GPUVirtualAddress + i, m_TransferBlock[i]);
        }
    }
    else
    {
        m_Processor->PciBusWrite(p_CPUPhysicalAddress, static_cast<u16>(m_WordsInTransferBlock), m_TransferBlock);
    }

    m_WordsTransferred += m_WordsInTransferBlock;
    m_WordsInTransferBlock = 0;
    if(m_WordsTransferred >= p_WordCount)
    {
        p_WordCount = 0;
        p_Active = 0;
        // m_Processor->UnlockDma(m_DmaIndex);
        p_out_Finished = 1;
    }
}
