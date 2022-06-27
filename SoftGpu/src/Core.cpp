#include "Core.hpp"
#include "StreamingMultiprocessor.hpp"

u32 FpCore::GetRegister(const u32 dispatchPort, const u32 targetRegister) const noexcept
{
    return m_SM->GetRegister(dispatchPort, targetRegister);
}

void FpCore::SetRegister(const u32 dispatchPort, const u32 targetRegister, const u32 value) noexcept
{
    m_SM->SetRegister(dispatchPort, targetRegister, value);
}

void FpCore::ReportReady() const noexcept
{
    m_SM->ReportCoreReady(m_CoreIndex);
}
