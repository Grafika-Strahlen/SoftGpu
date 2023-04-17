#pragma once

#include "RegisterFile.hpp"
#include "LoadStore.hpp"
#include "DispatchUnit.hpp"
#include "Core.hpp"
#include "DebugManager.hpp"
#include "RegisterAllocator.hpp"
#include "MMU.hpp"

class Processor;

class StreamingMultiprocessor final
{
    DEFAULT_DESTRUCT(StreamingMultiprocessor);
    DELETE_CM(StreamingMultiprocessor);
public:
    StreamingMultiprocessor(Processor* const processor, const u32 smIndex) noexcept
        : m_Processor(processor)
        , m_RegisterFile { }
        , m_Mmu(this)
        , m_LdSt { { this, 0 }, { this, 1 }, { this, 2 }, { this, 3 } }
        , m_FpCores { { this, 0 }, { this, 1 }, { this, 2 }, { this, 3 }, { this, 4 }, { this, 5 }, { this, 6 }, { this, 7 } }
        , m_IntFpCores { { this, 0 }, { this, 1 }, { this, 2 }, { this, 3 }, { this, 4 }, { this, 5 }, { this, 6 }, { this, 7 } }
        , m_DispatchUnits { { this, 0 }, { this, 1 } }
        , m_SMIndex(smIndex)
    { }

    void Reset()
    {
        m_RegisterFile.Reset();
        m_Mmu.Reset();
        m_LdSt[0].Reset();
        m_LdSt[1].Reset();
        m_LdSt[2].Reset();
        m_LdSt[3].Reset();

        for(u32 subClockIndex = 0; subClockIndex <= 5; ++subClockIndex)
        {
            for(u32 coreIndex = 0; coreIndex < 8; ++coreIndex)
            {
                m_FpCores[coreIndex].Reset();
                m_IntFpCores[coreIndex].Reset();
            }
        }

        m_DispatchUnits[0].Reset();
        m_DispatchUnits[1].Reset();
    }

    void Clock() noexcept
    {
        if(GlobalDebug.IsAttached())
        {
            m_RegisterFile.ReportRegisters(m_SMIndex);
            m_DispatchUnits[0].ReportBaseRegisters(m_SMIndex);
            m_DispatchUnits[1].ReportBaseRegisters(m_SMIndex);
        }

        for(uSys i = 0; i < LoadStore::MAX_EXECUTION_STAGE; ++i)
        {
            m_LdSt[0].Clock();
            m_LdSt[1].Clock();
            m_LdSt[2].Clock();
            m_LdSt[3].Clock();
            m_RegisterFile.Clock();
        }

        for(u32 subClockIndex = 0; subClockIndex <= 5; ++subClockIndex)
        {
            for(u32 coreIndex = 0; coreIndex < 4; ++coreIndex)
            {
                m_FpCores[coreIndex].Clock(subClockIndex);
                m_RegisterFile.Clock();
            }
            for(u32 coreIndex = 0; coreIndex < 4; ++coreIndex)
            {
                m_IntFpCores[coreIndex].Clock(subClockIndex);
                m_RegisterFile.Clock();
            }
            for(u32 coreIndex = 4; coreIndex < 8; ++coreIndex)
            {
                m_FpCores[coreIndex].Clock(subClockIndex);
                m_RegisterFile.Clock();
            }
            for(u32 coreIndex = 4; coreIndex < 8; ++coreIndex)
            {
                m_IntFpCores[coreIndex].Clock(subClockIndex);
                m_RegisterFile.Clock();
            }
        }

        m_DispatchUnits[0].ResetCycle();
        m_DispatchUnits[1].ResetCycle();

        for(u32 i = 0; i < 6; ++i)
        {
            m_DispatchUnits[0].Clock();
            m_DispatchUnits[1].Clock();
        }
    }

    void TestLoadProgram(const u32 dispatchPort, const u8 replicationMask, const u64 program)
    {
        const u16 baseRegisters[4] = { static_cast<u16>((dispatchPort * 4 + 0) * 256), static_cast<u16>((dispatchPort * 4 + 1) * 256), static_cast<u16>((dispatchPort * 4 + 2) * 256), static_cast<u16>((dispatchPort * 4 + 3) * 256) };
        m_DispatchUnits[dispatchPort].LoadIP(replicationMask, baseRegisters, program);
    }

    void TestLoadRegister(const u32 dispatchPort, const u32 replicationIndex, const u8 registerIndex, const u32 registerValue)
    {
        u32 value = registerValue;
        bool successful = false;
        bool unsuccessful = false;

        RegisterFile::CommandPacket packet;
        packet.Command = RegisterFile::ECommand::WriteRegister;
        packet.TargetRegister = replicationIndex & 0x7FF;
        packet.Value = &value;
        packet.Successful = &successful;
        packet.Unsuccessful = &unsuccessful;

        if(replicationIndex & 0x01)
        {
            InvokeRegisterFileHigh(0, packet);
        }
        else
        {
            InvokeRegisterFileLow(0, packet);
        }

        m_RegisterFile.Clock();

        packet.Command = RegisterFile::ECommand::Reset;

        if(replicationIndex & 0x01)
        {
            InvokeRegisterFileHigh(0, packet);
        }
        else
        {
            InvokeRegisterFileLow(0, packet);
        }

        m_RegisterFile.Clock();

        // m_RegisterFile.SetRegister((dispatchPort * 4 + replicationIndex) * 256 + registerIndex, registerValue);
    }

    void LoadWarp(const u32 dispatchPort, const u8 enabledMask, const u8 completedMask, const u16 baseRegisters[8], const u64 instructionPointer) noexcept
    {
        m_DispatchUnits[dispatchPort].LoadWarp(enabledMask, completedMask, baseRegisters, instructionPointer);
    }

    [[nodiscard]] u32 Read(u64 address) noexcept;
    void Write(u64 address, u32 value) noexcept;
    void Prefetch(u64 address) noexcept;

    void InvokeRegisterFileHigh(const u32 port, const RegisterFile::CommandPacket packet) noexcept
    {
        switch(port)
        {
            case 0: m_RegisterFile.InvokePort0High(packet); break;
            case 1: m_RegisterFile.InvokePort1High(packet); break;
            case 2: m_RegisterFile.InvokePort2High(packet); break;
            case 3: m_RegisterFile.InvokePort3High(packet); break;
            default:
                ConPrinter::PrintLn("Invalid high port target for register file: {}", port);
                assert(false);
                break;
        }
    }

    void InvokeRegisterFileLow(const u32 port, const RegisterFile::CommandPacket packet) noexcept
    {
        switch(port)
        {
            case 0: m_RegisterFile.InvokePort0Low(packet); break;
            case 1: m_RegisterFile.InvokePort1Low(packet); break;
            case 2: m_RegisterFile.InvokePort2Low(packet); break;
            case 3: m_RegisterFile.InvokePort3Low(packet); break;
            default:
                ConPrinter::PrintLn("Invalid low port target for register file: {}", port);
                assert(false);
                break;
        }
    }

    void ReportFpCoreReady(const u32 unitIndex) noexcept
    {
        m_DispatchUnits[0].ReportUnitReady(unitIndex + FP_AVAIL_OFFSET);
        m_DispatchUnits[1].ReportUnitReady(unitIndex + FP_AVAIL_OFFSET);
    }

    void ReportIntFpCoreReady(const u32 unitIndex) noexcept
    {
        m_DispatchUnits[0].ReportUnitReady(unitIndex + INT_FP_AVAIL_OFFSET);
        m_DispatchUnits[1].ReportUnitReady(unitIndex + INT_FP_AVAIL_OFFSET);
    }

    void ReportLdStReady(const u32 unitIndex) noexcept
    {
        m_DispatchUnits[0].ReportUnitReady(unitIndex + LDST_AVAIL_OFFSET);
        m_DispatchUnits[1].ReportUnitReady(unitIndex + LDST_AVAIL_OFFSET);
    }
    
    void DispatchLdSt(const u32 ldStIndex, const LoadStoreInstruction instructionInfo) noexcept
    {
        m_DispatchUnits[0].ReportUnitBusy(ldStIndex + LDST_AVAIL_OFFSET);
        m_DispatchUnits[1].ReportUnitBusy(ldStIndex + LDST_AVAIL_OFFSET);

        m_LdSt[ldStIndex].PrepareExecution(instructionInfo);
    }

    void DispatchFpu(const u32 fpIndex, const FpuInstruction instructionInfo) noexcept
    {
        if(fpIndex < 8)
        {
            m_DispatchUnits[0].ReportUnitBusy(fpIndex + FP_AVAIL_OFFSET);
            m_DispatchUnits[1].ReportUnitBusy(fpIndex + FP_AVAIL_OFFSET);
            m_FpCores[fpIndex].InitiateInstruction(instructionInfo);
        }
        else
        {
            m_DispatchUnits[0].ReportUnitBusy(fpIndex - 8 + INT_FP_AVAIL_OFFSET);
            m_DispatchUnits[1].ReportUnitBusy(fpIndex - 8 + INT_FP_AVAIL_OFFSET);
            m_IntFpCores[fpIndex - 8].InitiateInstructionFP(instructionInfo);
        }
    }

    void LoadPageDirectoryPointer(const u64 pageDirectoryPhysicalAddress) noexcept
    {
        m_Mmu.LoadPageDirectoryPointer(pageDirectoryPhysicalAddress);
    }

    void FlushMmuCache() noexcept
    {
        m_Mmu.FlushCache();
    }

    void WriteMmuPageInfo(u64 physicalAddress, u64 pageTableEntry) noexcept;

    void FlushCache() noexcept;

    u16 AllocateRegisters(const u16 registerCount) noexcept
    {
        return m_RegisterAllocator.AllocateRegisterBlock(registerCount);
    }

    void FreeRegisters(const u16 registerBase, const u16 registerCount) noexcept
    {
        m_RegisterAllocator.FreeRegisterBlock(registerBase, registerCount);
    }
private:
    Processor* m_Processor;
    RegisterFile m_RegisterFile;
    RegisterAllocator m_RegisterAllocator;
    Mmu m_Mmu;
    LoadStore m_LdSt[4];
    FpCore m_FpCores[8];
    IntFpCore m_IntFpCores[8];
    DispatchUnit m_DispatchUnits[2];
    u32 m_SMIndex;
};
