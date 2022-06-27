#include "StreamingMultiprocessor.hpp"
#include "Processor.hpp"

u32 StreamingMultiprocessor::Read(const u64 address) noexcept
{
    return m_Processor->Read(m_SMIndex, address);
}

void StreamingMultiprocessor::Write(const u64 address, const u32 value) noexcept
{
    m_Processor->Write(m_SMIndex, address, value);
}

void StreamingMultiprocessor::Prefetch(u64 address) noexcept
{
    m_Processor->Prefetch(m_SMIndex, address);
}

