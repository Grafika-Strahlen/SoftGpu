#pragma once

#include <Objects.hpp>
#include <NumTypes.hpp>

#include <cstring>

#include "FPU.hpp"
#include "DebugManager.hpp"

class StreamingMultiprocessor;

enum class EInstruction : u8
{
    Nop = 0,
    Hlt,
    LoadStore, // { 0 : 1, Read/Write : 1, IndexExponent : 3, RegisterCount : 3 }, BaseRegister : 8, [ IndexRegister : 8 ], TargetRegister : 8, Offset : 16
    LoadImmediate, // Register : 8, Value : 32
    LoadZero, // RegisterCount : 8, StartRegister : 8
    SwapRegister, // RegisterA : 8, RegisterB : 8
    CopyRegister, // SourceRegister : 8, DestinationRegister : 8
    FlushCache,
    ResetStatistics,
    WriteStatistics, // TargetStatisticIndex : 8, TargetRegister: 8, CounterRegister : 8
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
    SubF,     // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    SubVec2F, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    SubVec3F, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    SubVec4F, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    SubH,     // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    SubVec2H, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    SubVec3H, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    SubVec4H, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    SubD,     // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    SubVec2D, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    SubVec3D, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    SubVec4D, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    MulF,     // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    MulVec2F, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    MulVec3F, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    MulVec4F, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    MulH,     // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    MulVec2H, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    MulVec3H, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    MulVec4H, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    MulD,     // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    MulVec2D, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    MulVec3D, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    MulVec4D, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    DivF,     // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    DivVec2F, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    DivVec3F, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    DivVec4F, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    DivH,     // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    DivVec2H, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    DivVec3H, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    DivVec4H, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    DivD,     // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    DivVec2D, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    DivVec3D, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    DivVec4D, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    RemF,     // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    RemVec2F, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    RemVec3F, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    RemVec4F, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    RemH,     // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    RemVec2H, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    RemVec3H, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    RemVec4H, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    RemD,     // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    RemVec2D, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    RemVec3D, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
    RemVec4D, // RegisterA : 8, RegisterB : 8, StorageRegister : 8
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

struct WriteStatisticsData final
{
    u8 StatisticIndex;
    u8 StartRegister;
    u8 ClockStartRegister;
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
    WriteStatisticsData WriteStatistics;
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
        , m_BaseRegisters{ 0, 0, 0, 0, 0, 0, 0, 0 }
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
        , m_FpSaturationTracker(0)
        , m_IntFpSaturationTracker(0)
        , m_SfuSaturationTracker(0)
        , m_LdStSaturationTracker(0)
        , m_TextureSaturationTracker(0)
        , m_TotalIterationsTracker(0)
    { }

    void Reset()
    {
        m_BaseRegisters[0] = 0;
        m_BaseRegisters[1] = 0;
        m_BaseRegisters[2] = 0;
        m_BaseRegisters[3] = 0;
        m_BaseRegisters[4] = 0;
        m_BaseRegisters[5] = 0;
        m_BaseRegisters[6] = 0;
        m_BaseRegisters[7] = 0;

        m_ClockIndex = 0;
        m_InstructionPointer = 0;
        m_FpAvailabilityMap = 0xFF;
        m_IntFpAvailabilityMap = 0xFF;
        m_SfuAvailabilityMap = 0xF;
        m_LdStAvailabilityMap = 0xF;
        m_TextureSamplerAvailabilityMap = 0x3;
        m_IsStalled = 0;
        m_NeedToDecode = true;
        m_ReplicationMask = 0x0;
        m_ReplicationCompletedMask = 0x0;
        m_VectorOpIndex = 0;
        m_Pad1 = { };
        m_CurrentInstruction = EInstruction::Nop;
        m_DecodedInstructionData = { };
        m_FpSaturationTracker = 0;
        m_IntFpSaturationTracker = 0;
        m_SfuSaturationTracker = 0;
        m_LdStSaturationTracker = 0;
        m_TextureSaturationTracker = 0;
        m_TotalIterationsTracker = 0;
    }
    
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

    void LoadIP(const u32 replicationMask, const u16 baseRegisters[4], const u64 instructionPointer) noexcept
    {
        m_ReplicationMask = replicationMask;
        m_ReplicationCompletedMask = 0x0;
        ::std::memcpy(m_BaseRegisters, baseRegisters, sizeof(u16[4]));
        m_InstructionPointer = instructionPointer;
    }

    void LoadWarp(const u32 enabledMask, const u32 completedMask, const u16 baseRegisters[8], const u64 instructionPointer) noexcept
    {
        m_ReplicationMask = enabledMask;
        m_ReplicationCompletedMask = completedMask;
        ::std::memcpy(m_BaseRegisters, baseRegisters, sizeof(m_BaseRegisters));
        m_InstructionPointer = instructionPointer;
    }

    void ReportBaseRegisters(const u32 smIndex) noexcept
    {
        if(GlobalDebug.IsAttached())
        {
            GlobalDebug.WriteRawInfo(DebugCodeReportBaseRegister);
            GlobalDebug.WriteRawInfo(6);
            GlobalDebug.WriteRawInfo(smIndex);
            GlobalDebug.WriteRawInfo(m_Index);
            GlobalDebug.WriteRawInfo<u32>(m_BaseRegisters[0]);
            GlobalDebug.WriteRawInfo<u32>(m_BaseRegisters[1]);
            GlobalDebug.WriteRawInfo<u32>(m_BaseRegisters[2]);
            GlobalDebug.WriteRawInfo<u32>(m_BaseRegisters[3]);
        }
    }
private:
    void NextInstruction(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) const noexcept;
    u16 ReadU16(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) const noexcept;
    u32 ReadU32(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) const noexcept;
    u64 ReadU64(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) const noexcept;

    [[nodiscard]] bool CanReadRegister(u32 registerIndex, u32 replicationIndex) noexcept;
    [[nodiscard]] bool CanWriteRegister(u32 registerIndex, u32 replicationIndex) noexcept;
    void ReleaseRegisterContestation(u32 registerIndex, u32 replicationIndex) noexcept;
    void LockRegisterRead(u32 registerIndex, u32 replicationIndex) noexcept;
    void LockRegisterWrite(u32 registerIndex, u32 replicationIndex) noexcept;


    void DecodeLdSt(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) noexcept;
    void DecodeLoadImmediate(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) noexcept;
    void DecodeLoadZero(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) noexcept;
    void DecodeWriteStatistics(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) noexcept;
    void DecodeFpuBinOp(u64& localInstructionPointer, u32& wordIndex, u8 instructionBytes[4]) noexcept;

    void DispatchLdSt(u32 replicationIndex) noexcept;
    void DispatchLoadImmediate(u32 replicationIndex) noexcept;
    void DispatchLoadZero(u32 replicationIndex) noexcept;
    void DispatchWriteStatistics(u32 replicationIndex) noexcept;
    void DispatchFpuBinOp(u32 replicationIndex) noexcept;
private:
    StreamingMultiprocessor* m_SM;
    u32 m_Index;
    u16 m_BaseRegisters[8];
    u32 m_ClockIndex;
    u64 m_InstructionPointer;
    u32 m_FpAvailabilityMap : 8;
    u32 m_IntFpAvailabilityMap : 8;
    u32 m_SfuAvailabilityMap : 4;
    u32 m_LdStAvailabilityMap : 4;
    u32 m_TextureSamplerAvailabilityMap : 2;
    u32 m_IsStalled : 1;
    u32 m_NeedToDecode : 1;
    u32 m_ReplicationMask : 8;
    u32 m_ReplicationCompletedMask : 8;
    // The current element of a vector we're operating on.
    u32 m_VectorOpIndex : 2;
    u32 m_Pad1 : 14;
    // The currently decoded instruction.
    EInstruction m_CurrentInstruction;
    InstructionDecodeData::InstructionData m_DecodedInstructionData;

    u64 m_FpSaturationTracker;
    u64 m_IntFpSaturationTracker;
    u64 m_SfuSaturationTracker;
    u64 m_LdStSaturationTracker;
    u64 m_TextureSaturationTracker;
    u64 m_TotalIterationsTracker;
};

#define FP_AVAIL_OFFSET (0)
#define INT_FP_AVAIL_OFFSET (FP_AVAIL_OFFSET + 8)
#define SFU_AVAIL_OFFSET (INT_FP_AVAIL_OFFSET + 8)
#define LDST_AVAIL_OFFSET (SFU_AVAIL_OFFSET + 4)
#define TEX_AVAIL_OFFSET (LDST_AVAIL_OFFSET + 4)
