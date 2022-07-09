#include "CoreRegisterManager.hpp"
#include "Core.hpp"

#include <cstring>

void CoreRegisterManager::Clock(const u32 clockIndex) noexcept
{
    switch(clockIndex)
    {
        case 0:
            RegisterRead();
            break;
        case 1:
            ReadLockRelease();
            break;
        case 2:
            // Op Execute
            break;
        case 3:
            RegisterWrite();
            break;
        case 4:
            WriteLockRelease();
            break;
        case 5:
            m_WriteLock64Bit = m_Write64Bit;
            m_RegisterWriteLock = m_RegisterWrite;
            m_RegisterWriteLockReleaseReady = m_RegisterWriteReady;

            m_RegisterWriteReady = false;

            m_ReadLock64Bit = m_Read64Bit;
            m_RegisterReadLockEnabledCount = m_RegisterReadEnabledCount;
            m_RegisterReadLockA = m_RegisterReadA;
            m_RegisterReadLockB = m_RegisterReadB;
            m_RegisterReadLockC = m_RegisterReadC;
            m_RegisterReadLockReleaseReady = m_RegisterReadReady;

            m_RegisterReadReady = false;
            
            m_Core->ReportReady();
            break;
        default:
            break;
    }
}

void CoreRegisterManager::InitiateRegisterRead(const bool is64Bit, const u8 registerCount, const u32 registerA, const u32 registerB, const u32 registerC) noexcept
{
    m_Read64Bit = is64Bit;
    m_RegisterReadEnabledCount = registerCount;
    m_RegisterReadA = registerA;
    m_RegisterReadB = registerB;
    m_RegisterReadC = registerC;

    m_RegisterReadReady = true;
}

void CoreRegisterManager::InitiateRegisterWrite(const bool is64Bit, const u32 storageRegister, const u64 value) noexcept
{
    m_Write64Bit = is64Bit;
    m_RegisterWrite = storageRegister;
    m_RegisterWriteValue = value;

    m_RegisterWriteReady = true;
}

void CoreRegisterManager::RegisterRead() noexcept
{
    if(!m_RegisterReadReady)
    {
        return;
    }

    if(m_Read64Bit)
    {
        const u32 aLow = m_Core->GetRegister(m_RegisterReadA);
        const u32 aHigh = m_Core->GetRegister(m_RegisterReadA + 1);

        const u64 a = (static_cast<u64>(aHigh) << 32) | aLow;
        u64 b = 0;
        u64 c = 0;

        if(m_RegisterReadEnabledCount >= 1u)
        {
            const u32 bLow = m_Core->GetRegister(m_RegisterReadB);
            const u32 bHigh = m_Core->GetRegister(m_RegisterReadB + 1);

            b = (static_cast<u64>(bHigh) << 32) | bLow;

            if(m_RegisterReadEnabledCount >= 2u)
            {
                const u32 cLow = m_Core->GetRegister(m_RegisterReadC);
                const u32 cHigh = m_Core->GetRegister(m_RegisterReadC + 1);

                c = (static_cast<u64>(cHigh) << 32) | cLow;
            }
        }

        m_Core->ReportRegisterValues(a, b, c);
    }
    else
    {
        const u64 a = m_Core->GetRegister(m_RegisterReadA);
        u64 b = 0;
        u64 c = 0;

        if(m_RegisterReadEnabledCount >= 1u)
        {
            b = m_Core->GetRegister(m_RegisterReadB);

            if(m_RegisterReadEnabledCount >= 2u)
            {
                c = m_Core->GetRegister(m_RegisterReadC);
            }
        }

        m_Core->ReportRegisterValues(a, b, c);
    }
}

void CoreRegisterManager::ReadLockRelease() noexcept
{
    if(!m_RegisterReadLockReleaseReady)
    {
        return;
    }

    if(m_ReadLock64Bit)
    {
        m_Core->ReleaseRegisterContestation(m_RegisterReadA);
        m_Core->ReleaseRegisterContestation(m_RegisterReadA + 1);

        if(m_RegisterReadLockEnabledCount >= 1u)
        {
            m_Core->ReleaseRegisterContestation(m_RegisterReadB);
            m_Core->ReleaseRegisterContestation(m_RegisterReadB + 1);

            if(m_RegisterReadLockEnabledCount >= 2u)
            {
                m_Core->ReleaseRegisterContestation(m_RegisterReadC);
                m_Core->ReleaseRegisterContestation(m_RegisterReadC + 1);
            }
        }
    }
    else
    {
        m_Core->ReleaseRegisterContestation(m_RegisterReadA);

        if(m_RegisterReadLockEnabledCount >= 1u)
        {
            m_Core->ReleaseRegisterContestation(m_RegisterReadB);

            if(m_RegisterReadLockEnabledCount >= 2u)
            {
                m_Core->ReleaseRegisterContestation(m_RegisterReadC);
            }
        }
    }
}

void CoreRegisterManager::RegisterWrite() noexcept
{
    if(!m_RegisterWriteReady)
    {
        return;
    }

    if(m_Write64Bit)
    {
        u32 words[2];
        (void) ::std::memcpy(words, &m_RegisterWriteValue, sizeof(m_RegisterWriteValue));

        m_Core->SetRegister(m_RegisterWrite, words[0]);
        m_Core->SetRegister(m_RegisterWrite + 1, words[1]);
    }
    else
    {
        m_Core->SetRegister(m_RegisterWrite, static_cast<u32>(m_RegisterWriteValue));
    }
}

void CoreRegisterManager::WriteLockRelease() noexcept
{
    if(!m_RegisterWriteLockReleaseReady)
    {
        return;
    }

    m_Core->ReleaseRegisterContestation(m_RegisterWrite);

    if(m_WriteLock64Bit)
    {
        m_Core->ReleaseRegisterContestation(m_RegisterWrite + 1);
    }
}
