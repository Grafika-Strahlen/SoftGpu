/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <Common.hpp>
#include "ControlBus.hpp"
#include "InstructionFetch.hpp"
#include "RISCVCommon.hpp"

namespace riscv {

class ControllerReceiverSample
{
public:
    void ReceiveController_ControlBus([[maybe_unused]] const u32 index, [[maybe_unused]] const ControlBus& controlBus) noexcept { }
    void ReceiveController_CSRReadData([[maybe_unused]] const u32 index, [[maybe_unused]] const WordType data) noexcept { }
};

// template<typename Receiver>
class Controller final
{
    DEFAULT_DESTRUCT(Controller);
    DELETE_CM(Controller);
public:
    using Receiver = ControllerReceiverSample;

    constexpr static inline bool EnableISA_U      = false; // User Mode
    constexpr static inline bool EnableISA_Zaamo  = false; // Atomic read-modify-write operations
    constexpr static inline bool EnableISA_Zalrsc = false; // Atomic reservation-set operations
    constexpr static inline bool EnableISA_Zfinx  = false; // 32-Bit floating point
    constexpr static inline bool EnableISA_Zicntr = false; // Base counters
    constexpr static inline bool EnableISA_Zihpm  = false; // Hardware performance monitors
    constexpr static inline bool EnableISA_Zxcfu  = false; // Custom functions unit
    constexpr static inline bool EnableISA_Sdext  = true; // External debug mode
    constexpr static inline bool EnableISA_Sdtrig = true; // Trigger module
    constexpr static inline bool EnableISA_Smpmp  = false; // Physical memory protection

    enum class ExecutionEngineState : u32
    {
        Restart = 0,
        Dispatch,
        TrapEnter,
        TrapExit,
        Sleep,
        Execute,
        ALUWait,
        Branch,
        Branched,
        System,
        MemoryRequest,
        MemoryResponse
    };

    static inline constexpr WordType BootAddress = 0;

    struct ExecutionEngine final
    {
        DEFAULT_CONSTRUCT_PU(ExecutionEngine);
        DEFAULT_DESTRUCT(ExecutionEngine);
        DEFAULT_CM_PU(ExecutionEngine);

        ExecutionEngineState State : 4 = ExecutionEngineState::Restart;
        u32 Pad0 : 28;
        u32 InstructionWord;
        WordType PC;
        WordType PCNext;
        WordType ReturnAddress;
    };

    #pragma pack(push, 1)
    struct DebugControlAndStatusRegister final
    {
        DEFAULT_CONSTRUCT_PUC(DebugControlAndStatusRegister);
        DEFAULT_DESTRUCT_C(DebugControlAndStatusRegister);
        DEFAULT_CM_PUC(DebugControlAndStatusRegister);

        WordType PrivilegeLevel : 2;
        WordType Step : 1;
        WordType NoPendingNMI : 1 = 0;
        WordType MachinePrivilegeEnable : 1 = 1;
        WordType Reserved0 : 1 = 0;
        WordType DebugCause : 3;
        WordType StopTime : 1 = 0;
        WordType StopCount : 1 = 1;
        WordType StepInterruptEnable : 1 = 0;
        WordType EBreakUserMode : 1;
        WordType EBreakSupervisorMode : 1 = 0;
        WordType Reserved1 : 1 = 0;
        WordType EBreakMachineMode : 1;
        WordType Reserved2 : 12 = 0;
        WordType XDebugVer : 4 = 0b0100;
    };

    struct TriggerMatchControlAndStatusRegister final
    {
        DEFAULT_CONSTRUCT_PUC(TriggerMatchControlAndStatusRegister);
        DEFAULT_DESTRUCT_C(TriggerMatchControlAndStatusRegister);
        DEFAULT_CM_PUC(TriggerMatchControlAndStatusRegister);

        WordType Load : 1 = 0;
        WordType Store : 1 = 0;
        WordType Execute : 1;
        WordType UserMode : 1 = BOOL_TO_BIT(EnableISA_U);
        WordType SupervisorMode : 1 = 0;
        WordType UncertainEnable : 1 = 0;
        WordType MachineMode : 1 = 1;
        WordType Match : 4 = 0b0000;
        WordType Chain : 1 = 0;
        WordType Action : 4 = 0x1; // Enter Debug Mode on Trigger
        WordType Size : 3 = 0b000;
        WordType Reserved : 2 = 0;
        WordType Select : 1 = 0;
        WordType Hit0 : 1;
        WordType VirtualUserMode : 1 = 0;
        WordType VirtualSupervisorMode : 1 = 0;
        WordType Hit1 : 1 = 0;
        WordType Uncertain : 1 = 0;
        WordType DMode : 1 = 1;
        WordType Type : 4 = 0x6;
    };
    #pragma pack(pop)

    static_assert(sizeof(DebugControlAndStatusRegister) == sizeof(WordType), "DebugControlAndStatusRegister did not match word size.");
    static_assert(sizeof(TriggerMatchControlAndStatusRegister) == sizeof(WordType), "TriggerMatchControlAndStatusRegister did not match word size.");
private:
    SENSITIVITY_DECL(p_Reset_n, p_Clock);

    SIGNAL_ENTITIES();
public:
    Controller(
        Receiver* const parent,
        const u32 index
    ) noexcept
        : m_Parent(parent)
        , m_Index(index)
        , p_Reset_n(0)
        , p_Clock(0)
        , p_PMPFault(0)
        , p_ALUCoProcessorDone(0)
        , p_ALUCompareStatus(0)
        , p_IRQDebug(0)
        , p_IRQMachine(0)
        , p_IRQFast(0)
        , p_LSUWait(0)
        , p_LSUError(0)
        , p_InstructionFetchBus{ }
        , p_ALUAddressResult(0)
        , p_RS1(0)
        , p_XCSRReadData(0)
        , p_LSUMemoryAddressRegister(0)
        , m_Immediate(0)
        , m_CSRReadData(0)
        , m_CSRMachineExceptionPC(0)
        , m_CSRMachineTrapHandlerAddress(0)
        , m_CSRMachineBadAddress(0)
        , m_CSRMachineTrapInstruction(0)
        , m_CSRMachineScratchRegister(0)
        , m_CSRDebugPC(0)
        , m_CSRDebugScratchRegister0(0)
        , m_CSRTriggerAddressMatchRegister(0)
        , m_ExecutionEngine()
        , m_ExecutionEngineNext()
        , m_CSRAddress(0)
        , m_CSRWriteEnable(0)
        , m_CSRWriteEnableNext(0)
        , m_CSRReadEnable(0)
        , m_CSRReadEnableNext(0)
        , m_CSRMachineStatusIRQEnable(0)
        , m_CSRMachineStatusIRQEnablePrev(0)
        , m_CSRMachineStatusPrivilegeModePrev(0)
        , m_CSRMachineStatusEffectivePrivilegeMode(0)
        , m_CSRMachineStatusTW(0)
        , m_CSRMachineSoftwareInterruptEnable(0)
        , m_CSRMachineExternalInterruptEnable(0)
        , m_CSRMachineTimerInterruptEnable(0)
        , m_CSRFastIRQEnable(0)
        , m_CSRPrivilegeLevel(0)
        , m_CSRMachineTrapCause(0)
        , m_CSRMachineCycleCounterEnable(0)
        , m_CSRMachineInstructionCounterEnable(0)
        , m_CSRMachineInhibitCounterIncrement(0)
        , m_CSRDebugEBreakBehaviourMMode(0)
        , m_CSRDebugEBreakBehaviourUMode(0)
        , m_CSRDebugStep(0)
        , m_CSRDebugPrivilegeMode(0)
        , m_CSRDebugCause(0)
        , m_CSRMatchTriggerEnable(0)
        , m_InstructionFetchAcknowledge(0)
        , m_InstructionFetchReset(0)
        , m_BranchTaken(0)
        , m_MonitorCounter(0)
        , m_EventCounter(0)
        , m_HardwareTriggerFired(0)
        , m_HardwareTriggerStart(0)
        , m_TrapExceptionBuffer(0)
        , m_TrapIRQPending(0)
        , m_TrapIRQBuffer(0)
        , m_TrapCause(0)
        , m_TrapEnvironmentPending(0)
        , m_TrapEnvironmentEnter(0)
        , m_TrapEnvironmentEntered(0)
        , m_TrapEnvironmentExit(0)
        , m_TrapInstructionFetchBusError(0)
        , m_TrapInstructionAlignment(0)
        , m_TrapECall(0)
        , m_TrapEBreak(0)
        , m_DebugRun(0)
        , m_ControlBus{ }
        , m_ControlBusNext{ }
    { }

    void SetResetN(const bool reset_n) noexcept
    {
        p_Reset_n = BOOL_TO_BIT(reset_n);

        TRIGGER_SENSITIVITY(p_Reset_n);
    }

    void SetClock(const bool clock) noexcept
    {
        p_Clock = BOOL_TO_BIT(clock);

        TRIGGER_SENSITIVITY(p_Clock);
    }

    void SetInstructionFetchBus(const InstructionFetchBus& bus) noexcept
    {
        p_InstructionFetchBus = bus;
    }

    void SetPMPFault(const bool pmpFault) noexcept
    {
        p_PMPFault = pmpFault;
    }

    void SetALUCoProcessorDone(const bool aluCoProcessorDone) noexcept
    {
        p_ALUCoProcessorDone = aluCoProcessorDone;
    }

    void SetALUCompareStatus(const u8 compareStatus) noexcept
    {
        p_ALUCompareStatus = compareStatus;
    }

    void SetALUAddressResult(const WordType addressResult) noexcept
    {
        p_ALUAddressResult = addressResult;
    }

    void SetRS1(const WordType rs1) noexcept
    {
        p_RS1 = rs1;
    }

    void SetXCSRReadData(const WordType data) noexcept
    {
        p_XCSRReadData = data;
    }

    void SetIRQDebug(const bool irqDebug) noexcept
    {
        p_IRQDebug = irqDebug;
    }

    void SetIRQMachine(const u8 irqMachine) noexcept
    {
        p_IRQMachine = irqMachine;
    }

    void SetIRQFast(const u16 irqFast) noexcept
    {
        p_IRQFast = irqFast;
    }

    void SetLSUWait(const bool lsuWait) noexcept
    {
        p_LSUWait = lsuWait;
    }

    void SetLSUMemoryAddressRegister(const WordType data) noexcept
    {
        p_LSUMemoryAddressRegister = data;
    }

    void SetLSUError(const u8 error) noexcept
    {
        p_LSUError = error;
    }
private:
    // Wires

    [[nodiscard]] bool BranchCheck() const noexcept
    {
        if(!GetBitBool(m_ExecutionEngine.InstructionWord, InstructionOpcodeLSB + 2))
        {
            // BEQ / BNE
            if(!GetBitBool(m_ExecutionEngine.InstructionWord, InstructionFunction3MSB))
            {
                return GetBitBool(p_ALUCompareStatus, ALUComparatorEqual) ^ GetBitBool(m_ExecutionEngine.InstructionWord, InstructionFunction3LSB);
            }
            else // BLT / BGE
            {
                return GetBitBool(p_ALUCompareStatus, ALUComparatorLessThan) ^ GetBitBool(m_ExecutionEngine.InstructionWord, InstructionFunction3LSB);
            }
        }
        else // Unconditional Branch
        {
            return true;
        }
    }

    [[nodiscard]] u32 OpCode() const noexcept
    {
        constexpr u32 MaskMSB = (1 << (InstructionOpcodeMSB)) - 1;
        constexpr u32 MaskLSB = (1 << (InstructionOpcodeLSB + 2)) - 1;
        constexpr u32 Mask = MaskMSB ^ MaskLSB;

        return (m_ExecutionEngine.InstructionWord & Mask) | 0b11;
    }

    void UpdateControlBus() const noexcept
    {
        constexpr u32 RS1MaskMSB = (1 << (InstructionRS1MSB)) - 1;
        constexpr u32 RS1MaskLSB = (1 << (InstructionRS1LSB)) - 1;
        constexpr u32 RS1Mask = RS1MaskMSB ^ RS1MaskLSB;
        constexpr u32 RS2MaskMSB = (1 << (InstructionRS2MSB)) - 1;
        constexpr u32 RS2MaskLSB = (1 << (InstructionRS2LSB)) - 1;
        constexpr u32 RS2Mask = RS2MaskMSB ^ RS2MaskLSB;
        constexpr u32 RDMaskMSB = (1 << (InstructionDestinationRegisterMSB)) - 1;
        constexpr u32 RDMaskLSB = (1 << (InstructionDestinationRegisterLSB)) - 1;
        constexpr u32 RDMask = RDMaskMSB ^ RDMaskLSB;
        constexpr u32 Funct3MaskMSB = (1 << (InstructionFunction3MSB)) - 1;
        constexpr u32 Funct3MaskLSB = (1 << (InstructionFunction3LSB)) - 1;
        constexpr u32 Funct3Mask = Funct3MaskMSB ^ Funct3MaskLSB;
        constexpr u32 Funct12MaskMSB = 0xFFFF'FFFF;
        constexpr u32 Funct12MaskLSB = (1 << (InstructionFunction12LSB)) - 1;
        constexpr u32 Funct12Mask = Funct12MaskMSB ^ Funct12MaskLSB;

        ControlBus controlBus { };

        controlBus.IF_Fence = m_ControlBus.IF_Fence;
        controlBus.IF_Reset = m_InstructionFetchReset;
        controlBus.IF_Acknowledge = m_InstructionFetchAcknowledge;

        controlBus.PC_Current = m_ExecutionEngine.PC & 0xFFFF'FFFE;
        controlBus.PC_Next = m_ExecutionEngine.PCNext & 0xFFFF'FFFE;
        controlBus.PC_Return = m_ExecutionEngine.ReturnAddress & 0xFFFF'FFFE;

        controlBus.RF_WriteBackEnable = BOOL_TO_BIT(BIT_TO_BOOL(m_ControlBus.RF_WriteBackEnable) && !TrapExceptionFired());
        controlBus.RF_RS1_Address = (m_ExecutionEngine.InstructionWord & RS1Mask) >> InstructionRS1LSB;
        controlBus.RF_RS2_Address = (m_ExecutionEngine.InstructionWord & RS2Mask) >> InstructionRS2LSB;
        controlBus.RF_RD_Address = (m_ExecutionEngine.InstructionWord & RDMask) >> InstructionDestinationRegisterLSB;
        controlBus.RF_ZeroWriteEnable = m_ControlBus.RF_ZeroWriteEnable;

        controlBus.ALU_OperationSelect = m_ControlBus.ALU_OperationSelect;
        controlBus.ALU_Subtract = m_ControlBus.ALU_Subtract;
        controlBus.ALU_OperandSelectA = m_ControlBus.ALU_OperandSelectA;
        controlBus.ALU_OperandSelectB = m_ControlBus.ALU_OperandSelectB;
        controlBus.ALU_Unsigned = m_ControlBus.ALU_Unsigned;
        controlBus.ALU_BaseCoProcessorTrigger = m_ControlBus.ALU_BaseCoProcessorTrigger;
        controlBus.ALU_CFUCoProcessorTrigger = m_ControlBus.ALU_CFUCoProcessorTrigger;
        controlBus.ALU_FPUCoProcessorTrigger = m_ControlBus.ALU_FPUCoProcessorTrigger;
        controlBus.ALU_Immediate = m_Immediate;

        controlBus.LSU_Request = m_ControlBus.LSU_Request;
        controlBus.LSU_ReadWrite = m_ControlBus.LSU_ReadWrite;
        controlBus.LSU_Atomic = m_ControlBus.LSU_Atomic;
        controlBus.LSU_Enable = BOOL_TO_BIT(m_ExecutionEngine.State == ExecutionEngineState::MemoryRequest);
        controlBus.LSU_Privileged = BIT_TO_BOOL(m_CSRMachineStatusEffectivePrivilegeMode) ?  m_CSRMachineStatusPrivilegeModePrev : CSREffectivePrivilegeLevel();
        controlBus.LSU_Fence = m_ControlBus.LSU_Fence;

        controlBus.CSR_WriteEnable = m_CSRWriteEnable;
        controlBus.CSR_ReadEnable = m_CSRReadEnable;
        controlBus.CSR_Address = m_CSRAddress;
        controlBus.CSR_WriteData = CSRWriteData();

        controlBus.IR_Function3 = (m_ExecutionEngine.InstructionWord & Funct3Mask) >> InstructionFunction3LSB;
        controlBus.IR_Function3 = (m_ExecutionEngine.InstructionWord & Funct12Mask) >> InstructionFunction12LSB;
        controlBus.IR_Opcode = OpCode();

        controlBus.CPU_Sleep = 0;
        controlBus.CPU_Privileged = CSREffectivePrivilegeLevel();
        controlBus.CPU_Trap = m_TrapEnvironmentEnter;
        controlBus.CPU_Debug = m_DebugRun;

        m_Parent->ReceiveController_ControlBus(m_Index, controlBus);
    }

    [[nodiscard]] bool MonitorException() const noexcept
    {
        return GetBitBool(m_MonitorCounter, MonitorMultiCycleTimeOut);
    }

    [[nodiscard]] u8 CSRValid() const noexcept
    {
        constexpr u32 RS1MaskMSB = (1 << (InstructionRS1MSB)) - 1;
        constexpr u32 RS1MaskLSB = (1 << (InstructionRS1LSB)) - 1;
        constexpr u32 RS1Mask = RS1MaskMSB ^ RS1MaskLSB;
        constexpr u32 Funct3MaskMSB = (1 << (InstructionFunction3MSB)) - 1;
        constexpr u32 Funct3MaskLSB = (1 << (InstructionFunction3LSB)) - 1;
        constexpr u32 Funct3Mask = Funct3MaskMSB ^ Funct3MaskLSB;
        constexpr u32 Funct12MaskMSB = 0xFFFF'FFFF;
        constexpr u32 Funct12MaskLSB = (1 << (InstructionFunction12LSB)) - 1;
        constexpr u32 Funct12Mask = Funct12MaskMSB ^ Funct12MaskLSB;

        const u32 rs1 = (m_ExecutionEngine.InstructionWord & RS1Mask) >> InstructionRS1LSB;
        const u32 funct3 = (m_ExecutionEngine.InstructionWord & Funct3Mask) >> InstructionFunction3LSB;
        const u32 funct12 = (m_ExecutionEngine.InstructionWord & Funct12Mask) >> InstructionFunction12LSB;

        bool valid2 = false;
        switch(funct12)
        {
            case CSRCFURegister0:
            case CSRCFURegister1:
            case CSRCFURegister2:
            case CSRCFURegister3:
                valid2 = EnableISA_Zxcfu;
                break;
            case CSRFloatingPointFlags:
            case CSRFloatingPointRM:
            case CSRFloatingPointRegister:
                valid2 = EnableISA_Zfinx;
                break;
            case CSRMachineStatus:
            case CSRMachineStatusHigh:
            case CSRMachineISA:
            case CSRMachineInterruptEnable:
            case CSRMachineTrapHandlerAddress:
            case CSRMachineScratch:
            case CSRMachineExceptionPC:
            case CSRMachineCause:
            case CSRMachineIP:
            case CSRMachineTriggerValue:
            case CSRMachineInstruction:
            case CSRMachineVendorID:
            case CSRMachineArchitectureID:
            case CSRMachineImplementationID:
            case CSRMachineHartID:
            case CSRMachineConfigPointer:
                valid2 = true;
                break;
            case CSRMachineCounterEnable:
            case CSRMachineEnvironmentConfig:
            case CSRMachineEnvironmentConfigHigh:
                valid2 = EnableISA_U;
                break;
            case CSRPMPConfig0:
            case CSRPMPConfig1:
            case CSRPMPConfig2:
            case CSRPMPConfig3:
            case CSRPMPAddress0:
            case CSRPMPAddress1:
            case CSRPMPAddress2:
            case CSRPMPAddress3:
            case CSRPMPAddress4:
            case CSRPMPAddress5:
            case CSRPMPAddress6:
            case CSRPMPAddress7:
            case CSRPMPAddress8:
            case CSRPMPAddress9:
            case CSRPMPAddress10:
            case CSRPMPAddress11:
            case CSRPMPAddress12:
            case CSRPMPAddress13:
            case CSRPMPAddress14:
            case CSRPMPAddress15:
                valid2 = EnableISA_Smpmp;
                break;
            case CSRDebugCSR:
            case CSRDebugPC:
            case CSRDebugScratch0:
                valid2 = EnableISA_Sdext;
                break;
            case CSRTriggerSelect:
            case CSRTriggerData1:
            case CSRTriggerData2:
            case CSRTriggerInfo:
                valid2 = EnableISA_Sdtrig;
                break;
            default: break;
        }

        bool valid1 = false;

        if(
            (funct12 & 0xC00) >> 10 == 0b11 &&
            (
                funct3 == Function3CSRReadWrite ||
                funct3 == Function3CSRReadWriteImmediate ||
                rs1 != 0
            )
        )
        {
            valid1 = true;
        }

        bool valid0;

        if((funct12 & 0xFF0) == (CSRDebugCSR & 0xFF0) && EnableISA_Sdext && !BIT_TO_BOOL(m_DebugRun))
        {
            valid0 = false;
        }
        else if((funct12 & 0x300) >> 8 == 0b00 && !CSREffectivePrivilegeLevel())
        {
            valid0 = false;
        }
        else
        {
            valid0 = true;
        }

        return (BOOL_TO_BIT(valid2) << 2) | (BOOL_TO_BIT(valid1) << 1) | BOOL_TO_BIT(valid0);
    }

    [[nodiscard]] bool IllegalCommand() const noexcept
    {
        constexpr u32 OpCodeMaskMSB = (1 << (InstructionOpcodeMSB)) - 1;
        constexpr u32 OpCodeMaskLSB = (1 << (InstructionOpcodeLSB + 2)) - 1;
        constexpr u32 OpCodeMask = OpCodeMaskMSB ^ OpCodeMaskLSB;
        constexpr u32 RS1MaskMSB = (1 << (InstructionRS1MSB)) - 1;
        constexpr u32 RS1MaskLSB = (1 << (InstructionRS1LSB)) - 1;
        constexpr u32 RS1Mask = RS1MaskMSB ^ RS1MaskLSB;
        constexpr u32 RS2MaskMSB = (1 << (InstructionRS2MSB)) - 1;
        constexpr u32 RS2MaskLSB = (1 << (InstructionRS2LSB)) - 1;
        constexpr u32 RS2Mask = RS2MaskMSB ^ RS2MaskLSB;
        constexpr u32 RDMaskMSB = (1 << (InstructionDestinationRegisterMSB)) - 1;
        constexpr u32 RDMaskLSB = (1 << (InstructionDestinationRegisterLSB)) - 1;
        constexpr u32 RDMask = RDMaskMSB ^ RDMaskLSB;
        constexpr u32 Funct3MaskMSB = (1 << (InstructionFunction3MSB)) - 1;
        constexpr u32 Funct3MaskLSB = (1 << (InstructionFunction3LSB)) - 1;
        constexpr u32 Funct3Mask = Funct3MaskMSB ^ Funct3MaskLSB;
        constexpr u32 Funct5MaskMSB = 0xFFFF'FFFF;
        constexpr u32 Funct5MaskLSB = (1 << (InstructionFunction5LSB)) - 1;
        constexpr u32 Funct5Mask = Funct5MaskMSB ^ Funct5MaskLSB;
        constexpr u32 Funct12MaskMSB = 0xFFFF'FFFF;
        constexpr u32 Funct12MaskLSB = (1 << (InstructionFunction12LSB)) - 1;
        constexpr u32 Funct12Mask = Funct12MaskMSB ^ Funct12MaskLSB;

        const u32 funct3 = (m_ExecutionEngine.InstructionWord & Funct3Mask) >> InstructionFunction3LSB;
        const u32 funct5 = (m_ExecutionEngine.InstructionWord & Funct5Mask) >> InstructionFunction5LSB;
        const u32 funct12 = (m_ExecutionEngine.InstructionWord & Funct12Mask) >> InstructionFunction12LSB;

        switch((m_ExecutionEngine.InstructionWord & OpCodeMask) >> InstructionOpcodeLSB)
        {
            // U-Instruction
            case OpCodeLoadUpperImmediate:
            case OpCodeAddUpperImmediate:
            case OpCodeJumpAndLink:
                // All encodings are valid.
                return false;
            case OpCodeJumpAndLinkWithRegister:
                // Unconditional Jump and Link
                if(funct3 == 0b000)
                {
                    return false;
                }
                break;
            case OpCodeBranch:
                // Conditional Branch
                switch(funct3)
                {
                    case Function3BranchIfEqual:
                    case Function3BranchIfNotEqual:
                    case Function3BranchIfLessThan:
                    case Function3BranchIfGreaterThanOrEqual:
                    case Function3BranchIfLessThanUnsigned:
                    case Function3BranchIfGreaterThanOrEqualUnsigned:
                        return false;
                    default: break;
                }
                break;
            case OpCodeLoad:
                // Memory Load
                switch(funct3)
                {
                    case Function3LoadByteSigned:
                    case Function3LoadHalfWordSigned:
                    case Function3LoadWordSigned:
                    case Function3LoadByteUnsigned:
                    case Function3LoadHalfWordUnsigned:
                        return false;
                    case Function3LoadWordUnsigned:
                        if constexpr(WordBitCount == 64)
                        {
                            return false;
                        }
                    default: break;
                }
                break;
            case OpCodeStore:
                // Memory Store
                switch(funct3)
                {
                    case Function3StoreByte:
                    case Function3StoreHalfWord:
                    case Function3StoreWord:
                        return false;
                    default: break;
                }
                break;
            case OpCodeAtomic:
                // Atomic Memory Operation
                if(funct3 == 0b010)
                {
                    switch(funct5)
                    {
                        case 0b00001:
                        case 0b00000:
                        case 0b00100:
                        case 0b01100:
                        case 0b01000:
                        case 0b10000:
                        case 0b10100:
                        case 0b11000:
                        case 0b11100:
                            if constexpr(EnableISA_Zaamo)
                            {
                                return false;
                            }
                        case 0b00010:
                        case 0b00011:
                            if constexpr(EnableISA_Zalrsc)
                            {
                                return false;
                            }
                        default: break;
                    }
                }
                break;
            case OpCodeALU:
            case OpCodeALUImmediate:
            case OpCodeFloatingPoint:
            case OpCodeCustom0:
            case OpCodeCustom1:
                return false;
            case OpCodeFence:
                if((funct3 & 0b110) == (Function3Fence & 0b110))
                {
                    return false;
                }
                break;
            case OpCodeSystem:
                if(funct3 == Function3Environment)
                {
                    switch(funct12)
                    {
                        case Function12ECall:
                        case Function12EBreak:
                            return false;
                        case Function12MachineReturn: return true; // mret only allowed in real / non-debug Machine mode
                        case Function12DebugReturn: return true; // dret only allowed in debug mode
                        case Function12WFI: return true; // wfi only in Machine mode or if TW is zero
                        default: break;
                    }
                }
                else if(true && funct3 != Function3CSRIllegal)
                {
                    return false;
                }
                break;
            default: break;
        }

        return true;
    }

    [[nodiscard]] bool TrapIsIllegalInstruction() const noexcept
    {
        return (m_ExecutionEngine.State == ExecutionEngineState::Execute || m_ExecutionEngine.State == ExecutionEngineState::ALUWait) &&
            (MonitorException() || IllegalCommand());
    }

    [[nodiscard]] u32 TrapExceptionProgramCounter() const noexcept
    {
        if(GetBitBool(m_TrapCause, 6))
        {
            return m_ExecutionEngine.PCNext;
        }
        else
        {
            return m_ExecutionEngine.PC;
        }
    }

    [[nodiscard]] bool TrapExceptionFired() const noexcept
    {
        return m_TrapExceptionBuffer != 0;
    }

    [[nodiscard]] bool TrapIRQFire0() const noexcept
    {
        constexpr u32 IRQMaskMSB = (1 << static_cast<u32>(InterruptSources::Fast15)) - 1;
        constexpr u32 IRQMaskLSB = (1 << static_cast<u32>(InterruptSources::MachineSoftwareInterrupt)) - 1;
        constexpr u32 IRQMask = IRQMaskMSB ^ IRQMaskLSB;

        return m_ExecutionEngine.State == ExecutionEngineState::Execute &&
            (m_TrapIRQBuffer & IRQMask) != 0 &&
            (BIT_TO_BOOL(m_CSRMachineStatusIRQEnable) || m_CSRPrivilegeLevel == PrivilegeModeUser) &&
            !BIT_TO_BOOL(m_DebugRun) && !BIT_TO_BOOL(m_CSRDebugStep);
    }

    [[nodiscard]] bool TrapIRQFire1() const noexcept
    {
        return (m_ExecutionEngine.State == ExecutionEngineState::Execute || m_ExecutionEngine.State == ExecutionEngineState::Branched) &&
            GetBitBool(m_TrapIRQBuffer, InterruptSources::DebugHalt);
    }

    [[nodiscard]] bool TrapIRQFire2() const noexcept
    {
        return (m_ExecutionEngine.State == ExecutionEngineState::Execute ||
            (BIT_TO_BOOL(m_TrapEnvironmentEntered) && m_ExecutionEngine.State == ExecutionEngineState::Branched)) &&
            GetBitBool(m_TrapIRQBuffer, InterruptSources::DebugStep);
    }

    [[nodiscard]] WordType CSROperand() const noexcept
    {
        if(!GetBitBool(m_ExecutionEngine.InstructionWord, InstructionFunction3MSB))
        {
            return p_RS1;
        }
        else
        {
            return (m_ExecutionEngine.InstructionWord & 0x000F'8000) >> 15;
        }
    }

    [[nodiscard]] WordType CSRWriteData() const noexcept
    {
        constexpr u32 Funct3MaskMSB = (1 << (InstructionFunction3MSB - 1)) - 1;
        constexpr u32 Funct3MaskLSB = (1 << (InstructionFunction3LSB)) - 1;
        constexpr u32 Funct3Mask = Funct3MaskMSB ^ Funct3MaskLSB;

        switch((m_ExecutionEngine.InstructionWord & Funct3Mask) >> InstructionFunction3LSB)
        {
            case 0b10: return m_CSRReadData | CSROperand();  // Set
            case 0b11: return m_CSRReadData & ~CSROperand(); // Clear
            default: return CSROperand();                    // Write
        }
    }

    void UpdateCSRReadData() const noexcept
    {
        m_Parent->ReceiveController_CSRReadData(m_Index, m_CSRReadData);
    }

    [[nodiscard]] u32 CSREffectivePrivilegeLevel() const noexcept
    {
        return BIT_TO_BOOL(m_DebugRun) ? PrivilegeModeMachine : m_CSRPrivilegeLevel;
    }

    // ReSharper disable once CppDFAConstantFunctionResult
    [[nodiscard]] bool DebugTriggerHardware() const noexcept
    {
        if constexpr(EnableISA_Sdext)
        {
            return m_HardwareTriggerStart && !BIT_TO_BOOL(m_DebugRun);
        }
        else
        {
            return false;
        }
    }

    [[nodiscard]] bool DebugTriggerBreak() const noexcept
    {
        if constexpr(EnableISA_Sdext)
        {
            return m_TrapEBreak && (BIT_TO_BOOL(m_DebugRun) ||
                (BIT_TO_BOOL(m_CSRPrivilegeLevel) && BIT_TO_BOOL(m_CSRDebugEBreakBehaviourMMode)) ||
                (!BIT_TO_BOOL(m_CSRPrivilegeLevel) && BIT_TO_BOOL(m_CSRDebugEBreakBehaviourUMode)));
        }
        else
        {
            return false;
        }
    }

    [[nodiscard]] bool DebugTriggerHalt() const noexcept
    {
        if constexpr(EnableISA_Sdext)
        {
            return p_IRQDebug && !BIT_TO_BOOL(m_DebugRun);
        }
        else
        {
            return false;
        }
    }

    [[nodiscard]] bool DebugTriggerStep() const noexcept
    {
        if constexpr(EnableISA_Sdext)
        {
            return BIT_TO_BOOL(m_CSRDebugStep) && !BIT_TO_BOOL(m_DebugRun);
        }
        else
        {
            return false;
        }
    }

    [[nodiscard]] WordType CSRDebugRegisterReadBack() const noexcept
    {
        DebugControlAndStatusRegister ret;

        ret.Step = m_CSRDebugStep;
        ret.DebugCause = m_CSRDebugCause;
        if constexpr(EnableISA_U)
        {
            ret.EBreakUserMode = m_CSRDebugEBreakBehaviourUMode;
        }
        else
        {
            ret.EBreakUserMode = 0;
        }
        ret.EBreakMachineMode = m_CSRDebugEBreakBehaviourMMode;

        return ::std::bit_cast<WordType>(ret);
    }

    [[nodiscard]] bool HardwareTriggerMatch() const noexcept
    {
        if constexpr(EnableISA_Sdtrig)
        {
            return BIT_TO_BOOL(m_CSRMatchTriggerEnable) && !BIT_TO_BOOL(m_HardwareTriggerFired) && (m_CSRTriggerAddressMatchRegister & 0xFFFF'FFFE) == (m_ExecutionEngine.PCNext & 0xFFFF'FFFE);
        }
        else
        {
            return false;
        }
    }

    [[nodiscard]] WordType CSRTriggerData1ReadBack() const noexcept
    {
        TriggerMatchControlAndStatusRegister ret;

        ret.Execute = m_CSRMatchTriggerEnable;
        ret.Hit0 = m_HardwareTriggerFired;

        return ::std::bit_cast<WordType>(ret);
    }
private:
    PROCESSES_DECL()
    {
        PROCESS_ENTER(ImmediateGenerator, p_Reset_n, p_Clock);
        PROCESS_ENTER(ExecutionEngineFSMSync, p_Reset_n, p_Clock);
        PROCESS_ENTER(MultiCycleMonitor, p_Reset_n, p_Clock);
        PROCESS_ENTER(TrapBuffer, p_Reset_n, p_Clock);
        PROCESS_ENTER(TrapPriority, p_Reset_n, p_Clock);
        PROCESS_ENTER(TrapController, p_Reset_n, p_Clock);
        PROCESS_ENTER(CSRAddress, p_Reset_n, p_Clock);
        PROCESS_ENTER(CSRWrite, p_Reset_n, p_Clock);
        if constexpr(EnableISA_Sdext)
        {
            PROCESS_ENTER(DebugControl, p_Reset_n, p_Clock);
        }
        if constexpr(EnableISA_Sdtrig)
        {
            PROCESS_ENTER(HardwareTriggerException, p_Reset_n, p_Clock);
        }
    }

    PROCESS_DECL(ImmediateGenerator)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_Immediate = 0;

            UpdateControlBus();
        }
        else if(RISING_EDGE(p_Clock))
        {
            // Prepare update of the next PC (using ALUs PC + IMM in Execute state).
            if(m_ExecutionEngine.State == ExecutionEngineState::Dispatch)
            {
                m_Immediate = 4;
            }
            else
            {
                switch(OpCode())
                {
                    case OpCodeStore: // S-Immediate
                        m_Immediate =
                            (ReplicateToVector<21, WordType>(GetBitBool(m_ExecutionEngine.InstructionWord, 31)) << 11) |
                            ((m_ExecutionEngine.InstructionWord & 0x7E00'0000) >> 20) |
                            ((m_ExecutionEngine.InstructionWord & 0x0000'0F80) >> 7);
                        break;
                    case OpCodeBranch: // B-Immediate
                        m_Immediate =
                            (ReplicateToVector<20, WordType>(GetBitBool(m_ExecutionEngine.InstructionWord, 31)) << 12) |
                            ((m_ExecutionEngine.InstructionWord & 0x0000'0080) << 4) |
                            ((m_ExecutionEngine.InstructionWord & 0x7E00'0000) >> 20) |
                            ((m_ExecutionEngine.InstructionWord & 0x0000'0F00) >> 7);
                        break;
                    case OpCodeLoadUpperImmediate:
                    case OpCodeAddUpperImmediate: // U-Immediate
                        m_Immediate = m_ExecutionEngine.InstructionWord & 0xFFFF'F000;
                        break;
                    case OpCodeJumpAndLink: // J-Immediate
                        m_Immediate =
                            (ReplicateToVector<12, WordType>(GetBitBool(m_ExecutionEngine.InstructionWord, 31)) << 12) |
                            (m_ExecutionEngine.InstructionWord & 0x00F'F000) |
                            ((m_ExecutionEngine.InstructionWord & 0x0010'0000) >> 9) |
                            ((m_ExecutionEngine.InstructionWord & 0x7FE0'0000) >> 20);
                        break;
                    case OpCodeAtomic: // Atomic Memory Access
                        m_Immediate = 0;
                        break;
                    default: // I-Immediate
                        m_Immediate =
                            (ReplicateToVector<21, WordType>(GetBitBool(m_ExecutionEngine.InstructionWord, 31)) << 11) |
                            ((m_ExecutionEngine.InstructionWord & 0x7FF0'0000) >> 20);
                        break;
                }
            }

            UpdateControlBus();
        }
    }

    PROCESS_DECL(ExecutionEngineFSMSync)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_ControlBus = {};
            m_ExecutionEngine.State = ExecutionEngineState::Restart;
            m_ExecutionEngine.InstructionWord = 0;
            m_ExecutionEngine.PC = BootAddress & 0xFFFF'FFFC; // Align to 32-Bit byte boundary
            m_ExecutionEngine.PCNext = m_ExecutionEngine.PC;
            m_ExecutionEngine.ReturnAddress = 0;

            UpdateControlBus();
        }
        else if(RISING_EDGE(p_Clock))
        {
            m_ControlBus = m_ControlBusNext;

            m_ExecutionEngine = m_ExecutionEngineNext;

            UpdateControlBus();
        }
    }

    PROCESS_DECL(MultiCycleMonitor)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_MonitorCounter = 0;
        }
        else if(RISING_EDGE(p_Clock))
        {
            if(m_ExecutionEngine.State == ExecutionEngineState::ALUWait)
            {
                ++m_MonitorCounter;
            }
            else
            {
                m_MonitorCounter = 0;
            }
        }
    }

    PROCESS_DECL(TrapBuffer)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_TrapIRQPending = 0;
            m_TrapIRQBuffer = 0;
            m_TrapExceptionBuffer = 0;

            UpdateControlBus();
        }
        else if(RISING_EDGE(p_Clock))
        {
            // Interrupt-Pending Buffer
            // Once triggered the interrupt line should stay active until explicitly
            // cleared by a mechanism specific to the interrupt-causing source.

            // RISC-V Machine Interrupts
            m_TrapIRQPending = SetBit(m_TrapIRQPending, InterruptSources::MachineSoftwareInterrupt, GetBitBool(p_IRQMachine, 0));
            m_TrapIRQPending = SetBit(m_TrapIRQPending, InterruptSources::MachineExternalInterrupt, GetBitBool(p_IRQMachine, 1));
            m_TrapIRQPending = SetBit(m_TrapIRQPending, InterruptSources::MachineTimerInterrupt, GetBitBool(p_IRQMachine, 2));

            // Custom Fast Interrupts
            for(uSys i = 0; i < 16; ++i)
            {
                m_TrapIRQPending = SetBit(m_TrapIRQPending, static_cast<u32>(InterruptSources::Fast0) + i, GetBitBool(p_IRQFast, i));
            }

            // Debug Mode Entry
            m_TrapIRQPending = SetBit(m_TrapIRQPending, InterruptSources::DebugHalt, false);
            m_TrapIRQPending = SetBit(m_TrapIRQPending, InterruptSources::DebugStep, false);


            // Interrupt Buffer
            // Masking of interrupt request lines. Additionally, this buffer ensures
            // that an active interrupt request line stays active (even when
            // disabled via MIE) if the trap environment is already starting.

            // RISC-V Machine Interrupts
            m_TrapIRQBuffer = SetBit(m_TrapIRQBuffer, InterruptSources::MachineSoftwareInterrupt, (GetBitBool(m_TrapIRQPending, InterruptSources::MachineSoftwareInterrupt) && true) || (BIT_TO_BOOL(m_TrapEnvironmentPending) && GetBitBool(m_TrapIRQBuffer, InterruptSources::MachineSoftwareInterrupt)));
            m_TrapIRQBuffer = SetBit(m_TrapIRQBuffer, InterruptSources::MachineExternalInterrupt, (GetBitBool(m_TrapIRQPending, InterruptSources::MachineExternalInterrupt) && true) || (BIT_TO_BOOL(m_TrapEnvironmentPending) && GetBitBool(m_TrapIRQBuffer, InterruptSources::MachineExternalInterrupt)));
            m_TrapIRQBuffer = SetBit(m_TrapIRQBuffer, InterruptSources::MachineTimerInterrupt, (GetBitBool(m_TrapIRQPending, InterruptSources::MachineTimerInterrupt) && true) || (BIT_TO_BOOL(m_TrapEnvironmentPending) && GetBitBool(m_TrapIRQBuffer, InterruptSources::MachineTimerInterrupt)));

            // Custom Fast Interrupts
            for(uSys i = 0; i < 16; ++i)
            {
                m_TrapIRQBuffer = SetBit(m_TrapIRQBuffer, static_cast<u32>(InterruptSources::Fast0) + i, (GetBitBool(m_TrapIRQPending, static_cast<u32>(InterruptSources::Fast0) + i) && true) || (BIT_TO_BOOL(m_TrapEnvironmentPending) && GetBitBool(m_TrapIRQBuffer, static_cast<u32>(InterruptSources::Fast0) + i)));
            }

            // Debug Mode Entry
            m_TrapIRQBuffer = SetBit(m_TrapIRQBuffer, InterruptSources::DebugHalt, false || (BIT_TO_BOOL(m_TrapEnvironmentPending) && GetBitBool(m_TrapIRQBuffer, InterruptSources::DebugHalt)));
            m_TrapIRQBuffer = SetBit(m_TrapIRQBuffer, InterruptSources::DebugStep, false || (BIT_TO_BOOL(m_TrapEnvironmentPending) && GetBitBool(m_TrapIRQBuffer, InterruptSources::DebugStep)));

            // Exception Buffer
            // If several exception sources trigger at once, all the requests will
            // stay active until the trap environment is started. Only the exception
            // with the highest priority will be used to update the MCAUSE CSR. All
            // remaining ones will be discarded.

            // Misaligned Load/Store/Instruction Address
            m_TrapExceptionBuffer = SetBit(m_TrapExceptionBuffer, ExceptionTypes::LoadAddressAlignment, (GetBit(m_TrapExceptionBuffer, ExceptionTypes::LoadAddressAlignment) || GetBit(p_LSUError, 0)) && !BIT_TO_BOOL(m_TrapEnvironmentEnter));
            m_TrapExceptionBuffer = SetBit(m_TrapExceptionBuffer, ExceptionTypes::StoreAddressAlignment, (GetBit(m_TrapExceptionBuffer, ExceptionTypes::StoreAddressAlignment) || GetBit(p_LSUError, 2)) && !BIT_TO_BOOL(m_TrapEnvironmentEnter));
            m_TrapExceptionBuffer = SetBit(m_TrapExceptionBuffer, ExceptionTypes::InstructionAddressAlignment, (GetBit(m_TrapExceptionBuffer, ExceptionTypes::InstructionAddressAlignment) || BIT_TO_BOOL(m_TrapInstructionAlignment)) && !BIT_TO_BOOL(m_TrapEnvironmentEnter));

            // Load/Store/Instruction Access Fault
            m_TrapExceptionBuffer = SetBit(m_TrapExceptionBuffer, ExceptionTypes::LoadAccessFault, (GetBit(m_TrapExceptionBuffer, ExceptionTypes::LoadAccessFault) || GetBit(p_LSUError, 1)) && !BIT_TO_BOOL(m_TrapEnvironmentEnter));
            m_TrapExceptionBuffer = SetBit(m_TrapExceptionBuffer, ExceptionTypes::StoreAccessFault, (GetBit(m_TrapExceptionBuffer, ExceptionTypes::StoreAccessFault) || GetBit(p_LSUError, 3)) && !BIT_TO_BOOL(m_TrapEnvironmentEnter));
            m_TrapExceptionBuffer = SetBit(m_TrapExceptionBuffer, ExceptionTypes::InstructionAccessFault, (GetBit(m_TrapExceptionBuffer, ExceptionTypes::InstructionAccessFault) || BIT_TO_BOOL(m_TrapInstructionFetchBusError)) && !BIT_TO_BOOL(m_TrapEnvironmentEnter));

            // Environment Call & Illegal Instruction
            m_TrapExceptionBuffer = SetBit(m_TrapExceptionBuffer, ExceptionTypes::EnvironmentCall, (GetBit(m_TrapExceptionBuffer, ExceptionTypes::EnvironmentCall) || BIT_TO_BOOL(m_TrapECall)) && !BIT_TO_BOOL(m_TrapEnvironmentEnter));
            m_TrapExceptionBuffer = SetBit(m_TrapExceptionBuffer, ExceptionTypes::IllegalInstruction, (GetBit(m_TrapExceptionBuffer, ExceptionTypes::IllegalInstruction) || TrapIsIllegalInstruction()) && !BIT_TO_BOOL(m_TrapEnvironmentEnter));

            // TODO: Finish

            UpdateControlBus();
        }
    }

    PROCESS_DECL(TrapPriority)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_TrapCause = 0;
        }
        else if(RISING_EDGE(p_Clock))
        {
            m_TrapCause = 0;

            // Standard RISC-V Exceptions
            if(GetBitBool(m_TrapExceptionBuffer, ExceptionTypes::InstructionAccessFault))
            {
                m_TrapCause = TrapInstructionAccessFault;
            }
            else if(GetBitBool(m_TrapExceptionBuffer, ExceptionTypes::IllegalInstruction))
            {
                m_TrapCause = TrapIllegalInstruction;
            }
            else if(GetBitBool(m_TrapExceptionBuffer, ExceptionTypes::InstructionAddressAlignment))
            {
                m_TrapCause = TrapInstructionMisaligned;
            }
            else if(GetBitBool(m_TrapExceptionBuffer, ExceptionTypes::EnvironmentCall))
            {
                m_TrapCause = TrapEnvironment | ReplicateToVector<2, u32>(0);
            }
            else if(GetBitBool(m_TrapExceptionBuffer, ExceptionTypes::Breakpoint))
            {
                m_TrapCause = TrapEnvironmentBreakpoint;
            }
            else if(GetBitBool(m_TrapExceptionBuffer, ExceptionTypes::StoreAddressAlignment))
            {
                m_TrapCause = TrapStoreAddressMisaligned;
            }
            else if(GetBitBool(m_TrapExceptionBuffer, ExceptionTypes::LoadAddressAlignment))
            {
                m_TrapCause = TrapLoadAddressMisaligned;
            }
            else if(GetBitBool(m_TrapExceptionBuffer, ExceptionTypes::StoreAccessFault))
            {
                m_TrapCause = TrapStoreAccessFault;
            }
            else if(GetBitBool(m_TrapExceptionBuffer, ExceptionTypes::LoadAccessFault))
            {
                m_TrapCause = TrapLoadAccessFault;
            }
            // Standard RISC-V Debug Mode Exceptions
            else if(GetBitBool(m_TrapIRQBuffer, InterruptSources::DebugHalt))
            {
                m_TrapCause = TrapDebugHalt;
            }
            else if(GetBitBool(m_TrapExceptionBuffer, ExceptionTypes::DebugHardwareTrap))
            {
                m_TrapCause = TrapDebugHardwareBreak;
            }
            else if(GetBitBool(m_TrapExceptionBuffer, ExceptionTypes::DebugBreakpoint))
            {
                m_TrapCause = TrapDebugBreakInstruction;
            }
            else if(GetBitBool(m_TrapIRQBuffer, InterruptSources::DebugStep))
            {
                m_TrapCause = TrapDebugStep;
            }
            // Custom Fast Interrupts
            else if(GetBitBool(m_TrapIRQBuffer, InterruptSources::Fast0))
            {
                m_TrapCause = TrapFastIRQ0;
            }
            else if(GetBitBool(m_TrapIRQBuffer, InterruptSources::Fast1))
            {
                m_TrapCause = TrapFastIRQ1;
            }
            else if(GetBitBool(m_TrapIRQBuffer, InterruptSources::Fast2))
            {
                m_TrapCause = TrapFastIRQ2;
            }
            else if(GetBitBool(m_TrapIRQBuffer, InterruptSources::Fast3))
            {
                m_TrapCause = TrapFastIRQ3;
            }
            else if(GetBitBool(m_TrapIRQBuffer, InterruptSources::Fast4))
            {
                m_TrapCause = TrapFastIRQ4;
            }
            else if(GetBitBool(m_TrapIRQBuffer, InterruptSources::Fast5))
            {
                m_TrapCause = TrapFastIRQ5;
            }
            else if(GetBitBool(m_TrapIRQBuffer, InterruptSources::Fast6))
            {
                m_TrapCause = TrapFastIRQ6;
            }
            else if(GetBitBool(m_TrapIRQBuffer, InterruptSources::Fast7))
            {
                m_TrapCause = TrapFastIRQ7;
            }
            else if(GetBitBool(m_TrapIRQBuffer, InterruptSources::Fast8))
            {
                m_TrapCause = TrapFastIRQ8;
            }
            else if(GetBitBool(m_TrapIRQBuffer, InterruptSources::Fast9))
            {
                m_TrapCause = TrapFastIRQ9;
            }
            else if(GetBitBool(m_TrapIRQBuffer, InterruptSources::Fast10))
            {
                m_TrapCause = TrapFastIRQ10;
            }
            else if(GetBitBool(m_TrapIRQBuffer, InterruptSources::Fast11))
            {
                m_TrapCause = TrapFastIRQ11;
            }
            else if(GetBitBool(m_TrapIRQBuffer, InterruptSources::Fast12))
            {
                m_TrapCause = TrapFastIRQ12;
            }
            else if(GetBitBool(m_TrapIRQBuffer, InterruptSources::Fast13))
            {
                m_TrapCause = TrapFastIRQ13;
            }
            else if(GetBitBool(m_TrapIRQBuffer, InterruptSources::Fast14))
            {
                m_TrapCause = TrapFastIRQ14;
            }
            else if(GetBitBool(m_TrapIRQBuffer, InterruptSources::Fast15))
            {
                m_TrapCause = TrapFastIRQ15;
            }
            // Standard RISC-V Interrupts
            else if(GetBitBool(m_TrapIRQBuffer, InterruptSources::MachineExternalInterrupt))
            {
                m_TrapCause = TrapMachineExternalInterrupt;
            }
            else if(GetBitBool(m_TrapIRQBuffer, InterruptSources::MachineSoftwareInterrupt))
            {
                m_TrapCause = TrapMachineSoftwareInterrupt;
            }
            else if(GetBitBool(m_TrapIRQBuffer, InterruptSources::MachineTimerInterrupt))
            {
                m_TrapCause = TrapMachineTimerInterrupt;
            }
        }
    }

    PROCESS_DECL(TrapController)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_TrapEnvironmentPending = 0;
            m_TrapEnvironmentEntered = 0;
        }
        else if(RISING_EDGE(p_Clock))
        {
            // No pending trap environment
            if(!BIT_TO_BOOL(m_TrapEnvironmentPending))
            {
                if(TrapExceptionFired() || TrapIRQFire0() || TrapIRQFire1() || TrapIRQFire2())
                {
                    m_TrapEnvironmentPending = 1;
                }
            }
            else // Trap waiting to be served
            {
                if(BIT_TO_BOOL(m_TrapEnvironmentEnter))
                {
                    m_TrapEnvironmentPending = 0;
                }
            }

            // Trap environment entered.
            // First instruction of trap environment is executing.
            if(m_ExecutionEngine.State == ExecutionEngineState::Execute)
            {
                m_TrapEnvironmentEntered = 0;
            }
            else if(BIT_TO_BOOL(m_TrapEnvironmentEnter))
            {
                m_TrapEnvironmentEntered = 1;
            }
        }
    }

    PROCESS_DECL(CSRAddress)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_CSRAddress = 0;
        }
        else if(RISING_EDGE(p_Clock))
        {
            if(OpCode() == OpCodeSystem)
            {
                constexpr u32 Funct12MaskMSB = 0xFFFF'FFFF;
                constexpr u32 Funct12MaskLSB = (1 << (InstructionFunction12LSB)) - 1;
                constexpr u32 Funct12Mask = Funct12MaskMSB ^ Funct12MaskLSB;

                m_CSRAddress = (m_ExecutionEngine.InstructionWord & Funct12Mask) >> InstructionFunction12LSB;
            }
        }
    }

    PROCESS_DECL(CSRWrite)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_CSRWriteEnable = 0;
            m_CSRPrivilegeLevel = PrivilegeModeMachine;
            m_CSRMachineStatusIRQEnable = 0;
            m_CSRMachineStatusIRQEnablePrev = 0;
            m_CSRMachineStatusPrivilegeModePrev = PrivilegeModeMachine;
            m_CSRMachineStatusEffectivePrivilegeMode = 0;
            m_CSRMachineStatusTW = 0;
            m_CSRMachineSoftwareInterruptEnable = 0;
            m_CSRMachineExternalInterruptEnable = 0;
            m_CSRMachineTimerInterruptEnable = 0;
            m_CSRFastIRQEnable = 0;
            m_CSRMachineTrapHandlerAddress = 0;
            m_CSRMachineScratchRegister = 0;
            m_CSRMachineExceptionPC = 0;
            m_CSRMachineTrapCause = 0;
            m_CSRMachineBadAddress = 0;
            m_CSRMachineTrapInstruction = 0;
            m_CSRMachineCycleCounterEnable = 0;
            m_CSRMachineInstructionCounterEnable = 0;
            m_CSRMachineInhibitCounterIncrement = 0;
            m_CSRDebugEBreakBehaviourMMode = 0;
            m_CSRDebugEBreakBehaviourUMode = 0;
            m_CSRDebugStep = 0;
            m_CSRDebugPrivilegeMode = 0;
            m_CSRDebugCause = 0;
            m_CSRDebugPC = 0;
            m_CSRDebugScratchRegister0 = 0;
            m_CSRMatchTriggerEnable = 0;
            m_CSRTriggerAddressMatchRegister = 0;
        }
        else if(RISING_EDGE(p_Clock))
        {
            m_CSRWriteEnable = BOOL_TO_BIT(
                BIT_TO_BOOL(m_CSRWriteEnableNext) &&
                (
                    !GetBitBool(m_TrapExceptionBuffer, ExceptionTypes::InstructionAddressAlignment) ||
                    !GetBitBool(m_TrapExceptionBuffer, ExceptionTypes::IllegalInstruction) ||
                    !GetBitBool(m_TrapExceptionBuffer, ExceptionTypes::InstructionAccessFault)
                )
            );

            if(BIT_TO_BOOL(m_CSRWriteEnable))
            {
                if(m_CSRAddress == CSRMachineStatus)
                {
                    m_CSRMachineStatusIRQEnable = GetBit(CSRWriteData(), 3);
                    m_CSRMachineStatusIRQEnablePrev = GetBit(CSRWriteData(), 7);
                    if constexpr(EnableISA_U)
                    {
                        m_CSRMachineStatusPrivilegeModePrev = GetBit(CSRWriteData(), 11) | GetBit(CSRWriteData(), 12);
                        m_CSRMachineStatusEffectivePrivilegeMode = GetBit(CSRWriteData(), 17);
                        m_CSRMachineStatusTW = GetBit(CSRWriteData(), 21);
                    }
                }
                else if(m_CSRAddress == CSRMachineInterruptEnable)
                {
                    m_CSRMachineSoftwareInterruptEnable = GetBit(CSRWriteData(), 3);
                    m_CSRMachineExternalInterruptEnable = GetBit(CSRWriteData(), 11);
                    m_CSRMachineTimerInterruptEnable = GetBit(CSRWriteData(), 7);
                    m_CSRFastIRQEnable = (CSRWriteData() & 0xFFFF'0000) >> 16;
                }
                else if(m_CSRAddress == CSRMachineTrapHandlerAddress)
                {
                    m_CSRMachineTrapHandlerAddress = CSRWriteData() & 0xFFFF'FFFD;
                }
                else if(m_CSRAddress == CSRMachineCounterEnable)
                {
                    if constexpr(EnableISA_U && EnableISA_Zicntr)
                    {
                        m_CSRMachineCycleCounterEnable = GetBit(CSRWriteData(), 0);
                        m_CSRMachineInstructionCounterEnable = GetBit(CSRWriteData(), 2);
                    }
                }
                else if(m_CSRAddress == CSRMachineScratch)
                {
                    m_CSRMachineScratchRegister = CSRWriteData();
                }
                else if(m_CSRAddress == CSRMachineExceptionPC)
                {
                    m_CSRMachineExceptionPC = CSRWriteData() & 0xFFFF'FFFC;
                }
                else if(m_CSRAddress == CSRMachineCounterInhibit)
                {
                    if constexpr(EnableISA_Zicntr)
                    {
                        SetBit(m_CSRMachineInhibitCounterIncrement, 0, GetBitBool(CSRWriteData(), 0));
                        SetBit(m_CSRMachineInhibitCounterIncrement, 2, GetBitBool(CSRWriteData(), 2));
                    }
                    if constexpr(EnableISA_Zihpm)
                    {
                        m_CSRMachineInhibitCounterIncrement &= 0xFFF8;
                        m_CSRMachineInhibitCounterIncrement |= CSRWriteData() & 0xFFF8;
                    }
                }
                else if(m_CSRAddress == CSRDebugCSR)
                {
                    if constexpr(EnableISA_Sdext)
                    {
                        m_CSRDebugEBreakBehaviourMMode = GetBit(CSRWriteData(), 15);
                        m_CSRDebugStep = GetBit(CSRWriteData(), 2);

                        if constexpr(EnableISA_U)
                        {
                            m_CSRDebugEBreakBehaviourUMode = GetBit(CSRWriteData(), 12);
                            m_CSRDebugPrivilegeMode = GetBit(CSRWriteData(), 1) | GetBit(CSRWriteData(), 0);
                        }
                    }
                }
                else if(m_CSRAddress == CSRDebugPC)
                {
                    if constexpr(EnableISA_Sdext)
                    {
                        m_CSRDebugPC = CSRWriteData() & 0xFFFF'FFFC;
                    }
                }
                else if(m_CSRAddress == CSRDebugScratch0)
                {
                    if constexpr(EnableISA_Sdext)
                    {
                        m_CSRDebugScratchRegister0 = CSRWriteData();
                    }
                }
                else if(m_CSRAddress == CSRTriggerData1)
                {
                    if constexpr(EnableISA_Sdtrig)
                    {
                        if(BIT_TO_BOOL(m_DebugRun))
                        {
                            m_CSRMatchTriggerEnable = GetBit(CSRWriteData(), 2);
                        }
                    }
                }
                else if(m_CSRAddress == CSRTriggerData2)
                {
                    if constexpr(EnableISA_Sdtrig)
                    {
                        if(BIT_TO_BOOL(m_DebugRun))
                        {
                            m_CSRTriggerAddressMatchRegister = CSRWriteData() & 0xFFFF'FFFE;
                        }
                    }
                }
            }
            else if(BIT_TO_BOOL(m_TrapEnvironmentEnter))
            {
                if(!EnableISA_Sdext || (!GetBitBool(m_TrapCause, 5) && !BIT_TO_BOOL(m_DebugRun)))
                {
                    m_CSRDebugCause = (GetBit(m_TrapCause, 6) << 6) | (m_TrapCause & 0x1F);
                    m_CSRMachineExceptionPC = TrapExceptionProgramCounter() & 0xFFFF'FFFE;

                    if(!GetBitBool(m_TrapCause, 6) && GetBitBool(m_TrapCause, 2)) // Load/Store Misaligned/Access Faults
                    {
                        m_CSRMachineBadAddress = p_LSUMemoryAddressRegister; // Faulting data access address
                    }
                    else
                    {
                        m_CSRMachineBadAddress = 0;
                    }

                    // Exception
                    if(!GetBitBool(m_TrapCause, 6))
                    {
                        m_CSRMachineTrapInstruction = m_ExecutionEngine.InstructionWord;
                    }
                    else // Interrupt
                    {
                        m_CSRMachineTrapInstruction = 0;
                    }

                    m_CSRPrivilegeLevel = PrivilegeModeMachine;
                    m_CSRMachineStatusIRQEnable = 0;
                    m_CSRMachineStatusIRQEnablePrev = 0;
                }
            }
        }
    }

    PROCESS_DECL(DebugControl)
    {
        if constexpr(EnableISA_Sdext)
        {
            if(!BIT_TO_BOOL(p_Reset_n))
            {
                m_DebugRun = 0;
            }
            else if(RISING_EDGE(p_Clock))
            {
                // Debug mode not enabled
                if(!BIT_TO_BOOL(m_DebugRun))
                {
                    // Waiting for entry event
                    if(BIT_TO_BOOL(m_TrapEnvironmentEnter) && GetBitBool(m_TrapCause, 5))
                    {
                        m_DebugRun = 1;
                    }
                }
                else
                {
                    if(BIT_TO_BOOL(m_TrapEnvironmentExit))
                    {
                        m_DebugRun = 0;
                    }
                }
            }
        }
    }

    PROCESS_DECL(HardwareTriggerException)
    {
        if constexpr(EnableISA_Sdtrig)
        {
            if(!BIT_TO_BOOL(p_Reset_n))
            {
                m_HardwareTriggerFired = 0;
            }
            else if(RISING_EDGE(p_Clock))
            {
                if(!BIT_TO_BOOL(m_HardwareTriggerFired))
                {
                    m_HardwareTriggerFired = HardwareTriggerMatch() & GetBit(m_TrapExceptionBuffer, ExceptionTypes::DebugBreakpoint);
                }
                else if(BIT_TO_BOOL(m_CSRWriteEnable) && m_CSRAddress == CSRTriggerData1 && !GetBitBool(CSRWriteData(), 22))
                {
                    m_HardwareTriggerFired = 0;
                }
            }
        }
    }
private:
    Receiver* m_Parent;
    u32 m_Index;

    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    u32 p_PMPFault : 1;
    u32 p_ALUCoProcessorDone : 1;
    u32 p_ALUCompareStatus : 2;
    u32 p_IRQDebug : 1;
    u32 p_IRQMachine : 3;
    u32 p_IRQFast : 16;
    u32 p_LSUWait : 1;
    u32 p_LSUError : 4;

    InstructionFetchBus p_InstructionFetchBus;
    WordType p_ALUAddressResult;
    WordType p_RS1;
    WordType p_XCSRReadData;
    WordType p_LSUMemoryAddressRegister;

    WordType m_Immediate;
    WordType m_CSRReadData;
    WordType m_CSRMachineExceptionPC;
    WordType m_CSRMachineTrapHandlerAddress;
    WordType m_CSRMachineBadAddress;
    WordType m_CSRMachineTrapInstruction;
    WordType m_CSRMachineScratchRegister;
    WordType m_CSRDebugPC;
    WordType m_CSRDebugScratchRegister0;
    WordType m_CSRTriggerAddressMatchRegister;

    ExecutionEngine m_ExecutionEngine;
    ExecutionEngine m_ExecutionEngineNext;

    u32 m_CSRAddress : 12;
    u32 m_CSRWriteEnable : 1;
    u32 m_CSRWriteEnableNext : 1;
    u32 m_CSRReadEnable : 1;
    u32 m_CSRReadEnableNext : 1;

    u32 m_CSRMachineStatusIRQEnable : 1;
    u32 m_CSRMachineStatusIRQEnablePrev : 1;
    u32 m_CSRMachineStatusPrivilegeModePrev : 1;
    u32 m_CSRMachineStatusEffectivePrivilegeMode : 1;
    u32 m_CSRMachineStatusTW : 1;

    u32 m_CSRMachineSoftwareInterruptEnable : 1;
    u32 m_CSRMachineExternalInterruptEnable : 1;
    u32 m_CSRMachineTimerInterruptEnable : 1;
    u32 m_CSRFastIRQEnable : 16;

    u32 m_CSRPrivilegeLevel : 1;

    u32 m_CSRMachineTrapCause : 6;
    u32 m_CSRMachineCycleCounterEnable : 1;
    u32 m_CSRMachineInstructionCounterEnable : 1;
    u32 m_CSRMachineInhibitCounterIncrement : 16;

    u32 m_CSRDebugEBreakBehaviourMMode : 1;
    u32 m_CSRDebugEBreakBehaviourUMode : 1;
    u32 m_CSRDebugStep : 1;
    u32 m_CSRDebugPrivilegeMode : 1;
    u32 m_CSRDebugCause : 3;

    u32 m_CSRMatchTriggerEnable : 1;

    u32 m_InstructionFetchAcknowledge : 1;
    u32 m_InstructionFetchReset : 1;
    u32 m_BranchTaken : 1;
    u32 m_MonitorCounter : MonitorMultiCycleTimeOut + 1;
    u32 m_EventCounter : 12;
    u32 m_HardwareTriggerFired : 1;
    u32 m_HardwareTriggerStart : 1;

    u32 m_TrapExceptionBuffer : static_cast<u32>(ExceptionTypes::MAX_VALUE);
    u32 m_TrapIRQPending : static_cast<u32>(InterruptSources::MAX_VALUE);
    u32 m_TrapIRQBuffer : static_cast<u32>(InterruptSources::MAX_VALUE);
    u32 m_TrapCause : 7;
    u32 m_TrapEnvironmentPending : 1;
    u32 m_TrapEnvironmentEnter : 1;
    u32 m_TrapEnvironmentEntered : 1;
    u32 m_TrapEnvironmentExit : 1;
    u32 m_TrapInstructionFetchBusError : 1;
    u32 m_TrapInstructionAlignment : 1;
    u32 m_TrapECall : 1;
    u32 m_TrapEBreak : 1;

    u32 m_DebugRun : 1;

    ControlBus m_ControlBus;
    ControlBus m_ControlBusNext;
};

}
