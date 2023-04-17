#pragma once

#include <Objects.hpp>
#include <NumTypes.hpp>

class ICore;

class CoreRegisterManager final
{
    DEFAULT_DESTRUCT(CoreRegisterManager);
    DELETE_CM(CoreRegisterManager);
public:
    CoreRegisterManager(ICore* const core) noexcept
        : m_Core(core)
        , m_RegisterReadReady(false)
        , m_RegisterReadLockReleaseReady(false)
        , m_RegisterWriteReady(false)
        , m_RegisterWriteLockReleaseReady(false)
        , m_Read64Bit{ }
        , m_ReadLock64Bit{ }
        , m_Write64Bit{ }
        , m_WriteLock64Bit{ }
        , m_RegisterReadEnabledCount{ }
        , m_RegisterReadLockEnabledCount{ }
        , m_Pad0{ }
        , m_RegisterReadA{ }
        , m_RegisterReadB{ }
        , m_RegisterReadC{ }
        , m_RegisterReadLockA{ }
        , m_RegisterReadLockB{ }
        , m_RegisterReadLockC{ }
        , m_RegisterWrite{ }
        , m_RegisterWriteValue{ }
        , m_RegisterWriteLock{ }
    { }

    void Reset()
    {
        m_RegisterReadReady = false;
        m_RegisterReadLockReleaseReady = false;
        m_RegisterWriteReady = false;
        m_RegisterWriteLockReleaseReady = false;
        m_Read64Bit = { };
        m_ReadLock64Bit = { };
        m_Write64Bit = { };
        m_WriteLock64Bit = { };
        m_RegisterReadEnabledCount = { };
        m_RegisterReadLockEnabledCount = { };
        m_Pad0 = { };
        m_RegisterReadA = { };
        m_RegisterReadB = { };
        m_RegisterReadC = { };
        m_RegisterReadLockA = { };
        m_RegisterReadLockB = { };
        m_RegisterReadLockC = { };
        m_RegisterWrite = { };
        m_RegisterWriteValue = { };
        m_RegisterWriteLock = { };
    }

    void Clock() noexcept
    {
            // TODO: FIX
        // ReadRegisterA();
    }

    void Clock(u32 clockIndex) noexcept;

    void InitiateRegisterRead(bool is64Bit, u8 registerCount, u32 registerA, u32 registerB, u32 registerC) noexcept;
    void InitiateRegisterWrite(bool is64Bit, u32 storageRegister, u64 value) noexcept;
private:
    void RegisterRead() noexcept;
    void ReadLockRelease() noexcept;
    void RegisterWrite() noexcept;
    void WriteLockRelease() noexcept;
private:
    ICore* m_Core;

    // Whether to perform the read.
    u8 m_RegisterReadReady : 1;
    // Whether to perform the read lock release.
    u8 m_RegisterReadLockReleaseReady : 1;
    // Whether to perform the write.
    u8 m_RegisterWriteReady : 1;
    // Whether to perform the write lock release.
    u8 m_RegisterWriteLockReleaseReady : 1;
    // Whether to read 2 registers at a time.
    u8 m_Read64Bit : 1;
    // Whether to release the read lock on 2 registers at a time.
    u8 m_ReadLock64Bit : 1;
    // Whether to write 2 registers at a time.
    u8 m_Write64Bit : 1;
    // Whether to release the write lock on 2 registers at a time.
    u8 m_WriteLock64Bit : 1;

    // How many base registers are being read, can either be 1, 2, 3 using 1 indexing.
    u8 m_RegisterReadEnabledCount : 2;
    // How many base registers are having their read lock released, can either be 1, 2, 3 using 1 indexing.
    u8 m_RegisterReadLockEnabledCount : 2;
    // Padding for x86.
    u8 m_Pad0 : 4;

    // The A register to read.
    u16 m_RegisterReadA;
    // The B register to read.
    u16 m_RegisterReadB;
    // The C register to read.
    u16 m_RegisterReadC;

    // The A register to release the read lock on.
    u16 m_RegisterReadLockA;
    // The B register to release the read lock on.
    u16 m_RegisterReadLockB;
    // The C register to release the read lock on.
    u16 m_RegisterReadLockC;

    // The register to write to.
    u32 m_RegisterWrite;
    // The value to write to the register.
    u64 m_RegisterWriteValue;

    // The register to release the write lock on.
    u32 m_RegisterWriteLock;
};
