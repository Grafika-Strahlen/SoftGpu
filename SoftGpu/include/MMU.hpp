#pragma once

#include <Objects.hpp>
#include <NumTypes.hpp>
#include <cstring>

class StreamingMultiprocessor;

struct PageEntry final
{
    union
    {
        struct
        {
            // If 1 then this points to a valid entry.
            u64 Present : 1;
            // If 1 then read-write, other read-only
            u64 ReadWrite : 1;
            // If 1 then contains executable code. ReadWrite must be 0.
            u64 Execute : 1;
            // If 1 then perform cache-writethrough, else use writeback.
            u64 WriteThrough : 1;
            // If 1 then no caching will be performed
            u64 CacheDisable : 1;
            // If 1 then this page has been accessed.
            u64 Accessed : 1;
            // If 1 then this page has been modified.
            u64 Dirty : 1;
            // If 1 then this external memory (likely on the motherboard).
            u64 External : 1;
            u64 PhysicalAddress : 48;
            u64 Reserved1 : 8;
        };
        u64 Value;
    };
};

static_assert(sizeof(PageEntry) == 8, "Page Entry is not 8 bytes long.");

static inline constexpr u64 GpuPageSize = 65536;

class Mmu final
{
    DEFAULT_DESTRUCT(Mmu);
    DELETE_CM(Mmu);
public:
    Mmu(StreamingMultiprocessor* const sm) noexcept
        : m_SM(sm)
        , m_PageDirectoryPhysicalAddress(0)
        , m_CachedTableIndex(0)
        , m_CacheDirty(true)
        , m_ValueLoaded(false)
        , m_PageDirectoryCache{ }
        , m_PageTableCache{ }
    { }

    void Reset()
    {
        m_PageDirectoryPhysicalAddress = 0;
        m_CachedTableIndex = 0;
        m_CacheDirty = true;
        m_ValueLoaded = false;
    }

    [[nodiscard]] u64 TranslateAddress(u64 virtualAddress, bool* success, bool* readWrite, bool* execute, bool* writeThrough, bool* cacheDisable, bool* external) noexcept;

    void MarkDirty(u64 virtualAddress) noexcept;

    void LoadPageDirectoryPointer(const u64 pageDirectoryPhysicalAddress) noexcept
    {
        m_PageDirectoryPhysicalAddress = pageDirectoryPhysicalAddress;
        m_ValueLoaded = true;
        FlushCache();
    }

    void FlushCache() noexcept
    {
        if(!m_ValueLoaded)
        {
            return;
        }

        // Translate the address to be in bytes.
        const u64 pageDirectoryAddress = m_PageDirectoryPhysicalAddress << 16;
        // Convert the address to a C++ Pointer.
        void* const pageDirectoryPtr = reinterpret_cast<void*>(pageDirectoryAddress);

        // Copy the memory into our cache.
        (void) ::std::memcpy(m_PageDirectoryCache, pageDirectoryPtr, GpuPageSize);

        // Is m_CachedTableIndex valid?
        if(!m_CacheDirty)
        {
            // Is the target table present?
            if(m_PageDirectoryCache[m_CachedTableIndex].Present)
            {
                // Translate the address to be in bytes.
                const u64 pageTableAddress = m_PageDirectoryCache[m_CachedTableIndex].PhysicalAddress << 16;
                // Convert the address to a C++ Pointer.
                void* const pageTablePtr = reinterpret_cast<void*>(pageTableAddress);

                // Copy the memory into our cache.
                (void) ::std::memcpy(m_PageTableCache, pageTablePtr, GpuPageSize);
            }
            else
            {
                m_CacheDirty = true;
            }
        }
    }
private:
    StreamingMultiprocessor* m_SM;
    u64 m_PageDirectoryPhysicalAddress : 48;
    u64 m_CachedTableIndex : 14;
    u64 m_CacheDirty : 1;
    u64 m_ValueLoaded : 1;
    // Cache the entire page directory.
    PageEntry m_PageDirectoryCache[GpuPageSize / sizeof(PageEntry)];
    // Cache a single page table.
    PageEntry m_PageTableCache[GpuPageSize / sizeof(PageEntry)];
};
