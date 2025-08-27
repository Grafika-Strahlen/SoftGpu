/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#include "MMU.hpp"
#include "StreamingMultiprocessor.hpp"

u64 Mmu::TranslateAddress(const u64 virtualAddress, bool* const success, bool* const readWrite, bool* const execute, bool* const writeThrough, bool* const cacheDisable, bool* const external) noexcept
{
    if(!m_ValueLoaded)
    {
        if(success)
        {
            *success = true;
        }
        // Return the optional page info bits. These will be done with out ports.
        if(readWrite)
        {
            *readWrite = true;
        }
        if(execute)
        {
            *execute = false;
        }
        if(writeThrough)
        {
            *writeThrough = false;
        }
        if(cacheDisable)
        {
            *cacheDisable = false;
        }
        if(external)
        {
            *external = false;
        }
        return virtualAddress;
    }

    if((virtualAddress & 0xFFFF000000000000) != 0)
    {
        if(success)
        {
            *success = false;
        }
        // Return the optional page info bits. These will be done without ports.
        if(readWrite)
        {
            *readWrite = false;
        }
        if(execute)
        {
            *execute = false;
        }
        if(writeThrough)
        {
            *writeThrough = false;
        }
        if(cacheDisable)
        {
            *cacheDisable = false;
        }
        if(external)
        {
            *external = false;
        }
        return 0xFFFFFFFFFFFFFFFF;
    }

    // And by 0x3FFF instead of 0xFFFF because of 4 byte granularity
    const u64 pageBits = virtualAddress & 0x3FFF;
    // Shift by 14 instead of 16 because of 4 byte granularity
    const u64 pageTableBits = (virtualAddress >> 14) & 0xFFFF;
    // Shift by 30 instead of 32 because of 4 byte granularity
    const u64 pageDirectoryBits = (virtualAddress >> 30) & 0xFFFF;

    if(m_CacheDirty || pageDirectoryBits != m_CachedTableIndex)
    {
        // Translate the address to be in bytes.
        const u64 pageTableAddress = m_PageDirectoryCache[pageDirectoryBits].PhysicalAddress << 16;
        // Convert the address to a C++ Pointer.
        void* const pageTablePtr = reinterpret_cast<void*>(pageTableAddress);

        // Copy the memory into our cache.
        (void) ::std::memcpy(m_PageTableCache, pageTablePtr, GpuPageSize);

        m_CachedTableIndex = pageDirectoryBits;
    }

    if(m_PageTableCache[pageTableBits].Present)
    {
        if(!m_PageTableCache[pageTableBits].Accessed)
        {
            // Translate the address to be in bytes.
            const u64 pageTableAddress = m_PageDirectoryCache[pageDirectoryBits].PhysicalAddress << 16;

            const u64 pageEntryAddress = pageTableAddress + pageTableBits * sizeof(PageEntry);

            // Flag that the page was accessed.
            m_PageTableCache[pageTableBits].Accessed = true;
            m_SM->WriteMmuPageInfo(pageEntryAddress >> 2, m_PageTableCache[pageTableBits].Value);
        }

        if(success)
        {
            *success = true;
        }
        // Return the optional page info bits. These will be done with out ports.
        if(readWrite)
        {
            *readWrite = m_PageTableCache[pageTableBits].ReadWrite;
        }
        if(execute)
        {
            *execute = m_PageTableCache[pageTableBits].Execute;
        }
        if(writeThrough)
        {
            *writeThrough = m_PageTableCache[pageTableBits].WriteThrough;
        }
        if(cacheDisable)
        {
            *cacheDisable = m_PageTableCache[pageTableBits].CacheDisable;
        }
        if(external)
        {
            *external = m_PageTableCache[pageTableBits].External;
        }

        return (m_PageTableCache[pageTableBits].PhysicalAddress << 14) | pageBits;
    }
    else
    {
        if(success)
        {
            *success = false;
        }
        return 0xFFFFFFFFFFFFFFFF;
    }
}

void Mmu::MarkDirty(const u64 virtualAddress) noexcept
{
    if(!m_ValueLoaded)
    {
        return;
    }
    
    // Shift by 14 instead of 16 because of 4 byte granularity
    const u64 pageTableBits = (virtualAddress >> 14) & 0xFFFF;
    // Shift by 30 instead of 32 because of 4 byte granularity
    const u64 pageDirectoryBits = (virtualAddress >> 30) & 0xFFFF;

    if(m_CacheDirty || pageDirectoryBits != m_CachedTableIndex)
    {
        // Translate the address to be in bytes.
        const u64 pageTableAddress = m_PageDirectoryCache[pageDirectoryBits].PhysicalAddress << 16;
        // Convert the address to a C++ Pointer.
        void* const pageTablePtr = reinterpret_cast<void*>(pageTableAddress);

        // Copy the memory into our cache.
        (void) ::std::memcpy(m_PageTableCache, pageTablePtr, GpuPageSize);

        m_CachedTableIndex = pageDirectoryBits;
    }

    if(!m_PageTableCache[pageTableBits].Dirty)
    {
        // Translate the address to be in bytes.
        const u64 pageTableAddress = m_PageDirectoryCache[pageDirectoryBits].PhysicalAddress << 16;

        const u64 pageEntryAddress = pageTableAddress + pageTableBits * sizeof(PageEntry);

        // Flag that the page was accessed.
        m_PageTableCache[pageTableBits].Dirty = true;
        m_SM->WriteMmuPageInfo(pageEntryAddress >> 2, m_PageTableCache[pageTableBits].Value);
    }
}
