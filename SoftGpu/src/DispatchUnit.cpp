#include "DispatchUnit.hpp"
#include "StreamingMultiprocessor.hpp"
#include "LoadStore.hpp"

#include <cstring>

#include <immintrin.h>

void DispatchUnit::ResetCycle() noexcept
{
    m_IsStalled = false;
    m_ClockIndex = 0;
    ++m_TotalIterationsTracker;
    m_FpSaturationTracker += 8 - _mm_popcnt_u32(m_FpAvailabilityMap);
    m_IntFpSaturationTracker += 8 - _mm_popcnt_u32(m_IntFpAvailabilityMap);
    m_LdStSaturationTracker += 4 - _mm_popcnt_u32(m_LdStAvailabilityMap);
    m_TextureSaturationTracker += 2 - _mm_popcnt_u32(m_TextureSamplerAvailabilityMap);
}

void DispatchUnit::Clock() noexcept
{
    ++m_ClockIndex;

    if(!m_InstructionPointer)
    {
        m_IsStalled = true;
        return;
    }

    if(m_IsStalled)
    {
        return;
    }

    if(m_NeedToDecode)
    {
        u64 localInstructionPointer = m_InstructionPointer;

        u32 wordIndex = localInstructionPointer & 0x3;

        u8 instructionBytes[4];
        {
            const u64 wordAddress = localInstructionPointer >> 2;
            const u32 instructionWord = m_SM->Read(wordAddress);
            (void) ::std::memcpy(instructionBytes, &instructionWord, sizeof(instructionWord));
        }

        m_CurrentInstruction = static_cast<EInstruction>(instructionBytes[wordIndex]);

        switch(m_CurrentInstruction)
        {
            case EInstruction::LoadStore: DecodeLdSt(localInstructionPointer, wordIndex, instructionBytes); break;
            case EInstruction::LoadImmediate: DecodeLoadImmediate(localInstructionPointer, wordIndex, instructionBytes); break;
            case EInstruction::LoadZero: DecodeLoadZero(localInstructionPointer, wordIndex, instructionBytes); break;
            case EInstruction::WriteStatistics: DecodeWriteStatistics(localInstructionPointer, wordIndex, instructionBytes); break;
            case EInstruction::AddF:
            case EInstruction::AddVec2F:
            case EInstruction::AddVec3F:
            case EInstruction::AddVec4F:
            case EInstruction::AddH:
            case EInstruction::AddVec2H:
            case EInstruction::AddVec3H:
            case EInstruction::AddVec4H:
            case EInstruction::AddD:
            case EInstruction::AddVec2D:
            case EInstruction::AddVec3D:
            case EInstruction::AddVec4D:
            case EInstruction::SubF:
            case EInstruction::SubVec2F:
            case EInstruction::SubVec3F:
            case EInstruction::SubVec4F:
            case EInstruction::SubH:
            case EInstruction::SubVec2H:
            case EInstruction::SubVec3H:
            case EInstruction::SubVec4H:
            case EInstruction::SubD:
            case EInstruction::SubVec2D:
            case EInstruction::SubVec3D:
            case EInstruction::SubVec4D:
            case EInstruction::MulF:
            case EInstruction::MulVec2F:
            case EInstruction::MulVec3F:
            case EInstruction::MulVec4F:
            case EInstruction::MulH:
            case EInstruction::MulVec2H:
            case EInstruction::MulVec3H:
            case EInstruction::MulVec4H:
            case EInstruction::MulD:
            case EInstruction::MulVec2D:
            case EInstruction::MulVec3D:
            case EInstruction::MulVec4D:
            case EInstruction::DivF:
            case EInstruction::DivVec2F:
            case EInstruction::DivVec3F:
            case EInstruction::DivVec4F:
            case EInstruction::DivH:
            case EInstruction::DivVec2H:
            case EInstruction::DivVec3H:
            case EInstruction::DivVec4H:
            case EInstruction::DivD:
            case EInstruction::DivVec2D:
            case EInstruction::DivVec3D:
            case EInstruction::DivVec4D:
            case EInstruction::RemF:
            case EInstruction::RemVec2F:
            case EInstruction::RemVec3F:
            case EInstruction::RemVec4F:
            case EInstruction::RemH:
            case EInstruction::RemVec2H:
            case EInstruction::RemVec3H:
            case EInstruction::RemVec4H:
            case EInstruction::RemD:
            case EInstruction::RemVec2D:
            case EInstruction::RemVec3D:
            case EInstruction::RemVec4D:
                DecodeFpuBinOp(localInstructionPointer, wordIndex, instructionBytes);
                break;
            default: break;
        }

        m_InstructionPointer = localInstructionPointer + 1;
        m_NeedToDecode = false;
        return;
    }

    u32 replicationIndex = 0;

    // If the mask is not 0 then we're replicating instructions up to 4 times.
    if(m_ReplicationMask != 0x0u)
    {
        // If the replication masks match then we've completed all instruction for the previous cycle.
        if(static_cast<u32>(m_ReplicationCompletedMask) == static_cast<u32>(m_ReplicationMask))
        {
            m_ReplicationCompletedMask = 0;
        }

        // Store mutable versions of the masks so that we can bit shift them.
        u32 replicationMask = m_ReplicationMask;
        u32 replicationCompletedMask = m_ReplicationCompletedMask;

        // Check every bit in the mask;
        while(replicationMask)
        {
            // If the replication is enabled on this bit check to see if it is completed.
            if((replicationMask & 0x1u) != 0x0)
            {
                // If it has not been completed we can execute for this bit.
                if((replicationCompletedMask & 0x1u) == 0x0)
                {
                    break;
                }
            }

            // Shift the masks so that we can check the next bit
            replicationMask >>= 1;
            replicationCompletedMask >>= 1;
            // Keep track of the current bit we're checking
            ++replicationIndex;
        }
    }

    switch(m_CurrentInstruction)
    {
        case EInstruction::Nop: break;
        case EInstruction::Hlt:
        {
            switch(replicationIndex)
            {
                case 0:
                    m_ReplicationMask &= 0b1110;
                    m_ReplicationCompletedMask &= 0b1110;
                    break;
                case 1:
                    m_ReplicationMask &= 0b1101;
                    m_ReplicationCompletedMask &= 0b1101;
                    break;
                case 2:
                    m_ReplicationMask &= 0b1011;
                    m_ReplicationCompletedMask &= 0b1011;
                    break;
                case 3:
                    m_ReplicationMask &= 0b0111;
                    m_ReplicationCompletedMask &= 0b0111;
                    break;
                default: break;
            }

            if(m_ReplicationMask == 0x0u)
            {
                m_IsStalled = true;
            }

            break;
        }
        case EInstruction::LoadStore: DispatchLdSt(replicationIndex); break;
        case EInstruction::LoadImmediate: DispatchLoadImmediate(replicationIndex); break;
        case EInstruction::LoadZero: DispatchLoadZero(replicationIndex); break;
        case EInstruction::FlushCache:
            // for(u32 i = 0; i < 256; ++i)
            // {
            //     if(!CanReadRegister(i, replicationIndex))
            //     {
            //         m_IsStalled = true;
            //         return;
            //     }
            // }
            m_SM->FlushCache();
            break;
        case EInstruction::ResetStatistics:
        {
            m_FpSaturationTracker = 0;
            m_IntFpSaturationTracker = 0;
            m_LdStSaturationTracker = 0;
            m_TextureSaturationTracker = 0;
            m_TotalIterationsTracker = 0;
            break;
        }
        case EInstruction::WriteStatistics: DispatchWriteStatistics(replicationIndex); break;
        case EInstruction::AddF:
        case EInstruction::AddVec2F:
        case EInstruction::AddVec3F:
        case EInstruction::AddVec4F:
        case EInstruction::AddH:
        case EInstruction::AddVec2H:
        case EInstruction::AddVec3H:
        case EInstruction::AddVec4H:
        case EInstruction::AddD:
        case EInstruction::AddVec2D:
        case EInstruction::AddVec3D:
        case EInstruction::AddVec4D:
        case EInstruction::SubF:
        case EInstruction::SubVec2F:
        case EInstruction::SubVec3F:
        case EInstruction::SubVec4F:
        case EInstruction::SubH:
        case EInstruction::SubVec2H:
        case EInstruction::SubVec3H:
        case EInstruction::SubVec4H:
        case EInstruction::SubD:
        case EInstruction::SubVec2D:
        case EInstruction::SubVec3D:
        case EInstruction::SubVec4D:
        case EInstruction::MulF:
        case EInstruction::MulVec2F:
        case EInstruction::MulVec3F:
        case EInstruction::MulVec4F:
        case EInstruction::MulH:
        case EInstruction::MulVec2H:
        case EInstruction::MulVec3H:
        case EInstruction::MulVec4H:
        case EInstruction::MulD:
        case EInstruction::MulVec2D:
        case EInstruction::MulVec3D:
        case EInstruction::MulVec4D:
        case EInstruction::DivF:
        case EInstruction::DivVec2F:
        case EInstruction::DivVec3F:
        case EInstruction::DivVec4F:
        case EInstruction::DivH:
        case EInstruction::DivVec2H:
        case EInstruction::DivVec3H:
        case EInstruction::DivVec4H:
        case EInstruction::DivD:
        case EInstruction::DivVec2D:
        case EInstruction::DivVec3D:
        case EInstruction::DivVec4D:
        case EInstruction::RemF:
        case EInstruction::RemVec2F:
        case EInstruction::RemVec3F:
        case EInstruction::RemVec4F:
        case EInstruction::RemH:
        case EInstruction::RemVec2H:
        case EInstruction::RemVec3H:
        case EInstruction::RemVec4H:
        case EInstruction::RemD:
        case EInstruction::RemVec2D:
        case EInstruction::RemVec3D:
        case EInstruction::RemVec4D:
            DispatchFpuBinOp(replicationIndex);
            break;
        default: break;
    }

    if(!m_IsStalled)
    {
        // Only continue to the next instruction when all replications are complete
        // if(static_cast<u32>(m_ReplicationCompletedMask) >> (replicationIndex + 1) == 0x0u)
        if(static_cast<u32>(m_ReplicationCompletedMask) == static_cast<u32>(m_ReplicationMask))
        {
            m_NeedToDecode = true;
        }
    }
}

void DispatchUnit::NextInstruction(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) const noexcept
{
    ++localInstructionPointer;

    wordIndex = localInstructionPointer & 0x3;

    if(wordIndex == 0)
    {
        const u64 wordAddress = localInstructionPointer >> 2;
        const u32 instructionWord = m_SM->Read(wordAddress);
        (void) ::std::memcpy(instructionBytes, &instructionWord, sizeof(instructionWord));
    }
}

u16 DispatchUnit::ReadU16(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) const noexcept
{
    u8 bytes[sizeof(u16)];

    NextInstruction(localInstructionPointer, wordIndex, instructionBytes);

    bytes[0] = instructionBytes[wordIndex];

    NextInstruction(localInstructionPointer, wordIndex, instructionBytes);

    bytes[1] = instructionBytes[wordIndex];

    u16 ret;
    ::std::memcpy(&ret, bytes, sizeof(ret));

    return ret;
}

u32 DispatchUnit::ReadU32(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) const noexcept
{
    u8 bytes[sizeof(u32)];

    for(uSys i = 0; i < sizeof(u32); ++i)
    {
        NextInstruction(localInstructionPointer, wordIndex, instructionBytes);

        bytes[i] = instructionBytes[wordIndex];
    }

    u32 ret;
    ::std::memcpy(&ret, bytes, sizeof(ret));

    return ret;
}

u64 DispatchUnit::ReadU64(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) const noexcept
{
    u8 bytes[sizeof(u64)];

    for(uSys i = 0; i < sizeof(u64); ++i)
    {
        NextInstruction(localInstructionPointer, wordIndex, instructionBytes);

        bytes[i] = instructionBytes[wordIndex];
    }

    u64 ret;
    ::std::memcpy(&ret, bytes, sizeof(ret));

    return ret;
}

bool DispatchUnit::CanReadRegister(const u32 registerIndex, const u32 replicationIndex) noexcept
{
    return m_SM->CanReadRegister(m_BaseRegisters[replicationIndex] + registerIndex);
}

bool DispatchUnit::CanWriteRegister(const u32 registerIndex, const u32 replicationIndex) noexcept
{
    return m_SM->CanWriteRegister(m_BaseRegisters[replicationIndex] + registerIndex);
}

void DispatchUnit::ReleaseRegisterContestation(const u32 registerIndex, const u32 replicationIndex) noexcept
{
    m_SM->ReleaseRegisterContestation(m_BaseRegisters[replicationIndex] + registerIndex);
}

void DispatchUnit::LockRegisterRead(const u32 registerIndex, const u32 replicationIndex) noexcept
{
    m_SM->LockRegisterRead(m_BaseRegisters[replicationIndex] + registerIndex);
}

void DispatchUnit::LockRegisterWrite(const u32 registerIndex, const u32 replicationIndex) noexcept
{
    m_SM->LockRegisterWrite(m_BaseRegisters[replicationIndex] + registerIndex);
}

void DispatchUnit::DecodeLdSt(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) noexcept
{
    NextInstruction(localInstructionPointer, wordIndex, instructionBytes);

    const u32 readWrite = (instructionBytes[wordIndex] >> 6) & 0x1;
    const u32 indexExponent = (instructionBytes[wordIndex] >> 3) & 0x7;
    const u32 registerCount = instructionBytes[wordIndex] & 0x7;

    NextInstruction(localInstructionPointer, wordIndex, instructionBytes);

    const u8 baseRegister = instructionBytes[wordIndex];
    
    u8 indexRegister = 0;

    if(indexExponent != 7)
    {
        NextInstruction(localInstructionPointer, wordIndex, instructionBytes);

        indexRegister = instructionBytes[wordIndex];
    }

    NextInstruction(localInstructionPointer, wordIndex, instructionBytes);

    const u8 targetRegister = instructionBytes[wordIndex];

    NextInstruction(localInstructionPointer, wordIndex, instructionBytes);

    const u8 offsetLow = instructionBytes[wordIndex];

    NextInstruction(localInstructionPointer, wordIndex, instructionBytes);

    const u8 offsetHigh = instructionBytes[wordIndex];

    const i16 offset = static_cast<i16>((static_cast<u16>(offsetHigh) << 8) | offsetLow);

    m_DecodedInstructionData.LoadStore.Pad = 0;
    m_DecodedInstructionData.LoadStore.ReadWrite = readWrite;
    m_DecodedInstructionData.LoadStore.IndexExponent = indexExponent;
    m_DecodedInstructionData.LoadStore.RegisterCount = registerCount;
    m_DecodedInstructionData.LoadStore.BaseRegister = baseRegister;
    m_DecodedInstructionData.LoadStore.IndexRegister = indexRegister;;
    m_DecodedInstructionData.LoadStore.TargetRegister = targetRegister;
    m_DecodedInstructionData.LoadStore.Offset = offset;
}

void DispatchUnit::DecodeLoadImmediate(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) noexcept
{
    NextInstruction(localInstructionPointer, wordIndex, instructionBytes);

    const u8 targetRegister = instructionBytes[wordIndex];
    
    const u32 value = ReadU32(localInstructionPointer, wordIndex, instructionBytes);

    m_DecodedInstructionData.LoadImmediate.Register = targetRegister;
    m_DecodedInstructionData.LoadImmediate.Value = value;
}

void DispatchUnit::DecodeLoadZero(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) noexcept
{
    NextInstruction(localInstructionPointer, wordIndex, instructionBytes);

    const u8 registerCount = instructionBytes[wordIndex];

    NextInstruction(localInstructionPointer, wordIndex, instructionBytes);

    const u8 startRegister = instructionBytes[wordIndex];

    m_DecodedInstructionData.LoadZero.RegisterCount = registerCount;
    m_DecodedInstructionData.LoadZero.StartRegister = startRegister;
}

void DispatchUnit::DecodeWriteStatistics(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) noexcept
{
    NextInstruction(localInstructionPointer, wordIndex, instructionBytes);

    const u8 statisticIndex = instructionBytes[wordIndex];

    NextInstruction(localInstructionPointer, wordIndex, instructionBytes);

    const u8 startRegister = instructionBytes[wordIndex];

    NextInstruction(localInstructionPointer, wordIndex, instructionBytes);

    const u8 clockStartRegister = instructionBytes[wordIndex];

    m_DecodedInstructionData.WriteStatistics.StatisticIndex = statisticIndex;
    m_DecodedInstructionData.WriteStatistics.StartRegister = startRegister;
    m_DecodedInstructionData.WriteStatistics.ClockStartRegister = clockStartRegister;
}

static u32 GetElementCount(EInstruction instruction) noexcept;
static EPrecision GetElementPrecision(EInstruction instruction) noexcept;
static EBinOp GetElementOperation(EInstruction instruction) noexcept;

void DispatchUnit::DecodeFpuBinOp(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) noexcept
{
    NextInstruction(localInstructionPointer, wordIndex, instructionBytes);

    const u8 registerA = instructionBytes[wordIndex];

    NextInstruction(localInstructionPointer, wordIndex, instructionBytes);

    const u8 registerB = instructionBytes[wordIndex];

    NextInstruction(localInstructionPointer, wordIndex, instructionBytes);

    const u8 storageRegister = instructionBytes[wordIndex];

    const EPrecision precision = GetElementPrecision(m_CurrentInstruction);
    const EBinOp binOp = GetElementOperation(m_CurrentInstruction);
    const u32 baseRegisterCount = GetElementCount(m_CurrentInstruction);

    m_DecodedInstructionData.FpuBinOp.RegisterA = registerA;
    m_DecodedInstructionData.FpuBinOp.RegisterB = registerB;
    m_DecodedInstructionData.FpuBinOp.StorageRegister = storageRegister;
    m_DecodedInstructionData.FpuBinOp.RegisterCount = static_cast<u8>(baseRegisterCount);
    m_DecodedInstructionData.FpuBinOp.Precision = precision;
    m_DecodedInstructionData.FpuBinOp.BinOp = binOp;

    m_VectorOpIndex = 0;
}

void DispatchUnit::DispatchLdSt(const u32 replicationIndex) noexcept
{
    if(m_LdStAvailabilityMap == 0u)
    {
        m_IsStalled = true;
        return;
    }

    u32 ldStUnit;
    if((m_LdStAvailabilityMap & 0x1u) != 0)
    {
        ldStUnit = 0;
    }
    else if((m_LdStAvailabilityMap & 0x2u) != 0)
    {
        ldStUnit = 1;
    }
    else if((m_LdStAvailabilityMap & 0x4u) != 0)
    {
        ldStUnit = 2;
    }
    else if((m_LdStAvailabilityMap & 0x8u) != 0)
    {
        ldStUnit = 3;
    }
    else
    {
        m_IsStalled = true;
        return;
    }
    
    if(!CanReadRegister(m_DecodedInstructionData.LoadStore.BaseRegister, replicationIndex) || !CanReadRegister(m_DecodedInstructionData.LoadStore.BaseRegister + 1u, replicationIndex))
    {
        m_IsStalled = true;
        return;
    }
    
    if(m_DecodedInstructionData.LoadStore.IndexExponent != 7u && !CanReadRegister(m_DecodedInstructionData.LoadStore.IndexRegister, replicationIndex))
    {
        m_IsStalled = true;
        return;
    }
    
    for(u32 i = 0; i < m_DecodedInstructionData.LoadStore.RegisterCount + 1u; ++i)
    {
        if(m_DecodedInstructionData.LoadStore.ReadWrite == 1u && !CanReadRegister(m_DecodedInstructionData.LoadStore.TargetRegister + i, replicationIndex))
        {
            m_IsStalled = true;
            return;
        }
        else if(m_DecodedInstructionData.LoadStore.ReadWrite == 0u && !CanWriteRegister(m_DecodedInstructionData.LoadStore.TargetRegister + i, replicationIndex))
        {
            m_IsStalled = true;
            return;
        }
    }
    
    LockRegisterRead(m_DecodedInstructionData.LoadStore.BaseRegister, replicationIndex);
    LockRegisterRead(m_DecodedInstructionData.LoadStore.BaseRegister + 1u, replicationIndex);

    if(m_DecodedInstructionData.LoadStore.IndexExponent != 7u)
    {
        LockRegisterRead(m_DecodedInstructionData.LoadStore.IndexRegister, replicationIndex);
    }

    for(u32 i = 0; i < m_DecodedInstructionData.LoadStore.RegisterCount + 1u; ++i)
    {
        if(m_DecodedInstructionData.LoadStore.ReadWrite == 1u)
        {
            LockRegisterRead(m_DecodedInstructionData.LoadStore.TargetRegister + i, replicationIndex);
        }
        else
        {
            LockRegisterWrite(m_DecodedInstructionData.LoadStore.TargetRegister + i, replicationIndex);
        }
    }

    LoadStoreInstruction instruction;
    instruction.DispatchUnit = m_Index;
    instruction.ReadWrite = m_DecodedInstructionData.LoadStore.ReadWrite;
    instruction.IndexExponent = m_DecodedInstructionData.LoadStore.IndexExponent;
    instruction.RegisterCount = m_DecodedInstructionData.LoadStore.RegisterCount;
    instruction.BaseRegister = m_BaseRegisters[replicationIndex] + m_DecodedInstructionData.LoadStore.BaseRegister;
    instruction.IndexRegister = m_BaseRegisters[replicationIndex] + m_DecodedInstructionData.LoadStore.IndexRegister;
    instruction.TargetRegister = m_BaseRegisters[replicationIndex] + m_DecodedInstructionData.LoadStore.TargetRegister;
    instruction.Offset = m_DecodedInstructionData.LoadStore.Offset;

    m_SM->DispatchLdSt(ldStUnit, instruction);

    m_ReplicationCompletedMask |= 1 << replicationIndex;
}

void DispatchUnit::DispatchLoadImmediate(const u32 replicationIndex) noexcept
{
    if(!CanWriteRegister(m_DecodedInstructionData.LoadImmediate.Register, replicationIndex))
    {
        m_IsStalled = true;
        return;
    }
    
    m_SM->SetRegister(m_BaseRegisters[replicationIndex] + m_DecodedInstructionData.LoadImmediate.Register, m_DecodedInstructionData.LoadImmediate.Value);
    m_ReplicationCompletedMask |= 1 << replicationIndex;
}

void DispatchUnit::DispatchLoadZero(const u32 replicationIndex) noexcept
{
    for(u32 i = 0; i < m_DecodedInstructionData.LoadZero.RegisterCount + 1u; ++i)
    {
        if(!CanWriteRegister(m_DecodedInstructionData.LoadZero.StartRegister + i, replicationIndex))
        {
            m_IsStalled = true;
            return;
        }
    }

    for(u32 i = 0; i < m_DecodedInstructionData.LoadZero.RegisterCount + 1u; ++i)
    {
        m_SM->SetRegister(m_BaseRegisters[replicationIndex] + static_cast<u32>(m_DecodedInstructionData.LoadZero.StartRegister) + i, 0);
    }

    m_ReplicationCompletedMask |= 1 << replicationIndex;
}

void DispatchUnit::DispatchWriteStatistics(const u32 replicationIndex) noexcept
{
    if(!CanWriteRegister(m_DecodedInstructionData.WriteStatistics.StartRegister, replicationIndex))
    {
        m_IsStalled = true;
        return;
    }

    if(!CanWriteRegister(m_DecodedInstructionData.WriteStatistics.StartRegister + 1, replicationIndex))
    {
        m_IsStalled = true;
        return;
    }

    if(!CanWriteRegister(m_DecodedInstructionData.WriteStatistics.ClockStartRegister, replicationIndex))
    {
        m_IsStalled = true;
        return;
    }

    if(!CanWriteRegister(m_DecodedInstructionData.WriteStatistics.ClockStartRegister + 1, replicationIndex))
    {
        m_IsStalled = true;
        return;
    }

    u32 clockWords[2];
    (void) ::std::memcpy(clockWords, &m_TotalIterationsTracker, sizeof(m_TotalIterationsTracker));

    m_SM->SetRegister(m_BaseRegisters[replicationIndex] + m_DecodedInstructionData.WriteStatistics.ClockStartRegister, clockWords[0]);
    m_SM->SetRegister(m_BaseRegisters[replicationIndex] + m_DecodedInstructionData.WriteStatistics.ClockStartRegister + 1, clockWords[1]);

    u64 targetStatistic = 0;
    if(m_DecodedInstructionData.WriteStatistics.StatisticIndex == 0)
    {
        targetStatistic = m_FpSaturationTracker;
    }
    else if(m_DecodedInstructionData.WriteStatistics.StatisticIndex == 1)
    {
        targetStatistic = m_IntFpSaturationTracker;
    }
    else if(m_DecodedInstructionData.WriteStatistics.StatisticIndex == 2)
    {
        targetStatistic = m_LdStSaturationTracker;
    }
    else if(m_DecodedInstructionData.WriteStatistics.StatisticIndex == 3)
    {
        targetStatistic = m_TextureSaturationTracker;
    }

    u32 statisticWords[2];
    (void) ::std::memcpy(statisticWords, &targetStatistic, sizeof(targetStatistic));

    m_SM->SetRegister(m_BaseRegisters[replicationIndex] + m_DecodedInstructionData.WriteStatistics.StartRegister, statisticWords[0]);
    m_SM->SetRegister(m_BaseRegisters[replicationIndex] + m_DecodedInstructionData.WriteStatistics.StartRegister + 1, statisticWords[1]);

    m_ReplicationCompletedMask |= 1 << replicationIndex;
}

void DispatchUnit::DispatchFpuBinOp(const u32 replicationIndex) noexcept
{
    // Attempt to perform up to 4 ops per cycle for vectors.
    // This will exit after either there are no more available cores or we complete the required number of vector ops.
    // This can probably be a while(true) loop, but for safety and verbosity we're explicitly defining as for(0..3) loop.
    for(u32 i = 0; i < 4; ++i)
    {
        if(m_FpAvailabilityMap == 0u && m_IntFpAvailabilityMap == 0u)
        {
            m_IsStalled = true;
            return;
        }

        u32 registerOffset = static_cast<u32>(m_VectorOpIndex);

        if(m_DecodedInstructionData.FpuBinOp.Precision == EPrecision::Double)
        {
            registerOffset *= 2;

            if(!CanReadRegister(m_DecodedInstructionData.FpuBinOp.RegisterA + registerOffset, replicationIndex))
            {
                m_IsStalled = true;
                return;
            }

            if(!CanReadRegister(m_DecodedInstructionData.FpuBinOp.RegisterA + registerOffset + 1, replicationIndex))
            {
                m_IsStalled = true;
                return;
            }

            if(!CanReadRegister(m_DecodedInstructionData.FpuBinOp.RegisterB + registerOffset, replicationIndex))
            {
                m_IsStalled = true;
                return;
            }

            if(!CanReadRegister(m_DecodedInstructionData.FpuBinOp.RegisterB + registerOffset + 1, replicationIndex))
            {
                m_IsStalled = true;
                return;
            }

            if(!CanWriteRegister(m_DecodedInstructionData.FpuBinOp.StorageRegister + registerOffset, replicationIndex))
            {
                m_IsStalled = true;
                return;
            }

            if(!CanWriteRegister(m_DecodedInstructionData.FpuBinOp.StorageRegister + registerOffset + 1, replicationIndex))
            {
                m_IsStalled = true;
                return;
            }

            LockRegisterRead(m_DecodedInstructionData.FpuBinOp.RegisterA + registerOffset, replicationIndex);
            LockRegisterRead(m_DecodedInstructionData.FpuBinOp.RegisterA + registerOffset + 1, replicationIndex);

            LockRegisterRead(m_DecodedInstructionData.FpuBinOp.RegisterB + registerOffset, replicationIndex);
            LockRegisterRead(m_DecodedInstructionData.FpuBinOp.RegisterB + registerOffset + 1, replicationIndex);

            LockRegisterWrite(m_DecodedInstructionData.FpuBinOp.StorageRegister + registerOffset, replicationIndex);
            LockRegisterWrite(m_DecodedInstructionData.FpuBinOp.StorageRegister + registerOffset + 1, replicationIndex);
        }
        else
        {
            if(!CanReadRegister(m_DecodedInstructionData.FpuBinOp.RegisterA + registerOffset, replicationIndex))
            {
                m_IsStalled = true;
                return;
            }

            if(!CanReadRegister(m_DecodedInstructionData.FpuBinOp.RegisterB + registerOffset, replicationIndex))
            {
                m_IsStalled = true;
                return;
            }

            if(!CanWriteRegister(m_DecodedInstructionData.FpuBinOp.StorageRegister + registerOffset, replicationIndex))
            {
                m_IsStalled = true;
                return;
            }

            LockRegisterRead(m_DecodedInstructionData.FpuBinOp.RegisterA + registerOffset, replicationIndex);
            LockRegisterRead(m_DecodedInstructionData.FpuBinOp.RegisterB + registerOffset, replicationIndex);
            
            LockRegisterWrite(m_DecodedInstructionData.FpuBinOp.StorageRegister + registerOffset, replicationIndex);
        }

        u32 fpUnit = 0;
        {
            u32 fpAvailabilityMap = m_FpAvailabilityMap;
            while(fpUnit < 8)
            {
                if((fpAvailabilityMap & 0x1) != 0)
                {
                    break;
                }

                fpAvailabilityMap >>= 1;
                ++fpUnit;
            }

            if(fpUnit == 8)
            {
                fpAvailabilityMap = m_IntFpAvailabilityMap;
                while(fpUnit < 16)
                {
                    if((fpAvailabilityMap & 0x1) != 0)
                    {
                        break;
                    }

                    fpAvailabilityMap >>= 1;
                    ++fpUnit;
                }
            }

            if(fpUnit == 16)
            {
                m_IsStalled = true;
                return;
            }
        }

        // TODO: Handle replication for vectors.
        FpuInstruction fpuInstruction;
        fpuInstruction.DispatchPort = m_Index;
        fpuInstruction.Operation = EFpuOp::BasicBinOp;
        fpuInstruction.Precision = m_DecodedInstructionData.FpuBinOp.Precision;
        fpuInstruction.Reserved0 = 0;
        fpuInstruction.OperandA = m_BaseRegisters[replicationIndex] + m_DecodedInstructionData.FpuBinOp.RegisterA + registerOffset;
        fpuInstruction.OperandB = m_BaseRegisters[replicationIndex] + m_DecodedInstructionData.FpuBinOp.RegisterB + registerOffset;
        fpuInstruction.OperandC = static_cast<u32>(m_DecodedInstructionData.FpuBinOp.BinOp);
        fpuInstruction.StorageRegister = m_BaseRegisters[replicationIndex] + m_DecodedInstructionData.FpuBinOp.StorageRegister + registerOffset;
        fpuInstruction.Reserved1 = 0;

        m_SM->DispatchFpu(fpUnit, fpuInstruction);

        // Have we completed all operations for this vector.
        if(static_cast<u32>(m_VectorOpIndex) + 1 == m_DecodedInstructionData.FpuBinOp.RegisterCount)
        {
            m_ReplicationCompletedMask |= 1 << replicationIndex;
            m_VectorOpIndex = 0;
            return;
        }
        else
        {
            ++m_VectorOpIndex;
        }
    }
}

static u32 GetElementCount(const EInstruction instruction) noexcept
{
    switch(instruction)
    {
        case EInstruction::AddF:
        case EInstruction::AddH:
        case EInstruction::AddD:
        case EInstruction::SubF:
        case EInstruction::SubH:
        case EInstruction::SubD:
        case EInstruction::MulF:
        case EInstruction::MulH:
        case EInstruction::MulD:
        case EInstruction::DivF:
        case EInstruction::DivH:
        case EInstruction::DivD:
        case EInstruction::RemF:
        case EInstruction::RemH:
        case EInstruction::RemD:
            return 1;
        case EInstruction::AddVec2F:
        case EInstruction::AddVec2H:
        case EInstruction::AddVec2D:
        case EInstruction::SubVec2F:
        case EInstruction::SubVec2H:
        case EInstruction::SubVec2D:
        case EInstruction::MulVec2F:
        case EInstruction::MulVec2H:
        case EInstruction::MulVec2D:
        case EInstruction::DivVec2F:
        case EInstruction::DivVec2H:
        case EInstruction::DivVec2D:
        case EInstruction::RemVec2F:
        case EInstruction::RemVec2H:
        case EInstruction::RemVec2D:
            return 2;
        case EInstruction::AddVec3F:
        case EInstruction::AddVec3H:
        case EInstruction::AddVec3D:
        case EInstruction::SubVec3F:
        case EInstruction::SubVec3H:
        case EInstruction::SubVec3D:
        case EInstruction::MulVec3F:
        case EInstruction::MulVec3H:
        case EInstruction::MulVec3D:
        case EInstruction::DivVec3F:
        case EInstruction::DivVec3H:
        case EInstruction::DivVec3D:
        case EInstruction::RemVec3F:
        case EInstruction::RemVec3H:
        case EInstruction::RemVec3D:
            return 3;
        case EInstruction::AddVec4F:
        case EInstruction::AddVec4H:
        case EInstruction::AddVec4D:
        case EInstruction::SubVec4F:
        case EInstruction::SubVec4H:
        case EInstruction::SubVec4D:
        case EInstruction::MulVec4F:
        case EInstruction::MulVec4H:
        case EInstruction::MulVec4D:
        case EInstruction::DivVec4F:
        case EInstruction::DivVec4H:
        case EInstruction::DivVec4D:
        case EInstruction::RemVec4F:
        case EInstruction::RemVec4H:
        case EInstruction::RemVec4D:
            return 4;
        default: return 0;
    }
}

static EPrecision GetElementPrecision(const EInstruction instruction) noexcept
{
    switch(instruction)
    {
        case EInstruction::AddF:
        case EInstruction::AddVec2F:
        case EInstruction::AddVec3F:
        case EInstruction::AddVec4F:
        case EInstruction::SubF:
        case EInstruction::SubVec2F:
        case EInstruction::SubVec3F:
        case EInstruction::SubVec4F:
        case EInstruction::MulF:
        case EInstruction::MulVec2F:
        case EInstruction::MulVec3F:
        case EInstruction::MulVec4F:
        case EInstruction::DivF:
        case EInstruction::DivVec2F:
        case EInstruction::DivVec3F:
        case EInstruction::DivVec4F:
        case EInstruction::RemF:
        case EInstruction::RemVec2F:
        case EInstruction::RemVec3F:
        case EInstruction::RemVec4F:
            return EPrecision::Single;
        case EInstruction::AddH:
        case EInstruction::AddVec2H:
        case EInstruction::AddVec3H:
        case EInstruction::AddVec4H:
        case EInstruction::SubH:
        case EInstruction::SubVec2H:
        case EInstruction::SubVec3H:
        case EInstruction::SubVec4H:
        case EInstruction::MulH:
        case EInstruction::MulVec2H:
        case EInstruction::MulVec3H:
        case EInstruction::MulVec4H:
        case EInstruction::DivH:
        case EInstruction::DivVec2H:
        case EInstruction::DivVec3H:
        case EInstruction::DivVec4H:
        case EInstruction::RemH:
        case EInstruction::RemVec2H:
        case EInstruction::RemVec3H:
        case EInstruction::RemVec4H:
            return EPrecision::Half;
        case EInstruction::AddD:
        case EInstruction::AddVec2D:
        case EInstruction::AddVec3D:
        case EInstruction::AddVec4D:
        case EInstruction::SubD:
        case EInstruction::SubVec2D:
        case EInstruction::SubVec3D:
        case EInstruction::SubVec4D:
        case EInstruction::MulD:
        case EInstruction::MulVec2D:
        case EInstruction::MulVec3D:
        case EInstruction::MulVec4D:
        case EInstruction::DivD:
        case EInstruction::DivVec2D:
        case EInstruction::DivVec3D:
        case EInstruction::DivVec4D:
        case EInstruction::RemD:
        case EInstruction::RemVec2D:
        case EInstruction::RemVec3D:
        case EInstruction::RemVec4D:
            return EPrecision::Double;
        default: return EPrecision::Single;
    }
}

static EBinOp GetElementOperation(const EInstruction instruction) noexcept
{
    switch(instruction)
    {
        case EInstruction::AddF:
        case EInstruction::AddVec2F:
        case EInstruction::AddVec3F:
        case EInstruction::AddVec4F:
        case EInstruction::AddH:
        case EInstruction::AddVec2H:
        case EInstruction::AddVec3H:
        case EInstruction::AddVec4H:
        case EInstruction::AddD:
        case EInstruction::AddVec2D:
        case EInstruction::AddVec3D:
        case EInstruction::AddVec4D:
            return EBinOp::Add;
        case EInstruction::SubF:
        case EInstruction::SubVec2F:
        case EInstruction::SubVec3F:
        case EInstruction::SubVec4F:
        case EInstruction::SubH:
        case EInstruction::SubVec2H:
        case EInstruction::SubVec3H:
        case EInstruction::SubVec4H:
        case EInstruction::SubD:
        case EInstruction::SubVec2D:
        case EInstruction::SubVec3D:
        case EInstruction::SubVec4D:
            return EBinOp::Subtract;
        case EInstruction::MulF:
        case EInstruction::MulVec2F:
        case EInstruction::MulVec3F:
        case EInstruction::MulVec4F:
        case EInstruction::MulH:
        case EInstruction::MulVec2H:
        case EInstruction::MulVec3H:
        case EInstruction::MulVec4H:
        case EInstruction::MulD:
        case EInstruction::MulVec2D:
        case EInstruction::MulVec3D:
        case EInstruction::MulVec4D:
            return EBinOp::Multiply;
        case EInstruction::DivF:
        case EInstruction::DivVec2F:
        case EInstruction::DivVec3F:
        case EInstruction::DivVec4F:
        case EInstruction::DivH:
        case EInstruction::DivVec2H:
        case EInstruction::DivVec3H:
        case EInstruction::DivVec4H:
        case EInstruction::DivD:
        case EInstruction::DivVec2D:
        case EInstruction::DivVec3D:
        case EInstruction::DivVec4D:
            return EBinOp::Divide;
        case EInstruction::RemF:
        case EInstruction::RemVec2F:
        case EInstruction::RemVec3F:
        case EInstruction::RemVec4F:
        case EInstruction::RemH:
        case EInstruction::RemVec2H:
        case EInstruction::RemVec3H:
        case EInstruction::RemVec4H:
        case EInstruction::RemD:
        case EInstruction::RemVec2D:
        case EInstruction::RemVec3D:
        case EInstruction::RemVec4D:
            return EBinOp::Remainder;
        default: return EBinOp::Add;
    }
}
