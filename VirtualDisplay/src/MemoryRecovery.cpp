/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#include "vd/MemoryRecovery.hpp"
#include <NumTypes.hpp>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#endif

namespace tau::vd {

static constexpr uSys SacrificialBlockPageCount = 1024;

static void* s_SacrificialMemory = nullptr;

#ifdef _WIN32
static void InitSacrificialMemoryWindows() noexcept
{
    if(!s_SacrificialMemory)
    {
        HANDLE currentModule = GetModuleHandleW(nullptr);

        uSys minimumWorkingSetSize;
        uSys maximumWorkingSetSize;
        DWORD flags;

        if(!GetProcessWorkingSetSizeEx(currentModule, &minimumWorkingSetSize, &maximumWorkingSetSize, &flags))
        {
            return;
        }

        if(minimumWorkingSetSize < 96)
        {
            minimumWorkingSetSize = 96;
        }

        if(maximumWorkingSetSize < 4096)
        {
            maximumWorkingSetSize = 4096;
        }

        if((flags & QUOTA_LIMITS_HARDWS_MIN_ENABLE) != QUOTA_LIMITS_HARDWS_MIN_ENABLE)
        {
            flags &= ~(QUOTA_LIMITS_HARDWS_MIN_DISABLE);
            flags |= QUOTA_LIMITS_HARDWS_MIN_ENABLE;
        }

        if((flags & QUOTA_LIMITS_HARDWS_MAX_DISABLE) != QUOTA_LIMITS_HARDWS_MAX_DISABLE)
        {
            flags &= ~(QUOTA_LIMITS_HARDWS_MAX_ENABLE);
            flags |= QUOTA_LIMITS_HARDWS_MAX_DISABLE;
        }


        minimumWorkingSetSize += SacrificialBlockPageCount;
        maximumWorkingSetSize += SacrificialBlockPageCount;

        if(!SetProcessWorkingSetSizeEx(currentModule, minimumWorkingSetSize, maximumWorkingSetSize, flags))
        {
            return;
        }

        s_SacrificialMemory = VirtualAlloc(nullptr, SacrificialBlockPageCount * 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READONLY);

        if(!s_SacrificialMemory)
        {
            return;
        }

        VirtualLock(s_SacrificialMemory, SacrificialBlockPageCount * 4096);
    }
}
#else
static void InitSacrificialMemoryLinux() noexcept
{
    if(!s_SacrificialMemory)
    {
    }
}
#endif

void InitSacrificialMemory() noexcept
{
#ifdef _WIN32
    InitSacrificialMemoryWindows();
#else
    InitSacrificialMemoryLinux();
#endif
}

void RecoverSacrificialMemory() noexcept
{
#ifdef _WIN32
    if(s_SacrificialMemory)
    {
        VirtualFree(s_SacrificialMemory, SacrificialBlockPageCount * 4096, MEM_DECOMMIT | MEM_RELEASE);
        s_SacrificialMemory = nullptr;
    }
#endif
}

}
