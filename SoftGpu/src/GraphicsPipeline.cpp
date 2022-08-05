#include "GraphicsPipeline.hpp"
#include "StreamingMultiprocessor.hpp"

u32 GraphicsPipeline::Read(const u64 address) noexcept
{
    return m_SM->Read(address);
}

void GraphicsPipeline::Write(const u64 address, const u32 value) noexcept
{
    m_SM->Write(address, value);
}

void GraphicsPipeline::Prefetch(const u64 address) noexcept
{
    m_SM->Prefetch(address);
}
