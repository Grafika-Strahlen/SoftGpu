/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

namespace riscv {

/**
 * log2 of the maximum number of execution cycles before timing out multi-clock-cycle operations.
 */
constexpr static inline u32 MonitorMultiCycleTimeOut = 9;

constexpr static inline u32 InstructionOpcodeLSB = 0;
constexpr static inline u32 InstructionOpcodeMSB = 6;
constexpr static inline u32 InstructionDestinationRegisterLSB = 7;
constexpr static inline u32 InstructionDestinationRegisterMSB = 11;
constexpr static inline u32 InstructionFunction3LSB = 12;
constexpr static inline u32 InstructionFunction3MSB = 14;
constexpr static inline u32 InstructionRS1LSB = 15;
constexpr static inline u32 InstructionRS1MSB = 19;
constexpr static inline u32 InstructionRS2LSB = 20;
constexpr static inline u32 InstructionRS2MSB = 24;
constexpr static inline u32 InstructionFunction7LSB = 25;
constexpr static inline u32 InstructionFunction7MSB = 31;
constexpr static inline u32 InstructionFunction12LSB = 20;
constexpr static inline u32 InstructionFunction12MSB = 31;
constexpr static inline u32 InstructionImmediate12LSB = 20;
constexpr static inline u32 InstructionImmediate12MSB = 31;
constexpr static inline u32 InstructionImmediate20LSB = 12;
constexpr static inline u32 InstructionImmediate20MSB = 31;
constexpr static inline u32 InstructionFunction5LSB = 27;
constexpr static inline u32 InstructionFunction5MSB = 31;

// ALU
constexpr static inline u32 OpCodeALUImmediate            = 0b0010011;
constexpr static inline u32 OpCodeALU                     = 0b0110011;
constexpr static inline u32 OpCodeLoadUpperImmediate      = 0b0110111;
constexpr static inline u32 OpCodeAddUpperImmediate       = 0b0010111;
// Control Flow
constexpr static inline u32 OpCodeJumpAndLink             = 0b1101111;
constexpr static inline u32 OpCodeJumpAndLinkWithRegister = 0b1100111;
constexpr static inline u32 OpCodeBranch                  = 0b1100011;
// Memory Access
constexpr static inline u32 OpCodeLoad                    = 0b0000011;
constexpr static inline u32 OpCodeStore                   = 0b0100011;
constexpr static inline u32 OpCodeAtomic                  = 0b0101111;
constexpr static inline u32 OpCodeFence                   = 0b0001111;
// System / CSR
constexpr static inline u32 OpCodeSystem                  = 0b1110011;
// Floating Point
constexpr static inline u32 OpCodeFloatingPoint           = 0b1010011;
// Custom (part of the RISC-V spec)
constexpr static inline u32 OpCodeCustom0                 = 0b0001011;
constexpr static inline u32 OpCodeCustom1                 = 0b0101011;
constexpr static inline u32 OpCodeCustom2                 = 0b1011011;
constexpr static inline u32 OpCodeCustom3                 = 0b1111011;

// Control Flow
constexpr static inline u32 Function3BranchIfEqual                      = 0b000;
constexpr static inline u32 Function3BranchIfNotEqual                   = 0b001;
constexpr static inline u32 Function3BranchIfLessThan                   = 0b100;
constexpr static inline u32 Function3BranchIfGreaterThanOrEqual         = 0b101;
constexpr static inline u32 Function3BranchIfLessThanUnsigned           = 0b110;
constexpr static inline u32 Function3BranchIfGreaterThanOrEqualUnsigned = 0b111;
// Memory Access
constexpr static inline u32 Function3LoadByteSigned                     = 0b000;
constexpr static inline u32 Function3LoadHalfWordSigned                 = 0b001;
constexpr static inline u32 Function3LoadWordSigned                     = 0b010;
constexpr static inline u32 Function3LoadByteUnsigned                   = 0b100;
constexpr static inline u32 Function3LoadHalfWordUnsigned               = 0b101;
constexpr static inline u32 Function3LoadWordUnsigned                   = 0b110;
constexpr static inline u32 Function3StoreByte                          = 0b000;
constexpr static inline u32 Function3StoreHalfWord                      = 0b001;
constexpr static inline u32 Function3StoreWord                          = 0b010;
// ALU
constexpr static inline u32 Function3SubtractAdd                        = 0b000;
constexpr static inline u32 Function3ShiftLogicalLeft                   = 0b001;
constexpr static inline u32 Function3SetOnLess                          = 0b010;
constexpr static inline u32 Function3SetOnLessUnsigned                  = 0b011;
constexpr static inline u32 Function3XOR                                = 0b100;
constexpr static inline u32 Function3ShiftRight                         = 0b101;
constexpr static inline u32 Function3OR                                 = 0b110;
constexpr static inline u32 Function3AND                                = 0b111;
// System / CSR
constexpr static inline u32 Function3Environment                        = 0b000;
constexpr static inline u32 Function3CSRReadWrite                       = 0b001;
constexpr static inline u32 Function3CSRReadAndSet                      = 0b010;
constexpr static inline u32 Function3CSRReadAndClear                    = 0b011;
constexpr static inline u32 Function3CSRIllegal                         = 0b100;
constexpr static inline u32 Function3CSRReadWriteImmediate              = 0b101;
constexpr static inline u32 Function3CSRReadAndSetImmediate             = 0b110;
constexpr static inline u32 Function3CSRReadAndClearImmediate           = 0b111;
// Fence
constexpr static inline u32 Function3Fence                              = 0b000;
constexpr static inline u32 Function3FenceInstruction                   = 0b001;

// Function 12
constexpr static inline u32 Function12ECall         = 0x000;
constexpr static inline u32 Function12EBreak        = 0x001;
constexpr static inline u32 Function12WFI           = 0x105;
constexpr static inline u32 Function12MachineReturn = 0x302;
constexpr static inline u32 Function12DebugReturn   = 0x7B2;

// FP Registers
constexpr static inline u32 CSRFloatingPointFlags           = 0x001;
constexpr static inline u32 CSRFloatingPointRM              = 0x002;
constexpr static inline u32 CSRFloatingPointRegister        = 0x003;
// Machine Trap Setup Registers
constexpr static inline u32 CSRMachineStatus                = 0x300;
constexpr static inline u32 CSRMachineISA                   = 0x301;
constexpr static inline u32 CSRMachineInterruptEnable       = 0x304;
constexpr static inline u32 CSRMachineTrapHandlerAddress    = 0x305;
constexpr static inline u32 CSRMachineCounterEnable         = 0x306;
constexpr static inline u32 CSRMachineStatusHigh            = 0x310;
// Machine Configuration Registers
constexpr static inline u32 CSRMachineEnvironmentConfig     = 0x30A;
constexpr static inline u32 CSRMachineEnvironmentConfigHigh = 0x31A;
// Machine Counter Setup Registers
constexpr static inline u32 CSRMachineCounterInhibit        = 0x320;
constexpr static inline u32 CSRMachineHPMEvent3             = 0x323;
constexpr static inline u32 CSRMachineHPMEvent4             = 0x324;
constexpr static inline u32 CSRMachineHPMEvent5             = 0x325;
constexpr static inline u32 CSRMachineHPMEvent6             = 0x326;
constexpr static inline u32 CSRMachineHPMEvent7             = 0x327;
constexpr static inline u32 CSRMachineHPMEvent8             = 0x328;
constexpr static inline u32 CSRMachineHPMEvent9             = 0x329;
constexpr static inline u32 CSRMachineHPMEvent10            = 0x32A;
constexpr static inline u32 CSRMachineHPMEvent11            = 0x32B;
constexpr static inline u32 CSRMachineHPMEvent12            = 0x32C;
constexpr static inline u32 CSRMachineHPMEvent13            = 0x32D;
constexpr static inline u32 CSRMachineHPMEvent14            = 0x32E;
constexpr static inline u32 CSRMachineHPMEvent15            = 0x32F;
// Machine Trap Handling Registers
constexpr static inline u32 CSRMachineScratch               = 0x340;
constexpr static inline u32 CSRMachineExceptionPC           = 0x341;
constexpr static inline u32 CSRMachineCause                 = 0x342;
constexpr static inline u32 CSRMachineTriggerValue          = 0x343;
constexpr static inline u32 CSRMachineIP                    = 0x344;
constexpr static inline u32 CSRMachineInstruction           = 0x34A;
// PMP Config Registers
constexpr static inline u32 CSRPMPConfig0                    = 0x3A0;
constexpr static inline u32 CSRPMPConfig1                    = 0x3A1;
constexpr static inline u32 CSRPMPConfig2                    = 0x3A2;
constexpr static inline u32 CSRPMPConfig3                    = 0x3A3;
// PMP Address Registers
constexpr static inline u32 CSRPMPAddress0                  = 0x3B0;
constexpr static inline u32 CSRPMPAddress1                  = 0x3B1;
constexpr static inline u32 CSRPMPAddress2                  = 0x3B2;
constexpr static inline u32 CSRPMPAddress3                  = 0x3B3;
constexpr static inline u32 CSRPMPAddress4                  = 0x3B4;
constexpr static inline u32 CSRPMPAddress5                  = 0x3B5;
constexpr static inline u32 CSRPMPAddress6                  = 0x3B6;
constexpr static inline u32 CSRPMPAddress7                  = 0x3B7;
constexpr static inline u32 CSRPMPAddress8                  = 0x3B8;
constexpr static inline u32 CSRPMPAddress9                  = 0x3B9;
constexpr static inline u32 CSRPMPAddress10                 = 0x3BA;
constexpr static inline u32 CSRPMPAddress11                 = 0x3BB;
constexpr static inline u32 CSRPMPAddress12                 = 0x3BC;
constexpr static inline u32 CSRPMPAddress13                 = 0x3BD;
constexpr static inline u32 CSRPMPAddress14                 = 0x3BE;
constexpr static inline u32 CSRPMPAddress15                 = 0x3BF;
// Trigger Module Registers
constexpr static inline u32 CSRTriggerSelect                = 0x7A0;
constexpr static inline u32 CSRTriggerData1                 = 0x7A1;
constexpr static inline u32 CSRTriggerData2                 = 0x7A2;
constexpr static inline u32 CSRTriggerInfo                  = 0x7A4;
// Debug Registers
constexpr static inline u32 CSRDebugCSR                     = 0x7B0;
constexpr static inline u32 CSRDebugPC                      = 0x7B1;
constexpr static inline u32 CSRDebugScratch0                = 0x7B2;
// Custom Function Unit R/W User Registers
constexpr static inline u32 CSRCFURegister0                 = 0x800;
constexpr static inline u32 CSRCFURegister1                 = 0x801;
constexpr static inline u32 CSRCFURegister2                 = 0x802;
constexpr static inline u32 CSRCFURegister3                 = 0x803;
// Machine Information Registers
constexpr static inline u32 CSRMachineVendorID              = 0xF11;
constexpr static inline u32 CSRMachineArchitectureID        = 0xF12;
constexpr static inline u32 CSRMachineImplementationID      = 0xF13;
constexpr static inline u32 CSRMachineHartID                = 0xF14;
constexpr static inline u32 CSRMachineConfigPointer         = 0xF15;

constexpr static inline u32 ALUComparatorEqual = 0;
constexpr static inline u32 ALUComparatorLessThan = 1;

// MSB: 1 = Interrupt, 0 = Synchronous Exception
// MSB-1: 1 = Entry to Debug Mode, 0 = Normal Trapping
// RISC-V Compliant Synchronous Exceptions
constexpr static inline u32 TrapInstructionMisaligned    = 0b0'0'00000;
constexpr static inline u32 TrapInstructionAccessFault   = 0b0'0'00001;
constexpr static inline u32 TrapIllegalInstruction       = 0b0'0'00010;
constexpr static inline u32 TrapEnvironmentBreakpoint    = 0b0'0'00011;
constexpr static inline u32 TrapLoadAddressMisaligned    = 0b0'0'00100;
constexpr static inline u32 TrapLoadAccessFault          = 0b0'0'00101;
constexpr static inline u32 TrapStoreAddressMisaligned   = 0b0'0'00110;
constexpr static inline u32 TrapStoreAccessFault         = 0b0'0'00111;
constexpr static inline u32 TrapEnvironment              = 0b0'0'01000;
// RISC-V Compliant Asynchronous Exceptions (Interrupts)
constexpr static inline u32 TrapMachineSoftwareInterrupt = 0b1'0'00011;
constexpr static inline u32 TrapMachineTimerInterrupt    = 0b1'0'00111;
constexpr static inline u32 TrapMachineExternalInterrupt = 0b1'0'01011;
// Custom Asynchronous Exceptions (Interrupts)
constexpr static inline u32 TrapFastIRQ0                 = 0b1'0'10000;
constexpr static inline u32 TrapFastIRQ1                 = 0b1'0'10001;
constexpr static inline u32 TrapFastIRQ2                 = 0b1'0'10010;
constexpr static inline u32 TrapFastIRQ3                 = 0b1'0'10011;
constexpr static inline u32 TrapFastIRQ4                 = 0b1'0'10100;
constexpr static inline u32 TrapFastIRQ5                 = 0b1'0'10101;
constexpr static inline u32 TrapFastIRQ6                 = 0b1'0'10110;
constexpr static inline u32 TrapFastIRQ7                 = 0b1'0'10111;
constexpr static inline u32 TrapFastIRQ8                 = 0b1'0'11000;
constexpr static inline u32 TrapFastIRQ9                 = 0b1'0'11001;
constexpr static inline u32 TrapFastIRQ10                = 0b1'0'11010;
constexpr static inline u32 TrapFastIRQ11                = 0b1'0'11011;
constexpr static inline u32 TrapFastIRQ12                = 0b1'0'11100;
constexpr static inline u32 TrapFastIRQ13                = 0b1'0'11101;
constexpr static inline u32 TrapFastIRQ14                = 0b1'0'11110;
constexpr static inline u32 TrapFastIRQ15                = 0b1'0'11111;
// Debug Mode Exceptions
constexpr static inline u32 TrapDebugBreakInstruction    = 0b0'1'00001;
constexpr static inline u32 TrapDebugHardwareBreak       = 0b0'1'00010;
constexpr static inline u32 TrapDebugHalt                = 0b1'1'00011;
constexpr static inline u32 TrapDebugStep                = 0b1'1'00100;

enum class ExceptionTypes
{
    InstructionAccessFault = 0,
    IllegalInstruction,
    InstructionAddressAlignment,
    EnvironmentCall,
    Breakpoint,
    StoreAddressAlignment,
    LoadAddressAlignment,
    StoreAccessFault,
    LoadAccessFault,
    DebugBreakpoint,
    DebugHardwareTrap,
    MAX_VALUE
};

enum class InterruptSources
{
    MachineSoftwareInterrupt = 0,
    MachineTimerInterrupt,
    MachineExternalInterrupt,
    Fast0,
    Fast1,
    Fast2,
    Fast3,
    Fast4,
    Fast5,
    Fast6,
    Fast7,
    Fast8,
    Fast9,
    Fast10,
    Fast11,
    Fast12,
    Fast13,
    Fast14,
    Fast15,
    DebugHalt,
    DebugStep,
    MAX_VALUE
};

constexpr static inline u32 PrivilegeModeMachine = 1;
constexpr static inline u32 PrivilegeModeUser = 0;

}
