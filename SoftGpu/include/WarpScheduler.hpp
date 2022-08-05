#pragma once

#include <Objects.hpp>
#include <NumTypes.hpp>

#define WARP_COUNT (16)

class StreamingMultiprocessor;

struct WarpInfo final
{
    // The pointer to the current instruction. This is only updated after a warp is swapped out. This needs to only be 48/52 bits.
    u64 InstructionPointer;
    // If the high bit is 1 then the register file is resident. This needs to only be 48/52 bits.
    u64 RegisterFilePointer;
    u16 RegisterFileBase[8];
    // The number of registers required in the view for this thread warp. This uses 1 based indexing.
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
        , m_CurrentWarp(0)
        , m_Warps{ }
    { }

    void Clock() noexcept;

    void NextWarp(u64 instructionPointer, u8 threadEnabledMask, u8 threadCompletedMask) noexcept;
private:
    StreamingMultiprocessor* m_SM;
    u32 m_Index;

    u32 m_CurrentWarp;
    WarpInfo m_Warps[WARP_COUNT];
};
