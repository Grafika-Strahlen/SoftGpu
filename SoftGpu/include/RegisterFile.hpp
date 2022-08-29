#pragma once

#include <cassert>
#include <Objects.hpp>
#include <NumTypes.hpp>
#include <ConPrinter.hpp>
#include "DebugManager.hpp"

#define REGISTER_FILE_REGISTER_COUNT (4096)

class RegisterFile final
{
    DEFAULT_CONSTRUCT_PU(RegisterFile);
    DEFAULT_DESTRUCT(RegisterFile);
    DELETE_CM(RegisterFile);
public:
    void Reset()
    {
        ::std::memset(m_Registers, 0, sizeof(m_Registers));
        ::std::memset(m_RegisterContestationMap, 0, sizeof(m_RegisterContestationMap));
    }

    [[nodiscard]] u32 GetRegister(const u32 targetRegister) const noexcept
    {
        return m_Registers[targetRegister];
    }

    void SetRegister(const u32 targetRegister, const u32 value) noexcept
    {
        m_Registers[targetRegister] = value;
    }
    
    [[nodiscard]] bool CanReadRegister(const u32 registerIndex) noexcept
    {
        return m_RegisterContestationMap[registerIndex] != 1 && m_RegisterContestationMap[registerIndex] != 0xFF;
    }

    [[nodiscard]] bool CanWriteRegister(const u32 registerIndex) noexcept
    {
        return m_RegisterContestationMap[registerIndex] == 0;
    }

    void ReleaseRegisterContestation(const u32 registerIndex) noexcept
    {
        --m_RegisterContestationMap[registerIndex];

        if(m_RegisterContestationMap[registerIndex] == 0xFF)
        {
            ConPrinter::PrintLn("Register Unlock {} has underflowed.", registerIndex);

            assert(m_RegisterContestationMap[registerIndex] != 0xFF);
        }

        // If that was the last read lock the value will now be 1, which is actually the write lock. Set it to no lock.
        if(m_RegisterContestationMap[registerIndex] == 1)
        {
            m_RegisterContestationMap[registerIndex] = 0;
        }
    }

    void LockRegisterRead(const u32 registerIndex) noexcept
    {
        if(m_RegisterContestationMap[registerIndex] == 0)
        {
            m_RegisterContestationMap[registerIndex] = 2;
        }
        else
        {
            if(m_RegisterContestationMap[registerIndex] == 0xFF)
            {
                ConPrinter::PrintLn("Register Read Lock {} has overflowed.", registerIndex);

                assert(m_RegisterContestationMap[registerIndex] != 0xFF);
            }

            ++m_RegisterContestationMap[registerIndex];
        }
    }

    void LockRegisterWrite(const u32 registerIndex) noexcept
    {
        m_RegisterContestationMap[registerIndex] = 1;
    }

    void ReportRegisters(const u32 smIndex) noexcept
    {
        if(GlobalDebug.IsAttached())
        {
            {
                GlobalDebug.WriteRawInfo(&DebugCodeReportRegisterFile, sizeof(DebugCodeReportRegisterFile));
                constexpr u32 length = sizeof(smIndex) + sizeof(m_Registers);
                GlobalDebug.WriteRawInfo(&length, sizeof(length));
                GlobalDebug.WriteRawInfo(&smIndex, sizeof(smIndex));
                GlobalDebug.WriteRawInfo(m_Registers, sizeof(m_Registers));
            }

            {
                GlobalDebug.WriteRawInfo(&DebugCodeReportRegisterContestion, sizeof(DebugCodeReportRegisterContestion));
                constexpr u32 length = sizeof(smIndex) + sizeof(m_RegisterContestationMap);
                GlobalDebug.WriteRawInfo(&length, sizeof(length));
                GlobalDebug.WriteRawInfo(&smIndex, sizeof(smIndex));
                GlobalDebug.WriteRawInfo(m_RegisterContestationMap, sizeof(m_RegisterContestationMap));
            }
        }
    }
private:
    u32 m_Registers[REGISTER_FILE_REGISTER_COUNT];

    // This contains information about how each register is being used.
    // If the value is zero the register is unused.
    // If the value is one it is locked for writes.
    // Otherwise the register is locked for reads. Any number of simultaneous reads are allowed.
    u8 m_RegisterContestationMap[REGISTER_FILE_REGISTER_COUNT];
};
