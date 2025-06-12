#pragma once

#include <Common.hpp>
#include "ControlBus.hpp"
#include "ClockGate.hpp"
#include "RegisterFile.hpp"

namespace riscv {

class CPUCore final
{
    DEFAULT_DESTRUCT(CPUCore);
    DELETE_CM(CPUCore);
public:
    constexpr static inline u32 BootAddress = 0;
    constexpr static inline u32 DebugParkAddress = 0;
    constexpr static inline u32 DebugExceptionAddress = 0;
    constexpr static inline bool EnableInterCoreComm = false;

    constexpr static inline bool EnableISA_C = false; // Compressed
    constexpr static inline bool EnableISA_E = false; // Embedded RF
    constexpr static inline bool EnableISA_M = false; // Mul/Div
    constexpr static inline bool EnableISA_U = false; // User Mode
    constexpr static inline bool EnableISA_Zaamo = false; // Atomic read-modify-write operations
    constexpr static inline bool EnableISA_Zalrsc = false; // Atomic reservation-set operations
    constexpr static inline bool EnableISA_Zba = false; // Shifted-add bit manipulation
    constexpr static inline bool EnableISA_Zbb = false; // Basic bit manipulation
    constexpr static inline bool EnableISA_Zbkb = false; // Bit manipulation for cryptography
    constexpr static inline bool EnableISA_Zbkc = false; // Carry-less multiplication
    constexpr static inline bool EnableISA_Zbkx = false; // Cryptography crossbar permutation
    constexpr static inline bool EnableISA_Zbs = false; // Single bit manipulation
    constexpr static inline bool EnableISA_Zfinx = false; // 32-Bit floating point
    constexpr static inline bool EnableISA_Zicntr = false; // Base counters
    constexpr static inline bool EnableISA_Zicond = false; // Integer conditional operations
    constexpr static inline bool EnableISA_Zihpm = false; // Hardware performance monitors
    constexpr static inline bool EnableISA_Zkne = false; // AES encryption
    constexpr static inline bool EnableISA_Zknd = false; // AES decryption
    constexpr static inline bool EnableISA_Zknh = false; // Hashing
    constexpr static inline bool EnableISA_Zmmul = false; // Multiply only M sub-extension
    constexpr static inline bool EnableISA_Zxcfu = false; // Custom functions unit
    constexpr static inline bool EnableISA_Sdext = false; // External debug mode
    constexpr static inline bool EnableISA_Smpmp = false; // Physical memory protection

    constexpr static inline bool EnableClockGating = false;

    constexpr static inline bool EnableRS3 = EnableISA_Zxcfu || EnableISA_Zfinx; // 3rd register file read port.
public:
    CPUCore(const u32 hartID) noexcept
        : m_HartID(hartID)
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
        , m_ControlBus{ }
        , m_ClockGate(this)
        , m_RegisterFile(this)
    { }

    void SetResetN(const bool reset_n) noexcept
    {
        p_Reset_n = BOOL_TO_BIT(reset_n);

        m_ClockGate.SetResetN(reset_n);
        m_RegisterFile.SetResetN(reset_n);
    }

    void SetClock(const bool clock) noexcept
    {
        p_Clock = BOOL_TO_BIT(clock);

        m_ClockGate.SetClock(clock);

        m_RegisterFile.SetClock(ClockGated());
    }
private:
    void ReceiveClockGate_Clock(const u32 index, const bool clock) noexcept
    {
        (void) index;
        m_ClockGated = BOOL_TO_BIT(clock);
    }

    void ReceiveRegisterFile_RS1(const u32 index, const u32 rs1) noexcept
    {
        
    }

    void ReceiveRegisterFile_RS2(const u32 index, const u32 rs2) noexcept
    {
        
    }

    void ReceiveRegisterFile_RS3(const u32 index, const u32 rs3) noexcept
    {
        
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
private:
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
    u32 m_Pad0 : 8;


    ControlBus m_ControlBus;
    ClockGate<CPUCore> m_ClockGate;
    RegisterFile<CPUCore, EnableISA_E, EnableRS3> m_RegisterFile;
    
};

}
