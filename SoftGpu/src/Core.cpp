#include "Core.hpp"
#include "StreamingMultiprocessor.hpp"

u32 FpCore::GetRegister(const u32 dispatchPort, const u32 replicationIndex, const u32 targetRegister) const noexcept
{
    return m_SM->GetRegister(dispatchPort, replicationIndex, targetRegister);
}

void FpCore::SetRegister(const u32 dispatchPort, const u32 replicationIndex, const u32 targetRegister, const u32 value) noexcept
{
    m_SM->SetRegister(dispatchPort, replicationIndex, targetRegister, value);
}

void FpCore::ReportReady() const noexcept
{
    m_SM->ReportFpCoreReady(m_UnitIndex);
}

void FpCore::ReleaseRegisterContestation(const u32 dispatchPort, u32 replicationIndex, const u32 registerIndex) const noexcept
{
    m_SM->ReleaseRegisterContestation(dispatchPort, replicationIndex, registerIndex);
}

u32 IntFpCore::GetRegister(const u32 dispatchPort, const u32 replicationIndex, const u32 targetRegister) const noexcept
{
    return m_SM->GetRegister(dispatchPort, replicationIndex, targetRegister);
}

void IntFpCore::SetRegister(const u32 dispatchPort, const u32 replicationIndex, const u32 targetRegister, const u32 value) noexcept
{
    m_SM->SetRegister(dispatchPort, replicationIndex, targetRegister, value);
}

void IntFpCore::ReportReady() const noexcept
{
    m_SM->ReportIntFpCoreReady(m_UnitIndex);
}

void IntFpCore::ReleaseRegisterContestation(const u32 dispatchPort, u32 replicationIndex, const u32 registerIndex) const noexcept
{
    m_SM->ReleaseRegisterContestation(dispatchPort, replicationIndex, registerIndex);
}
