/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
// ReSharper disable CppClangTidyClangDiagnosticUnusedPrivateField
#pragma once

#include <NumTypes.hpp>
#include <Objects.hpp>

class StreamingMultiprocessor;

class CommandListDispatcher final
{
    DEFAULT_DESTRUCT(CommandListDispatcher);
    DELETE_CM(CommandListDispatcher);
public:
    CommandListDispatcher(StreamingMultiprocessor* const sm) noexcept
        : m_SM(sm)
        , m_CommandListPtr(0)
        , m_Running(false)
        , m_Reserved{ }
    { }

    void Reset()
    {
        m_CommandListPtr = 0;
        m_Running = false;
    }

    void Clock()
    {
        
    }

    void LoadCommandList(const u64 commandListPtr) noexcept
    {
        m_CommandListPtr = commandListPtr;
        m_Running = true;
    }

    void Stop() noexcept
    {
        m_Running = false;
    }
private:
    StreamingMultiprocessor* m_SM;
    u64 m_CommandListPtr : 48;
    u64 m_Running : 1;
    u64 m_Reserved : 15;
};
