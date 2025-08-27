/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <Objects.hpp>
#include <NumTypes.hpp>

class StreamingMultiprocessor;

class GraphicsPipeline final
{
    DEFAULT_DESTRUCT(GraphicsPipeline);
    DELETE_CM(GraphicsPipeline);
public:
    GraphicsPipeline(StreamingMultiprocessor* const m_sm) noexcept
        : m_SM(m_sm)
        , m_VertexShaderPtr(0)
        , m_TessellationControlShaderPtr(0)
        , m_TessellationEvalShaderPtr(0)
        , m_GeometryShaderPtr(0)
        , m_PixelShaderPtr(0)
        , m_VertexShaderMaxRegisters(0)
        , m_TessellationControlShaderMaxRegisters(0)
        , m_TessellationEvalShaderMaxRegisters(0)
        , m_GeometryShaderMaxRegisters(0)
        , m_PixelShaderMaxRegisters(0)
    { }

    void StoreIAWord(const u32 word, const u32 address) noexcept
    {
        
    }

    [[nodiscard]] u32 Read(u64 address) noexcept;
    void Write(u64 address, u32 value) noexcept;
    void Prefetch(u64 address) noexcept;
private:
    StreamingMultiprocessor* m_SM;

    u64 m_VertexShaderPtr;
    u64 m_TessellationControlShaderPtr;
    u64 m_TessellationEvalShaderPtr;
    u64 m_GeometryShaderPtr;
    u64 m_PixelShaderPtr;

    u8 m_VertexShaderMaxRegisters;
    u8 m_TessellationControlShaderMaxRegisters;
    u8 m_TessellationEvalShaderMaxRegisters;
    u8 m_GeometryShaderMaxRegisters;
    u8 m_PixelShaderMaxRegisters;
};
