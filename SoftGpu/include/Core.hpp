/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <Objects.hpp>
#include <NumTypes.hpp>
#include "FPU.hpp"
#include "CoreRegisterManager.hpp"
#include "RegisterFile.hpp"

#include <cstring>

class StreamingMultiprocessor;

// This polymorphism would map to the FPU and FPU/ALU simply receiving data lines for which they can perform register calls.
class ICore
{
    DEFAULT_CONSTRUCT_PO(ICore);
    DEFAULT_DESTRUCT_VI(ICore);
    DELETE_CM(ICore);
public:
    virtual void InvokeRegisterFileHigh(RegisterFile::CommandPacket packet) noexcept = 0;
    virtual void InvokeRegisterFileLow(RegisterFile::CommandPacket packet) noexcept = 0;

    virtual void ReportRegisterValues(u64 a, u64 b, u64 c) noexcept = 0;
    virtual void PrepareRegisterWrite(bool is64Bit, u32 storageRegister, u64 value) noexcept = 0;

    virtual void ReportReady() const noexcept = 0;
};

class FpCore final : public ICore
{
    DEFAULT_DESTRUCT(FpCore);
    DELETE_CM(FpCore);
public:
    FpCore(StreamingMultiprocessor* const sm, const u32 unitIndex) noexcept
        : m_SM(sm)
        , m_UnitIndex(unitIndex)
        , m_Fpu(this)
        , m_CRM(this)
        , m_PipelineSlot0{ }
        , m_PipelineSlot1{ }
        , m_PipelineSlot2{ }
        , m_Stage0Ready(false)
        , m_Stage1Ready(false)
        , m_Stage2Ready(false)
        , m_Pad{ }
    { }

    void Reset()
    {
        m_Fpu.Reset();
        m_CRM.Reset();
        m_PipelineSlot0 = { };
        m_PipelineSlot1 = { };
        m_PipelineSlot2 = { };
        m_Stage0Ready = false;
        m_Stage1Ready = false;
        m_Stage2Ready = false;
    }

    void Clock(const u32 clockIndex) noexcept
    {
        m_CRM.Clock(clockIndex);

        if(clockIndex == 2 && m_Stage2Ready)
        {
            m_Fpu.ExecuteInstruction(m_PipelineSlot2);
        }
        else if(clockIndex == 5)
        {
            (void) ::std::memcpy(&m_PipelineSlot2, &m_PipelineSlot1, sizeof(LoadedFpuInstruction));
            (void) ::std::memcpy(&m_PipelineSlot1, &m_PipelineSlot0, sizeof(LoadedFpuInstruction));

            m_Stage2Ready = m_Stage1Ready;
            m_Stage1Ready = m_Stage0Ready;
            m_Stage0Ready = false;
        }
    }
    
    void InvokeRegisterFileHigh(RegisterFile::CommandPacket packet) noexcept override;
    void InvokeRegisterFileLow(RegisterFile::CommandPacket packet) noexcept override;

    void InitiateInstruction(const FpuInstruction fpuInstruction) noexcept
    {
        m_CRM.InitiateRegisterRead(fpuInstruction.Precision == EPrecision::Double, RequiredRegisterCount(fpuInstruction.Operation), fpuInstruction.OperandA, fpuInstruction.OperandB, fpuInstruction.OperandC);

        m_PipelineSlot0.DispatchPort = fpuInstruction.DispatchPort;
        m_PipelineSlot0.Operation = fpuInstruction.Operation;
        m_PipelineSlot0.Precision = fpuInstruction.Precision;
        m_PipelineSlot0.StorageRegister = fpuInstruction.StorageRegister;
        m_PipelineSlot0.OperandA = fpuInstruction.OperandA;
        m_PipelineSlot0.OperandB = fpuInstruction.OperandB;
        m_PipelineSlot0.OperandC = fpuInstruction.OperandC;

        m_Stage0Ready = true;
    }

    void ReportRegisterValues(const u64 a, const u64 b, const u64 c) noexcept override
    {
        switch(RequiredRegisterCount(m_PipelineSlot2.Operation))
        {
            case 2:
                m_PipelineSlot0.OperandC = c;
            case 1:
                m_PipelineSlot0.OperandB = b;
            case 0:
                m_PipelineSlot0.OperandA = a;
            default: break;
        }
    }

    void PrepareRegisterWrite(const bool is64Bit, const u32 storageRegister, const u64 value) noexcept override
    {
        m_CRM.InitiateRegisterWrite(is64Bit, storageRegister, value);
    }

    void ReportReady() const noexcept override;
private:
    StreamingMultiprocessor* m_SM;
    u32 m_UnitIndex;
    Fpu m_Fpu;
    CoreRegisterManager m_CRM;

    LoadedFpuInstruction m_PipelineSlot0;
    LoadedFpuInstruction m_PipelineSlot1;
    LoadedFpuInstruction m_PipelineSlot2;

    u8 m_Stage0Ready : 1;
    u8 m_Stage1Ready : 1;
    u8 m_Stage2Ready : 1;
    u8 m_Pad : 5;
};

class IntFpCore final : public ICore
{
    DEFAULT_DESTRUCT(IntFpCore);
    DELETE_CM(IntFpCore);
public:
    IntFpCore(StreamingMultiprocessor* const sm, const u32 unitIndex) noexcept
        : m_SM(sm)
        , m_UnitIndex(unitIndex)
        , m_Fpu(this)
        , m_CRM(this)
        , m_PipelineSlot0{ }
        , m_PipelineSlot1{ }
        , m_PipelineSlot2{ }
        , m_Stage0Ready(false)
        , m_Stage1Ready(false)
        , m_Stage2Ready(false)
        , m_Pad{ }
    { }

    void Reset()
    {
        m_Fpu.Reset();
        m_CRM.Reset();
        m_PipelineSlot0 = { };
        m_PipelineSlot1 = { };
        m_PipelineSlot2 = { };
        m_Stage0Ready = false;
        m_Stage1Ready = false;
        m_Stage2Ready = false;
    }

    void Clock(const u32 clockIndex) noexcept
    {
        m_CRM.Clock(clockIndex);

        if(clockIndex == 2 && m_Stage2Ready)
        {
            m_Fpu.ExecuteInstruction(m_PipelineSlot2);
        }
        else if(clockIndex == 5)
        {
            (void) ::std::memcpy(&m_PipelineSlot2, &m_PipelineSlot1, sizeof(LoadedFpuInstruction));
            (void) ::std::memcpy(&m_PipelineSlot1, &m_PipelineSlot0, sizeof(LoadedFpuInstruction));

            m_Stage2Ready = m_Stage1Ready;
            m_Stage1Ready = m_Stage0Ready;
            m_Stage0Ready = false;
        }
    }
    
    void InvokeRegisterFileHigh(RegisterFile::CommandPacket packet) noexcept override;
    void InvokeRegisterFileLow(RegisterFile::CommandPacket packet) noexcept override;
    
    void InitiateInstructionFP(const FpuInstruction fpuInstruction) noexcept
    {
        m_CRM.InitiateRegisterRead(fpuInstruction.Precision == EPrecision::Double, RequiredRegisterCount(fpuInstruction.Operation), fpuInstruction.OperandA, fpuInstruction.OperandB, fpuInstruction.OperandC);

        m_PipelineSlot0.DispatchPort = fpuInstruction.DispatchPort;
        m_PipelineSlot0.Operation = fpuInstruction.Operation;
        m_PipelineSlot0.Precision = fpuInstruction.Precision;
        m_PipelineSlot0.StorageRegister = fpuInstruction.StorageRegister;
        m_PipelineSlot0.OperandA = fpuInstruction.OperandA;
        m_PipelineSlot0.OperandB = fpuInstruction.OperandB;
        m_PipelineSlot0.OperandC = fpuInstruction.OperandC;

        m_Stage0Ready = true;
    }

    void ReportRegisterValues(const u64 a, const u64 b, const u64 c) noexcept override
    {
        switch(RequiredRegisterCount(m_PipelineSlot2.Operation))
        {
            case 2:
                m_PipelineSlot0.OperandC = c;
            case 1:
                m_PipelineSlot0.OperandB = b;
            case 0:
                m_PipelineSlot0.OperandA = a;
            default: break;
        }
    }

    void PrepareRegisterWrite(const bool is64Bit, const u32 storageRegister, const u64 value) noexcept override
    {
        m_CRM.InitiateRegisterWrite(is64Bit, storageRegister, value);
    }

    void ReportReady() const noexcept override;
private:
    StreamingMultiprocessor* m_SM;
    u32 m_UnitIndex;
    Fpu m_Fpu;
    CoreRegisterManager m_CRM;

    LoadedFpuInstruction m_PipelineSlot0;
    LoadedFpuInstruction m_PipelineSlot1;
    LoadedFpuInstruction m_PipelineSlot2;

    u8 m_Stage0Ready : 1;
    u8 m_Stage1Ready : 1;
    u8 m_Stage2Ready : 1;
    u8 m_Pad : 5;
};
