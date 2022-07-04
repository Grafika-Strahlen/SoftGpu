#pragma once

#include "RegisterFile.hpp"
#include "LoadStore.hpp"
#include "DispatchUnit.hpp"
#include "Core.hpp"

class Processor;

class StreamingMultiprocessor final
{
    DEFAULT_DESTRUCT(StreamingMultiprocessor);
    DELETE_CM(StreamingMultiprocessor);
public:
    StreamingMultiprocessor(Processor* const processor, const u32 smIndex) noexcept
        : m_Processor(processor)
        , m_RegisterFile{ }
        , m_LdSt { { this, 0 }, { this, 1 }, { this, 2 }, { this, 3 } }
        , m_FpCores { { this, 0 }, { this, 1 }, { this, 2 }, { this, 3 }, { this, 4 }, { this, 5 }, { this, 6 }, { this, 7 } }
        , m_IntFpCores { { this, 0 }, { this, 1 }, { this, 2 }, { this, 3 }, { this, 4 }, { this, 5 }, { this, 6 }, { this, 7 } }
        , m_DispatchUnits { { this, 0 }, { this, 1 } }
        , m_SMIndex(smIndex)
    { }

    void Clock() noexcept
    {
        m_DispatchUnits[0].Clock();
        m_DispatchUnits[1].Clock();

        m_DispatchUnits[0].Clock();
        m_DispatchUnits[1].Clock();

        m_DispatchUnits[0].Clock();
        m_DispatchUnits[1].Clock();
    }

    void TestLoadProgram(const u32 dispatchPort, const u64 program)
    {
        const u32 baseRegisters[4] = { dispatchPort * 256, 0, 0, 0 };
        m_DispatchUnits[dispatchPort].LoadIP(0x1, baseRegisters, program);
    }

    [[nodiscard]] u32 Read(u64 address) noexcept;
    void Write(u64 address, u32 value) noexcept;
    void Prefetch(u64 address) noexcept;

    [[nodiscard]] u32 GetRegister(const u32 dispatchPort, const u32 replicationIndex, const u32 targetRegister) const noexcept
    {
        return m_RegisterFile.GetRegister(m_DispatchUnits[dispatchPort].BaseRegister(replicationIndex), targetRegister);
    }

    void SetRegister(const u32 dispatchPort, const u32 replicationIndex, const u32 targetRegister, const u32 value) noexcept
    {
        m_RegisterFile.SetRegister(m_DispatchUnits[dispatchPort].BaseRegister(replicationIndex), targetRegister, value);
    }

    void ReportCoreReady(const u32 unitIndex) noexcept
    {
        m_DispatchUnits[0].ReportUnitReady(unitIndex);
        m_DispatchUnits[1].ReportUnitReady(unitIndex);
    }

    void ReleaseRegisterContestation(const u32 dispatchPort, const u32 replicationIndex, const u32 registerIndex)
    {
        m_DispatchUnits[dispatchPort].ReleaseRegisterContestation(registerIndex, replicationIndex);
    }

    void DispatchLdSt(const u32 ldStIndex, const LoadStoreInstruction instructionInfo) noexcept
    {
        m_DispatchUnits[0].ReportUnitBusy(ldStIndex + LDST_AVAIL_OFFSET);
        m_DispatchUnits[1].ReportUnitBusy(ldStIndex + LDST_AVAIL_OFFSET);

        m_LdSt[ldStIndex].Execute(instructionInfo);
    }

    void DispatchFpu(const u32 fpIndex, const FpuInstruction instructionInfo) noexcept
    {
        if(fpIndex < 8)
        {
            m_DispatchUnits[0].ReportUnitBusy(fpIndex + FP_AVAIL_OFFSET);
            m_DispatchUnits[1].ReportUnitBusy(fpIndex + FP_AVAIL_OFFSET);
            m_FpCores[fpIndex].Execute(instructionInfo);
        }
        else
        {
            m_DispatchUnits[0].ReportUnitBusy(fpIndex + INT_FP_AVAIL_OFFSET);
            m_DispatchUnits[1].ReportUnitBusy(fpIndex + INT_FP_AVAIL_OFFSET);
            m_IntFpCores[fpIndex].ExecuteFP(instructionInfo);
        }
    }

    void FlushCache() noexcept;
private:
    Processor* m_Processor;
    RegisterFile m_RegisterFile;
    LoadStore m_LdSt[4];
    FpCore m_FpCores[8];
    IntFpCore m_IntFpCores[8];
    DispatchUnit m_DispatchUnits[2];
    u32 m_SMIndex;
};
