#pragma once

#include <Objects.hpp>
#include <NumTypes.hpp>

#define WARP_COUNT (16)
#define WARP_COUNT_BITS (4)

class StreamingMultiprocessor;

struct WarpInfo final
{
    // The pointer to the current instruction. This is only updated after a warp is swapped out. This needs to only be 48/52 bits.
    u64 InstructionPointer : 48;
    u64 RegisterFilePointer : 48;
    u64 RegisterFileResident : 1;
    u64 RegisterFileBase : 12;
    // The total number of registers required for all threads. This uses 1 based indexing.
    u64 TotalRequiredRegisterCount : 11;
    u64 Pad : 8;
    // The number of registers per thread required in the view for this thread warp. This uses 1 based indexing.
    u8 RequiredRegisterCount;
    u8 ThreadEnabledMask;
    u8 ThreadCompletedMask;
};

class WarpScheduler final
{
    DEFAULT_DESTRUCT(WarpScheduler);
    DELETE_CM(WarpScheduler);
public:
    WarpScheduler(StreamingMultiprocessor* const sm, const u32 index) noexcept
        : m_SM(sm)
        , m_Index(index)
        , m_Warps{ }
        , m_CurrentWarp(0)
        , m_ActiveWarpCount(0)
    { }

    void Clock() noexcept;

    void NextWarp(u64 instructionPointer, u8 threadEnabledMask, u8 threadCompletedMask) noexcept;


private:
    StreamingMultiprocessor* m_SM;
    u32 m_Index;

    WarpInfo m_Warps[WARP_COUNT];

    u8 m_CurrentWarp : WARP_COUNT_BITS;
    u8 m_ActiveWarpCount : WARP_COUNT_BITS;
};
