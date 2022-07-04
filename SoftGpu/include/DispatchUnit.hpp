#pragma once

#include <Objects.hpp>
#include <NumTypes.hpp>

#include <cstring>

#include "FPU.hpp"

class StreamingMultiprocessor;

enum class EInstruction : u8
{
    Nop = 0,
    Hlt,
    LoadStore, // { 0 : 1, Read/Write : 1, IndexExponent : 3, RegisterCount : 3 }, BaseRegister : 8, [ IndexRegister : 8 ], TargetRegister : 8, Offset : 16
    LoadImmediate, // Register : 8, Value : 32
    LoadZero, // RegisterCount : 8, StartRegister : 8
    FlushCache,
    AddF,     // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    AddVec2F, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    AddVec3F, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    AddVec4F, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    AddH,     // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    AddVec2H, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    AddVec3H, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    AddVec4H, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    AddD,     // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    AddVec2D, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    AddVec3D, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    AddVec4D, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
};

namespace InstructionDecodeData {

struct LoadStoreData final
{
    u32 Pad : 1;
    u32 ReadWrite : 1;
    u32 IndexExponent : 3;
    u32 RegisterCount : 3;
    u32 BaseRegister : 8;
    u32 IndexRegister : 8;
    u32 TargetRegister : 8;
    i16 Offset;
};

struct LoadImmediateData final
{
    u8 Register;
    u32 Value;
};

struct LoadZeroData final
{
    u8 RegisterCount;
    u8 StartRegister;
};

struct FpuBinOpData final
{
    u8 RegisterA;
    u8 RegisterB;
    u8 StorageRegister;
    u8 RegisterCount;
    EPrecision Precision;
    EBinOp BinOp;
};

union InstructionData
{
    LoadStoreData LoadStore;
    LoadImmediateData LoadImmediate;
    LoadZeroData LoadZero;
    FpuBinOpData FpuBinOp;
};

}

class DispatchUnit final
{
    DEFAULT_DESTRUCT(DispatchUnit);
    DELETE_CM(DispatchUnit);
public:
    DispatchUnit(StreamingMultiprocessor* const m_sm, const u32 m_index) noexcept
        : m_SM(m_sm)
        , m_Index(m_index)
        , m_BaseRegisters{ 0, 0, 0, 0 }
        , m_ClockIndex(0)
        , m_InstructionPointer(0)
        , m_FpAvailabilityMap(0xFF)
        , m_IntFpAvailabilityMap(0xFF)
        , m_SfuAvailabilityMap(0xF)
        , m_LdStAvailabilityMap(0xF)
        , m_TextureSamplerAvailabilityMap(0x3)
        , m_IsStalled(0)
        , m_NeedToDecode(true)
        , m_ReplicationMask(0x0)
        , m_ReplicationCompletedMask(0x0)
        , m_VectorOpIndex(0)
        , m_Pad1{ }
        , m_CurrentInstruction(EInstruction::Nop)
        , m_DecodedInstructionData{ }
        , m_RegisterContestationMap()
    { }

    [[nodiscard]] u32 BaseRegister(const u32 index) const noexcept { return m_BaseRegisters[index]; }

    void ResetCycle() noexcept;

    void Clock() noexcept;

    void ReportUnitReady(u32 unitIndex) noexcept
    {
        if(unitIndex < 8)
        {
            m_FpAvailabilityMap |= 1 << unitIndex;
            return;
        }
        else
        {
            unitIndex -= 8;
        }


        if(unitIndex < 8)
        {
            m_IntFpAvailabilityMap |= 1 << unitIndex;
            return;
        }
        else
        {
            unitIndex -= 8;
        }

        if(unitIndex < 4)
        {
            m_SfuAvailabilityMap |= 1 << unitIndex;
            return;
        }
        else
        {
            unitIndex -= 4;
        }

        if(unitIndex < 4)
        {
            m_LdStAvailabilityMap |= 1 << unitIndex;
            return;
        }
        else
        {
            unitIndex -= 4;
        }

        if(unitIndex < 2)
        {
            m_TextureSamplerAvailabilityMap |= 1 << unitIndex;
            return;
        }
        else
        {
            unitIndex -= 2;
        }
    }

    void ReportUnitBusy(u32 unitIndex) noexcept
    {
        if(unitIndex < 8)
        {
            m_FpAvailabilityMap &= ~(1 << unitIndex);
            return;
        }
        else
        {
            unitIndex -= 8;
        }


        if(unitIndex < 8)
        {
            m_IntFpAvailabilityMap &= ~(1 << unitIndex);
            return;
        }
        else
        {
            unitIndex -= 8;
        }

        if(unitIndex < 4)
        {
            m_SfuAvailabilityMap &= ~(1 << unitIndex);
            return;
        }
        else
        {
            unitIndex -= 4;
        }

        if(unitIndex < 4)
        {
            m_LdStAvailabilityMap &= ~(1 << unitIndex);
            return;
        }
        else
        {
            unitIndex -= 4;
        }

        if(unitIndex < 2)
        {
            m_TextureSamplerAvailabilityMap &= ~(1 << unitIndex);
            return;
        }
        else
        {
            unitIndex -= 2;
        }
    }

    void ReleaseRegisterContestation(const u32 registerIndex, const u32 replicationIndex) noexcept
    {
        --m_RegisterContestationMap[replicationIndex][registerIndex];
        // If that was the last read lock the value will now be 1, which is actually the write lock. Set it to no lock.
        if(m_RegisterContestationMap[replicationIndex][registerIndex] == 1)
        {
            m_RegisterContestationMap[replicationIndex][registerIndex] = 0;
        }
    }

    void LoadIP(const u32 replicationMask, const u32 baseRegisters[4], const u64 instructionPointer) noexcept
    {
        m_ReplicationMask = replicationMask;
        m_ReplicationCompletedMask = 0x0;
        ::std::memcpy(m_BaseRegisters, baseRegisters, sizeof(u32[4]));
        m_InstructionPointer = instructionPointer;
    }
private:
    void NextInstruction(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) const noexcept;
    u16 ReadU16(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) const noexcept;
    u32 ReadU32(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) const noexcept;
    u64 ReadU64(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) const noexcept;
    void LockRegisterRead(u32 registerIndex, const u32 replicationIndex) noexcept;

    void DecodeLdSt(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) noexcept;
    void DecodeLoadImmediate(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) noexcept;
    void DecodeLoadZero(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) noexcept;
    void DecodeFpuBinOp(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) noexcept;

    void DispatchLdSt(u32 replicationIndex) noexcept;
    void DispatchLoadImmediate(u32 replicationIndex) noexcept;
    void DispatchLoadZero(u32 replicationIndex) noexcept;
    void DispatchFpuBinOp(u32 replicationIndex) noexcept;
private:
    StreamingMultiprocessor* m_SM;
    u32 m_Index;
    u32 m_BaseRegisters[4];
    u32 m_ClockIndex;
    u64 m_InstructionPointer;
    u32 m_FpAvailabilityMap : 8;
    u32 m_IntFpAvailabilityMap : 8;
    u32 m_SfuAvailabilityMap : 4;
    u32 m_LdStAvailabilityMap : 4;
    u32 m_TextureSamplerAvailabilityMap : 2;
    u32 m_IsStalled : 1;
    u32 m_NeedToDecode : 1;
    u32 m_ReplicationMask : 4;
    u32 m_ReplicationCompletedMask : 4;
    // The current element of a vector we're operating on.
    u32 m_VectorOpIndex : 2;
    u32 m_Pad1 : 22;
    // The currently decoded instruction.
    EInstruction m_CurrentInstruction;
    InstructionDecodeData::InstructionData m_DecodedInstructionData;
    // This contains information about how each register is being used.
    // If the value is zero the register is unused.
    // If the value is one it is locked for writes.
    // Otherwise the register is locked for reads. Any number of simultaneous reads are allowed.
    u8 m_RegisterContestationMap[4][256];
};


#define FP_AVAIL_OFFSET (0)
#define INT_FP_AVAIL_OFFSET (FP_AVAIL_OFFSET + 8)
#define SFU_AVAIL_OFFSET (INT_FP_AVAIL_OFFSET + 8)
#define LDST_AVAIL_OFFSET (SFU_AVAIL_OFFSET + 4)
#define TEX_AVAIL_OFFSET (LDST_AVAIL_OFFSET + 4)
