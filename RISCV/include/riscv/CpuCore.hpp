/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <Common.hpp>
#include "ControlBus.hpp"
#include "ClockGate.hpp"
#include "InstructionFetch.hpp"
#include "Controller.hpp"
#include "ArithmeticLogicUnit.hpp"
#include "RegisterFile.hpp"
#include "LoadStoreUnit.hpp"
#include "PhysicalMemoryProtectionUnit.hpp"

namespace riscv {

class CPUCoreReceiverSample
{
public:
    void ReceiveCPUCore_MemoryRequest([[maybe_unused]] const u32 index, [[maybe_unused]] const MemoryBusRequest& bus) noexcept { }
};

// template<typename Receiver = CPUCoreReceiverSample>
class CPUCore final
{
    DEFAULT_DESTRUCT(CPUCore);
    DELETE_CM(CPUCore);
public:
    // Just temporary for better IntelliSense.
    using Receiver = CPUCoreReceiverSample;

    constexpr static inline u32 BootAddress = 0;
    constexpr static inline u32 DebugParkAddress = 0;
    constexpr static inline u32 DebugExceptionAddress = 0;
    constexpr static inline bool EnableInterCoreComm = false;

    constexpr static inline bool EnableISA_C      = false; // Compressed
    constexpr static inline bool EnableISA_E      = false; // Embedded RF
    constexpr static inline bool EnableISA_M      = false; // Mul/Div
    constexpr static inline bool EnableISA_U      = false; // User Mode
    constexpr static inline bool EnableISA_Zaamo  = false; // Atomic read-modify-write operations
    constexpr static inline bool EnableISA_Zalrsc = false; // Atomic reservation-set operations
    constexpr static inline bool EnableISA_Zba    = false; // Shifted-add bit manipulation
    constexpr static inline bool EnableISA_Zbb    = false; // Basic bit manipulation
    constexpr static inline bool EnableISA_Zbkb   = false; // Bit manipulation for cryptography
    constexpr static inline bool EnableISA_Zbkc   = false; // Carry-less multiplication
    constexpr static inline bool EnableISA_Zbkx   = false; // Cryptography crossbar permutation
    constexpr static inline bool EnableISA_Zbs    = false; // Single bit manipulation
    constexpr static inline bool EnableISA_Zfinx  = false; // 32-Bit floating point
    constexpr static inline bool EnableISA_Zicntr = false; // Base counters
    constexpr static inline bool EnableISA_Zicond = false; // Integer conditional operations
    constexpr static inline bool EnableISA_Zihpm  = false; // Hardware performance monitors
    constexpr static inline bool EnableISA_Zkne   = false; // AES encryption
    constexpr static inline bool EnableISA_Zknd   = false; // AES decryption
    constexpr static inline bool EnableISA_Zknh   = false; // Hashing
    constexpr static inline bool EnableISA_Zmmul  = false; // Multiply only M sub-extension
    constexpr static inline bool EnableISA_Zxcfu  = false; // Custom functions unit
    constexpr static inline bool EnableISA_Sdext  = false; // External debug mode
    constexpr static inline bool EnableISA_Smpmp  = false; // Physical memory protection

    constexpr static inline bool EnableClockGating = false;

    constexpr static inline bool EnableRS3 = EnableISA_Zxcfu || EnableISA_Zfinx; // 3rd register file read port.
public:
    CPUCore(Receiver* const parent, const u32 index, const u32 hartID) noexcept
        : m_Parent(parent)
        , m_Index(index)
        , m_HartID(hartID)
        , p_Reset_n(0)
        , p_Clock(0)
        , p_MachineSoftwareInterrupt(0)
        , p_MachineExternalInterrupt(0)
        , p_MachineTimerInterrupt(0)
        , p_DebugHaltInterrupt(0)
        , p_CustomFastInterrupt(0)
        , p_MemSync(0)
        , m_ClockGated(0)
        , m_Pad0(0)
        , m_CSR_ALU(0)
        , m_CSR_PMP(0)
        , m_ALUResult(0)
        , m_LSUReadData(0)
        , m_CSRReadData(0)
        , m_ControlBus{ }
        , m_ClockGate(this, 0)
        , m_InstructionFetch(this, 0)
        , m_Controller(nullptr, 0)
        , m_RegisterFile(this, 0)
        , m_ArithmeticLogicUnit(this, 0)
        , m_LoadStoreUnit(this, 0)
        , m_PhysicalMemoryProtectionUnit(this, 0)
    { }

    void SetResetN(const bool reset_n) noexcept
    {
        p_Reset_n = BOOL_TO_BIT(reset_n);

        m_ClockGate.SetResetN(reset_n);
        m_InstructionFetch.SetResetN(reset_n);
        m_Controller.SetResetN(reset_n);
        m_ArithmeticLogicUnit.SetResetN(reset_n);
        m_RegisterFile.SetResetN(reset_n);
        m_LoadStoreUnit.SetResetN(reset_n);
        m_PhysicalMemoryProtectionUnit.SetResetN(reset_n);
    }

    void SetClock(const bool clock) noexcept
    {
        p_Clock = BOOL_TO_BIT(clock);

        m_ClockGate.SetClock(clock);

        m_InstructionFetch.SetClock(ClockGated());
        m_Controller.SetClock(ClockGated());
        m_ArithmeticLogicUnit.SetClock(ClockGated());
        m_RegisterFile.SetClock(ClockGated());
        m_LoadStoreUnit.SetClock(ClockGated());
        m_PhysicalMemoryProtectionUnit.SetClock(ClockGated());
    }

    void SetMemoryResponse(const MemoryBusResponse& response) noexcept
    {
        m_InstructionFetch.SetMemoryResponse(response);
        m_LoadStoreUnit.SetMemoryResponse(response);
    }
public:
    void ReceiveClockGate_Clock(const u32 index, const bool clock) noexcept
    {
        (void) index;
        m_ClockGated = BOOL_TO_BIT(clock);
    }

    void ReceiveInstructionFetch_Bus(const u32 index, const InstructionFetchBus& bus) noexcept
    {
        (void) index;
        m_Controller.SetInstructionFetchBus(bus);
    }

    void ReceiveInstructionFetch_MemoryRequest(const u32 index, const MemoryBusRequest& bus) noexcept
    {
        (void) index;
        m_Parent->ReceiveCPUCore_MemoryRequest(m_Index, bus);
    }

    void ReceiveController_ControlBus([[maybe_unused]] const u32 index, [[maybe_unused]] const ControlBus& controlBus) noexcept
    {
        (void) index;
        m_ControlBus = controlBus;

        m_InstructionFetch.SetControlBus(controlBus);
        m_RegisterFile.SetControlBus(controlBus);
        m_ArithmeticLogicUnit.SetControlBus(controlBus);
        m_LoadStoreUnit.SetControlBus(controlBus);
        m_PhysicalMemoryProtectionUnit.SetControlBus(controlBus);
    }

    void ReceiveController_CSRReadData([[maybe_unused]] const u32 index, [[maybe_unused]] const u32 data) noexcept
    {
        (void) index;
        m_CSRReadData = data;
        UpdateRegisterFileWriteData();
    }

    void ReceiveRegisterFile_RS1(const u32 index, const u32 rs1) noexcept
    {
        (void) index;
        m_Controller.SetRS1(rs1);
        m_ArithmeticLogicUnit.SetRS1(rs1);
    }

    void ReceiveRegisterFile_RS2(const u32 index, const u32 rs2) noexcept
    {
        (void) index;
        m_ArithmeticLogicUnit.SetRS1(rs2);
        m_LoadStoreUnit.SetAddress(rs2);
    }

    void ReceiveRegisterFile_RS3(const u32 index, const u32 rs3) noexcept
    {
        (void) index;
        m_ArithmeticLogicUnit.SetRS1(rs3);
    }

    void ReceiveArithmeticLogicUnit_ComparatorStatus([[maybe_unused]] const u32 index, [[maybe_unused]] const u8 status) noexcept
    {
        (void) index;
        m_Controller.SetALUCompareStatus(status);
    }

    void ReceiveArithmeticLogicUnit_Result([[maybe_unused]] const u32 index, [[maybe_unused]] const u32 result) noexcept
    {
        (void) index;
        m_ALUResult = result;
        UpdateRegisterFileWriteData();
    }

    void ReceiveArithmeticLogicUnit_Address([[maybe_unused]] const u32 index, [[maybe_unused]] const u32 address) noexcept
    {
        (void) index;
        m_Controller.SetALUAddressResult(address);
        m_LoadStoreUnit.SetAddress(address);
        m_PhysicalMemoryProtectionUnit.SetAddress(address);
    }

    void ReceiveArithmeticLogicUnit_CSR([[maybe_unused]] const u32 index, [[maybe_unused]] const u32 csr) noexcept
    {
        (void) index;
        m_CSR_ALU = csr;
        UpdateExternalCSRResult();
    }

    void ReceiveArithmeticLogicUnit_Done([[maybe_unused]] const u32 index, [[maybe_unused]] const bool done) noexcept
    {
        (void) index;
        m_Controller.SetALUCoProcessorDone(done);
    }

    void ReceiveLoadStoreUnit_ReadData([[maybe_unused]] const u32 index, const u32 readData) noexcept
    {
        (void) index;
        m_LSUReadData = readData;
        UpdateRegisterFileWriteData();
    }

    void ReceiveLoadStoreUnit_MemoryAddress([[maybe_unused]] const u32 index, [[maybe_unused]] const u32 memoryAddress) noexcept
    {
        (void) index;
        m_Controller.SetLSUMemoryAddressRegister(memoryAddress);
    }

    void ReceiveLoadStoreUnit_Wait([[maybe_unused]] const u32 index, [[maybe_unused]] const bool wait) noexcept
    {
        (void) index;
        m_Controller.SetLSUWait(wait);
    }

    void ReceiveLoadStoreUnit_Error([[maybe_unused]] const u32 index, [[maybe_unused]] const u32 error) noexcept
    {
        (void) index;
        m_Controller.SetLSUError(static_cast<u8>(error));
    }

    void ReceiveLoadStoreUnit_MemoryRequest([[maybe_unused]] const u32 index, [[maybe_unused]] const MemoryBusRequest& memoryRequest) noexcept
    {
        (void) index;
        m_Parent->ReceiveCPUCore_MemoryRequest(m_Index, memoryRequest);
    }

    void ReceivePhysicalMemoryProtectionUnit_CSR([[maybe_unused]] const u32 index, [[maybe_unused]] const u32 csr) noexcept
    {
        (void) index;
        m_CSR_PMP = csr;
        UpdateExternalCSRResult();
    }

    void ReceivePhysicalMemoryProtectionUnit_Fault([[maybe_unused]] const u32 index, [[maybe_unused]] const bool fault) noexcept
    {
        (void) index;
        m_Controller.SetPMPFault(fault);
        m_LoadStoreUnit.SetPMPFault(fault);
    }
private:
    // Muxes

    [[nodiscard]] bool ClockGated() const noexcept
    {
        if constexpr(EnableClockGating)
        {
            return m_ClockGated;
        }
        else
        {
            return p_Clock;
        }
    }

    [[nodiscard]] u32 ExternalCSRResult() const noexcept
    {
        return m_CSR_ALU | m_CSR_PMP;
    }

    void UpdateExternalCSRResult() noexcept
    {
        m_Controller.SetXCSRReadData(ExternalCSRResult());
    }

    [[nodiscard]] u32 RegisterFileWriteData() const noexcept
    {
        return m_ALUResult | m_LSUReadData | m_CSRReadData | m_ControlBus.PC_Return;
    }

    void UpdateRegisterFileWriteData() noexcept
    {
        m_RegisterFile.SetRD(RegisterFileWriteData());
    }
private:
    Receiver* m_Parent;
    u32 m_Index;
    u32 m_HartID;

    u32 p_Reset_n : 1;
    u32 p_Clock : 1;

    u32 p_MachineSoftwareInterrupt : 1;
    u32 p_MachineExternalInterrupt : 1;
    u32 p_MachineTimerInterrupt : 1;
    u32 p_DebugHaltInterrupt : 1;
    u32 p_CustomFastInterrupt : 16;

    u32 p_MemSync : 1;

    u32 m_ClockGated : 1;
    [[maybe_unused]] u32 m_Pad0 : 8;

    u32 m_CSR_ALU;
    u32 m_CSR_PMP;

    u32 m_ALUResult;
    u32 m_LSUReadData;
    u32 m_CSRReadData;

    ControlBus m_ControlBus;
    ClockGate<CPUCore> m_ClockGate;
    InstructionFetch<CPUCore> m_InstructionFetch;
    Controller m_Controller;
    RegisterFile<CPUCore, EnableISA_E, EnableRS3> m_RegisterFile;
    ArithmeticLogicUnit<CPUCore> m_ArithmeticLogicUnit;
    LoadStoreUnit<CPUCore> m_LoadStoreUnit;
    PhysicalMemoryProtectionUnit<CPUCore> m_PhysicalMemoryProtectionUnit;
};

}
