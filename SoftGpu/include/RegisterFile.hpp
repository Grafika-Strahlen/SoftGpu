#pragma once

#include <cassert>
#include <Objects.hpp>
#include <NumTypes.hpp>
#include <ConPrinter.hpp>
#include "DebugManager.hpp"

/**
 * \brief Manages the set of register for a given SM.s
 *
 *   The register file allows for up to 8 simultaneous operations, with
 * the added caveat that there can be at most 4 operating with the low
 * bit set (and vice versa). The intent is for each of the 4 Ld/St
 * entities to be able to access 2 ports (1 high, and 1 low). This
 * allows it generally access a single 32 bit register, or 2 sequential
 * 32 bit registers forming a 64 bit value.
 *
 *   There are 16 register banks. Each bank translates to the hardware
 * representation of an array, which only allow for a single operation
 * to occur at a time. While this theoretically allows us to perform
 * up to 16 simultaneous operations, to simplify things there are only
 * 2 effective ports in use at once, each only able to access half of
 * the banks.
 */
class RegisterFile final
{
    DEFAULT_CONSTRUCT_PU(RegisterFile);
    DEFAULT_DESTRUCT(RegisterFile);
    DELETE_CM(RegisterFile);
public:
    enum class ECommand : u8
    {
        None = 0,
        ReadRegister,
        WriteRegister,
        CheckRead,
        CheckWrite,
        LockRead,
        LockWrite,
        Unlock,
        Reset
    };

    struct CommandPacket final
    {
        ECommand Command : 4;
        u16 TargetRegister : 11; // Only 11 bits because we differentiate the last bit with a different set of ports.
        u16 Pad : 1;
        u32* Value;
        bool* Successful;
        bool* Unsuccessful;
    };

    static inline constexpr uSys REGISTER_FILE_BANK_COUNT = 16;
    static inline constexpr uSys REGISTER_FILE_BANK_REGISTER_COUNT = 256;
    static inline constexpr uSys REGISTER_FILE_REGISTER_COUNT = REGISTER_FILE_BANK_COUNT * REGISTER_FILE_BANK_REGISTER_COUNT;
public:
    void Reset() noexcept
    {
        (void) ::std::memset(m_RegisterBank0, 0, sizeof(m_RegisterBank0));
        (void) ::std::memset(m_RegisterBank1, 0, sizeof(m_RegisterBank0));
        (void) ::std::memset(m_RegisterBank2, 0, sizeof(m_RegisterBank0));
        (void) ::std::memset(m_RegisterBank3, 0, sizeof(m_RegisterBank0));
        (void) ::std::memset(m_RegisterBank4, 0, sizeof(m_RegisterBank0));
        (void) ::std::memset(m_RegisterBank5, 0, sizeof(m_RegisterBank0));
        (void) ::std::memset(m_RegisterBank6, 0, sizeof(m_RegisterBank0));
        (void) ::std::memset(m_RegisterBank7, 0, sizeof(m_RegisterBank0));
        (void) ::std::memset(m_RegisterBank8, 0, sizeof(m_RegisterBank0));
        (void) ::std::memset(m_RegisterBank9, 0, sizeof(m_RegisterBank0));
        (void) ::std::memset(m_RegisterBankA, 0, sizeof(m_RegisterBank0));
        (void) ::std::memset(m_RegisterBankB, 0, sizeof(m_RegisterBank0));
        (void) ::std::memset(m_RegisterBankC, 0, sizeof(m_RegisterBank0));
        (void) ::std::memset(m_RegisterBankD, 0, sizeof(m_RegisterBank0));
        (void) ::std::memset(m_RegisterBankE, 0, sizeof(m_RegisterBank0));
        (void) ::std::memset(m_RegisterBankF, 0, sizeof(m_RegisterBank0));

        (void) ::std::memset(m_RegisterContestationMapBank0, 0, sizeof(m_RegisterContestationMapBank0));
        (void) ::std::memset(m_RegisterContestationMapBank1, 0, sizeof(m_RegisterContestationMapBank0));
        (void) ::std::memset(m_RegisterContestationMapBank2, 0, sizeof(m_RegisterContestationMapBank0));
        (void) ::std::memset(m_RegisterContestationMapBank3, 0, sizeof(m_RegisterContestationMapBank0));
        (void) ::std::memset(m_RegisterContestationMapBank4, 0, sizeof(m_RegisterContestationMapBank0));
        (void) ::std::memset(m_RegisterContestationMapBank5, 0, sizeof(m_RegisterContestationMapBank0));
        (void) ::std::memset(m_RegisterContestationMapBank6, 0, sizeof(m_RegisterContestationMapBank0));
        (void) ::std::memset(m_RegisterContestationMapBank7, 0, sizeof(m_RegisterContestationMapBank0));
        (void) ::std::memset(m_RegisterContestationMapBank8, 0, sizeof(m_RegisterContestationMapBank0));
        (void) ::std::memset(m_RegisterContestationMapBank9, 0, sizeof(m_RegisterContestationMapBank0));
        (void) ::std::memset(m_RegisterContestationMapBankA, 0, sizeof(m_RegisterContestationMapBank0));
        (void) ::std::memset(m_RegisterContestationMapBankB, 0, sizeof(m_RegisterContestationMapBank0));
        (void) ::std::memset(m_RegisterContestationMapBankC, 0, sizeof(m_RegisterContestationMapBank0));
        (void) ::std::memset(m_RegisterContestationMapBankD, 0, sizeof(m_RegisterContestationMapBank0));
        (void) ::std::memset(m_RegisterContestationMapBankE, 0, sizeof(m_RegisterContestationMapBank0));
        (void) ::std::memset(m_RegisterContestationMapBankF, 0, sizeof(m_RegisterContestationMapBank0));
    }

    // We'll use a pulsed model for handling multiple ports.
    void Clock() noexcept
    {
        ExecutePacket(m_Port0High, m_Port0Low);
        ExecutePacket(m_Port1High, m_Port1Low);
        ExecutePacket(m_Port2High, m_Port2Low);
        ExecutePacket(m_Port3High, m_Port3Low);
    }

    void InvokePort0High(const CommandPacket packet) noexcept
    {
        (void) ::std::memcpy(&m_Port0High, &packet, sizeof(packet));
    }

    void InvokePort1High(const CommandPacket packet) noexcept
    {
        (void) ::std::memcpy(&m_Port1High, &packet, sizeof(packet));
    }

    void InvokePort2High(const CommandPacket packet) noexcept
    {
        (void) ::std::memcpy(&m_Port2High, &packet, sizeof(packet));
    }

    void InvokePort3High(const CommandPacket packet) noexcept
    {
        (void) ::std::memcpy(&m_Port3High, &packet, sizeof(packet));
    }

    void InvokePort0Low(const CommandPacket packet) noexcept
    {
        (void) ::std::memcpy(&m_Port0Low, &packet, sizeof(packet));
    }

    void InvokePort1Low(const CommandPacket packet) noexcept
    {
        (void) ::std::memcpy(&m_Port1Low, &packet, sizeof(packet));
    }

    void InvokePort2Low(const CommandPacket packet) noexcept
    {
        (void) ::std::memcpy(&m_Port2Low, &packet, sizeof(packet));
    }

    void InvokePort3Low(const CommandPacket packet) noexcept
    {
        (void) ::std::memcpy(&m_Port3Low, &packet, sizeof(packet));
    }

    void ReportRegisters(const u32 smIndex) const noexcept
    {
        if(GlobalDebug.IsAttached())
        {
            {
                GlobalDebug.WriteRawInfo(&DebugCodeReportRegisterFile, sizeof(DebugCodeReportRegisterFile));
                constexpr u32 length = sizeof(smIndex) + sizeof(u32) * REGISTER_FILE_REGISTER_COUNT;
                GlobalDebug.WriteRawInfo(&length, sizeof(length));
                GlobalDebug.WriteRawInfo(&smIndex, sizeof(smIndex));
                for(u32 i = 0; i < REGISTER_FILE_REGISTER_COUNT; ++i)
                {
                    GlobalDebug.WriteRawInfo(&m_RegisterBank0[i], sizeof(u32));
                    GlobalDebug.WriteRawInfo(&m_RegisterBank1[i], sizeof(u32));
                    GlobalDebug.WriteRawInfo(&m_RegisterBank2[i], sizeof(u32));
                    GlobalDebug.WriteRawInfo(&m_RegisterBank3[i], sizeof(u32));
                    GlobalDebug.WriteRawInfo(&m_RegisterBank4[i], sizeof(u32));
                    GlobalDebug.WriteRawInfo(&m_RegisterBank5[i], sizeof(u32));
                    GlobalDebug.WriteRawInfo(&m_RegisterBank6[i], sizeof(u32));
                    GlobalDebug.WriteRawInfo(&m_RegisterBank7[i], sizeof(u32));
                    GlobalDebug.WriteRawInfo(&m_RegisterBank8[i], sizeof(u32));
                    GlobalDebug.WriteRawInfo(&m_RegisterBank9[i], sizeof(u32));
                    GlobalDebug.WriteRawInfo(&m_RegisterBankA[i], sizeof(u32));
                    GlobalDebug.WriteRawInfo(&m_RegisterBankB[i], sizeof(u32));
                    GlobalDebug.WriteRawInfo(&m_RegisterBankC[i], sizeof(u32));
                    GlobalDebug.WriteRawInfo(&m_RegisterBankD[i], sizeof(u32));
                    GlobalDebug.WriteRawInfo(&m_RegisterBankE[i], sizeof(u32));
                    GlobalDebug.WriteRawInfo(&m_RegisterBankF[i], sizeof(u32));
                }
            }

            {
                GlobalDebug.WriteRawInfo(&DebugCodeReportRegisterContestion, sizeof(DebugCodeReportRegisterContestion));
                constexpr u32 length = sizeof(smIndex) + sizeof(u8) * REGISTER_FILE_REGISTER_COUNT;
                GlobalDebug.WriteRawInfo(&length, sizeof(length));
                GlobalDebug.WriteRawInfo(&smIndex, sizeof(smIndex));
                for(u32 i = 0; i < REGISTER_FILE_REGISTER_COUNT; ++i)
                {
                    GlobalDebug.WriteRawInfo(&m_RegisterContestationMapBank0[i], sizeof(u8));
                    GlobalDebug.WriteRawInfo(&m_RegisterContestationMapBank1[i], sizeof(u8));
                    GlobalDebug.WriteRawInfo(&m_RegisterContestationMapBank2[i], sizeof(u8));
                    GlobalDebug.WriteRawInfo(&m_RegisterContestationMapBank3[i], sizeof(u8));
                    GlobalDebug.WriteRawInfo(&m_RegisterContestationMapBank4[i], sizeof(u8));
                    GlobalDebug.WriteRawInfo(&m_RegisterContestationMapBank5[i], sizeof(u8));
                    GlobalDebug.WriteRawInfo(&m_RegisterContestationMapBank6[i], sizeof(u8));
                    GlobalDebug.WriteRawInfo(&m_RegisterContestationMapBank7[i], sizeof(u8));
                    GlobalDebug.WriteRawInfo(&m_RegisterContestationMapBank8[i], sizeof(u8));
                    GlobalDebug.WriteRawInfo(&m_RegisterContestationMapBank9[i], sizeof(u8));
                    GlobalDebug.WriteRawInfo(&m_RegisterContestationMapBankA[i], sizeof(u8));
                    GlobalDebug.WriteRawInfo(&m_RegisterContestationMapBankB[i], sizeof(u8));
                    GlobalDebug.WriteRawInfo(&m_RegisterContestationMapBankC[i], sizeof(u8));
                    GlobalDebug.WriteRawInfo(&m_RegisterContestationMapBankD[i], sizeof(u8));
                    GlobalDebug.WriteRawInfo(&m_RegisterContestationMapBankE[i], sizeof(u8));
                    GlobalDebug.WriteRawInfo(&m_RegisterContestationMapBankF[i], sizeof(u8));
                }
            }
        }
    }
private:
    void ExecutePacket(const CommandPacket packetHigh, const CommandPacket packetLow) noexcept
    {
        switch(packetHigh.Command)
        {
            case ECommand::ReadRegister:
                switch(packetHigh.TargetRegister & 0x7)
                {
                    case 0: *packetHigh.Value = m_RegisterBank1[packetHigh.TargetRegister >> 3]; break;
                    case 1: *packetHigh.Value = m_RegisterBank3[packetHigh.TargetRegister >> 3]; break;
                    case 2: *packetHigh.Value = m_RegisterBank5[packetHigh.TargetRegister >> 3]; break;
                    case 3: *packetHigh.Value = m_RegisterBank7[packetHigh.TargetRegister >> 3]; break;
                    case 4: *packetHigh.Value = m_RegisterBank9[packetHigh.TargetRegister >> 3]; break;
                    case 5: *packetHigh.Value = m_RegisterBankB[packetHigh.TargetRegister >> 3]; break;
                    case 6: *packetHigh.Value = m_RegisterBankD[packetHigh.TargetRegister >> 3]; break;
                    case 7: *packetHigh.Value = m_RegisterBankF[packetHigh.TargetRegister >> 3]; break;
                    default: break;
                }
                *packetHigh.Successful = true;
                *packetHigh.Unsuccessful = false;
                break;
            case ECommand::WriteRegister:
                switch(packetHigh.TargetRegister & 0x7)
                {
                    case 0: m_RegisterBank1[packetHigh.TargetRegister >> 3] = *packetHigh.Value; break;
                    case 1: m_RegisterBank3[packetHigh.TargetRegister >> 3] = *packetHigh.Value; break;
                    case 2: m_RegisterBank5[packetHigh.TargetRegister >> 3] = *packetHigh.Value; break;
                    case 3: m_RegisterBank7[packetHigh.TargetRegister >> 3] = *packetHigh.Value; break;
                    case 4: m_RegisterBank9[packetHigh.TargetRegister >> 3] = *packetHigh.Value; break;
                    case 5: m_RegisterBankB[packetHigh.TargetRegister >> 3] = *packetHigh.Value; break;
                    case 6: m_RegisterBankD[packetHigh.TargetRegister >> 3] = *packetHigh.Value; break;
                    case 7: m_RegisterBankF[packetHigh.TargetRegister >> 3] = *packetHigh.Value; break;
                    default: break;
                }
                *packetHigh.Successful = true;
                *packetHigh.Unsuccessful = false;
                break;
            case ECommand::CheckRead:
            {
                u32 contestation;

                switch(packetHigh.TargetRegister & 0x7)
                {
                    case 0: contestation = m_RegisterContestationMapBank1[packetHigh.TargetRegister >> 3]; break;
                    case 1: contestation = m_RegisterContestationMapBank3[packetHigh.TargetRegister >> 3]; break;
                    case 2: contestation = m_RegisterContestationMapBank5[packetHigh.TargetRegister >> 3]; break;
                    case 3: contestation = m_RegisterContestationMapBank7[packetHigh.TargetRegister >> 3]; break;
                    case 4: contestation = m_RegisterContestationMapBank9[packetHigh.TargetRegister >> 3]; break;
                    case 5: contestation = m_RegisterContestationMapBankB[packetHigh.TargetRegister >> 3]; break;
                    case 6: contestation = m_RegisterContestationMapBankD[packetHigh.TargetRegister >> 3]; break;
                    case 7: contestation = m_RegisterContestationMapBankF[packetHigh.TargetRegister >> 3]; break;
                    default: contestation = 0xFF; break;
                }

                if(contestation != 1 && contestation != 0xFF)
                {
                    *packetHigh.Successful = true;
                    *packetHigh.Unsuccessful = false;
                }
                else
                {
                    *packetHigh.Successful = false;
                    *packetHigh.Unsuccessful = true;
                }
                break;
            }
            case ECommand::CheckWrite:
            {
                u8 contestation;

                switch(packetHigh.TargetRegister & 0x7)
                {
                    case 0: contestation = m_RegisterContestationMapBank1[packetHigh.TargetRegister >> 3]; break;
                    case 1: contestation = m_RegisterContestationMapBank3[packetHigh.TargetRegister >> 3]; break;
                    case 2: contestation = m_RegisterContestationMapBank5[packetHigh.TargetRegister >> 3]; break;
                    case 3: contestation = m_RegisterContestationMapBank7[packetHigh.TargetRegister >> 3]; break;
                    case 4: contestation = m_RegisterContestationMapBank9[packetHigh.TargetRegister >> 3]; break;
                    case 5: contestation = m_RegisterContestationMapBankB[packetHigh.TargetRegister >> 3]; break;
                    case 6: contestation = m_RegisterContestationMapBankD[packetHigh.TargetRegister >> 3]; break;
                    case 7: contestation = m_RegisterContestationMapBankF[packetHigh.TargetRegister >> 3]; break;
                    default: contestation = 0xFF; break;
                }

                if(contestation == 0)
                {
                    *packetHigh.Successful = true;
                    *packetHigh.Unsuccessful = false;
                }
                else
                {
                    *packetHigh.Successful = false;
                    *packetHigh.Unsuccessful = true;
                }
                break;
            }
            case ECommand::LockRead:
            {
                u8 contestation;

                switch(packetHigh.TargetRegister & 0x7)
                {
                    case 0: contestation = m_RegisterContestationMapBank1[packetHigh.TargetRegister >> 3]; break;
                    case 1: contestation = m_RegisterContestationMapBank3[packetHigh.TargetRegister >> 3]; break;
                    case 2: contestation = m_RegisterContestationMapBank5[packetHigh.TargetRegister >> 3]; break;
                    case 3: contestation = m_RegisterContestationMapBank7[packetHigh.TargetRegister >> 3]; break;
                    case 4: contestation = m_RegisterContestationMapBank9[packetHigh.TargetRegister >> 3]; break;
                    case 5: contestation = m_RegisterContestationMapBankB[packetHigh.TargetRegister >> 3]; break;
                    case 6: contestation = m_RegisterContestationMapBankD[packetHigh.TargetRegister >> 3]; break;
                    case 7: contestation = m_RegisterContestationMapBankF[packetHigh.TargetRegister >> 3]; break;
                    default: contestation = 0xFF; break;
                }

                if(contestation == 0)
                {
                    contestation = 2;
                }
                else
                {
                    if(contestation == 0xFF)
                    {
                        ConPrinter::PrintLn("Register Read Lock {} has overflowed.", packetHigh.TargetRegister);

                        *packetHigh.Successful = false;
                        *packetHigh.Unsuccessful = true;

                        assert(contestation != 0xFF);
                    }

                    ++contestation;
                }

                switch(packetHigh.TargetRegister & 0x7)
                {
                    case 0: m_RegisterContestationMapBank1[packetHigh.TargetRegister >> 3] = contestation; break;
                    case 1: m_RegisterContestationMapBank3[packetHigh.TargetRegister >> 3] = contestation; break;
                    case 2: m_RegisterContestationMapBank5[packetHigh.TargetRegister >> 3] = contestation; break;
                    case 3: m_RegisterContestationMapBank7[packetHigh.TargetRegister >> 3] = contestation; break;
                    case 4: m_RegisterContestationMapBank9[packetHigh.TargetRegister >> 3] = contestation; break;
                    case 5: m_RegisterContestationMapBankB[packetHigh.TargetRegister >> 3] = contestation; break;
                    case 6: m_RegisterContestationMapBankD[packetHigh.TargetRegister >> 3] = contestation; break;
                    case 7: m_RegisterContestationMapBankF[packetHigh.TargetRegister >> 3] = contestation; break;
                    default: break;
                }

                *packetHigh.Successful = true;
                *packetHigh.Unsuccessful = false;
                break;
            }
            case ECommand::LockWrite:
            {
                u8 contestation;

                switch(packetHigh.TargetRegister & 0x7)
                {
                    case 0: contestation = m_RegisterContestationMapBank1[packetHigh.TargetRegister >> 3]; break;
                    case 1: contestation = m_RegisterContestationMapBank3[packetHigh.TargetRegister >> 3]; break;
                    case 2: contestation = m_RegisterContestationMapBank5[packetHigh.TargetRegister >> 3]; break;
                    case 3: contestation = m_RegisterContestationMapBank7[packetHigh.TargetRegister >> 3]; break;
                    case 4: contestation = m_RegisterContestationMapBank9[packetHigh.TargetRegister >> 3]; break;
                    case 5: contestation = m_RegisterContestationMapBankB[packetHigh.TargetRegister >> 3]; break;
                    case 6: contestation = m_RegisterContestationMapBankD[packetHigh.TargetRegister >> 3]; break;
                    case 7: contestation = m_RegisterContestationMapBankF[packetHigh.TargetRegister >> 3]; break;
                    default: contestation = 0xFF; break;
                }

                if(contestation != 0)
                {
                    *packetHigh.Successful = false;
                    *packetHigh.Unsuccessful = true;
                }
                else
                {
                    contestation = 1;

                    switch(packetHigh.TargetRegister & 0x7)
                    {
                        case 0: m_RegisterContestationMapBank1[packetHigh.TargetRegister >> 3] = contestation; break;
                        case 1: m_RegisterContestationMapBank3[packetHigh.TargetRegister >> 3] = contestation; break;
                        case 2: m_RegisterContestationMapBank5[packetHigh.TargetRegister >> 3] = contestation; break;
                        case 3: m_RegisterContestationMapBank7[packetHigh.TargetRegister >> 3] = contestation; break;
                        case 4: m_RegisterContestationMapBank9[packetHigh.TargetRegister >> 3] = contestation; break;
                        case 5: m_RegisterContestationMapBankB[packetHigh.TargetRegister >> 3] = contestation; break;
                        case 6: m_RegisterContestationMapBankD[packetHigh.TargetRegister >> 3] = contestation; break;
                        case 7: m_RegisterContestationMapBankF[packetHigh.TargetRegister >> 3] = contestation; break;
                        default: break;
                    }

                    *packetHigh.Successful = true;
                    *packetHigh.Unsuccessful = false;
                }
                break;
            }
            case ECommand::Unlock:
            {
                u8 contestation;

                switch(packetHigh.TargetRegister & 0x7)
                {
                    case 0: contestation = m_RegisterContestationMapBank1[packetHigh.TargetRegister >> 3]; break;
                    case 1: contestation = m_RegisterContestationMapBank3[packetHigh.TargetRegister >> 3]; break;
                    case 2: contestation = m_RegisterContestationMapBank5[packetHigh.TargetRegister >> 3]; break;
                    case 3: contestation = m_RegisterContestationMapBank7[packetHigh.TargetRegister >> 3]; break;
                    case 4: contestation = m_RegisterContestationMapBank9[packetHigh.TargetRegister >> 3]; break;
                    case 5: contestation = m_RegisterContestationMapBankB[packetHigh.TargetRegister >> 3]; break;
                    case 6: contestation = m_RegisterContestationMapBankD[packetHigh.TargetRegister >> 3]; break;
                    case 7: contestation = m_RegisterContestationMapBankF[packetHigh.TargetRegister >> 3]; break;
                    default: contestation = 0xFF; break;
                }

                --contestation;

                if(contestation == 0xFF)
                {
                    ConPrinter::PrintLn("Register Unlock {} has underflowed.", packetHigh.TargetRegister);

                    *packetHigh.Successful = false;
                    *packetHigh.Unsuccessful = true;

                    assert(contestation != 0xFF);
                }

                // If that was the last read lock the value will now be 1, which is actually the write lock. Set it to no lock.
                if(contestation == 1)
                {
                    contestation = 0;
                }

                switch(packetHigh.TargetRegister & 0x7)
                {
                    case 0: m_RegisterContestationMapBank1[packetHigh.TargetRegister >> 3] = contestation; break;
                    case 1: m_RegisterContestationMapBank3[packetHigh.TargetRegister >> 3] = contestation; break;
                    case 2: m_RegisterContestationMapBank5[packetHigh.TargetRegister >> 3] = contestation; break;
                    case 3: m_RegisterContestationMapBank7[packetHigh.TargetRegister >> 3] = contestation; break;
                    case 4: m_RegisterContestationMapBank9[packetHigh.TargetRegister >> 3] = contestation; break;
                    case 5: m_RegisterContestationMapBankB[packetHigh.TargetRegister >> 3] = contestation; break;
                    case 6: m_RegisterContestationMapBankD[packetHigh.TargetRegister >> 3] = contestation; break;
                    case 7: m_RegisterContestationMapBankF[packetHigh.TargetRegister >> 3] = contestation; break;
                    default: break;
                }

                *packetHigh.Successful = true;
                *packetHigh.Unsuccessful = false;
                break;
            }
            case ECommand::Reset:
                // This is used for synchronization in hardware.
                break;
            case ECommand::None: break;
            default:
                ConPrinter::PrintLn("Default case invoked while handling register file command. This should not be possible. {}", static_cast<u8>(packetHigh.Command));
                assert(false);
                break;
        }

        switch(packetLow.Command)
        {
            case ECommand::ReadRegister:
                switch(packetLow.TargetRegister & 0x7)
                {
                    case 0: *packetLow.Value = m_RegisterBank0[packetLow.TargetRegister >> 3]; break;
                    case 1: *packetLow.Value = m_RegisterBank2[packetLow.TargetRegister >> 3]; break;
                    case 2: *packetLow.Value = m_RegisterBank4[packetLow.TargetRegister >> 3]; break;
                    case 3: *packetLow.Value = m_RegisterBank6[packetLow.TargetRegister >> 3]; break;
                    case 4: *packetLow.Value = m_RegisterBank8[packetLow.TargetRegister >> 3]; break;
                    case 5: *packetLow.Value = m_RegisterBankA[packetLow.TargetRegister >> 3]; break;
                    case 6: *packetLow.Value = m_RegisterBankC[packetLow.TargetRegister >> 3]; break;
                    case 7: *packetLow.Value = m_RegisterBankE[packetLow.TargetRegister >> 3]; break;
                    default: break;
                }
                *packetLow.Successful = true;
                *packetLow.Unsuccessful = false;
                break;
            case ECommand::WriteRegister:
                switch(packetLow.TargetRegister & 0x7)
                {
                    case 0: m_RegisterBank0[packetLow.TargetRegister >> 3] = *packetLow.Value; break;
                    case 1: m_RegisterBank2[packetLow.TargetRegister >> 3] = *packetLow.Value; break;
                    case 2: m_RegisterBank4[packetLow.TargetRegister >> 3] = *packetLow.Value; break;
                    case 3: m_RegisterBank6[packetLow.TargetRegister >> 3] = *packetLow.Value; break;
                    case 4: m_RegisterBank8[packetLow.TargetRegister >> 3] = *packetLow.Value; break;
                    case 5: m_RegisterBankA[packetLow.TargetRegister >> 3] = *packetLow.Value; break;
                    case 6: m_RegisterBankC[packetLow.TargetRegister >> 3] = *packetLow.Value; break;
                    case 7: m_RegisterBankE[packetLow.TargetRegister >> 3] = *packetLow.Value; break;
                    default: break;
                }
                *packetLow.Successful = true;
                *packetLow.Unsuccessful = false;
                break;
            case ECommand::CheckRead:
            {
                u8 contestation;

                switch(packetLow.TargetRegister & 0x7)
                {
                    case 0: contestation = m_RegisterContestationMapBank0[packetLow.TargetRegister >> 3]; break;
                    case 1: contestation = m_RegisterContestationMapBank2[packetLow.TargetRegister >> 3]; break;
                    case 2: contestation = m_RegisterContestationMapBank4[packetLow.TargetRegister >> 3]; break;
                    case 3: contestation = m_RegisterContestationMapBank6[packetLow.TargetRegister >> 3]; break;
                    case 4: contestation = m_RegisterContestationMapBank8[packetLow.TargetRegister >> 3]; break;
                    case 5: contestation = m_RegisterContestationMapBankA[packetLow.TargetRegister >> 3]; break;
                    case 6: contestation = m_RegisterContestationMapBankC[packetLow.TargetRegister >> 3]; break;
                    case 7: contestation = m_RegisterContestationMapBankE[packetLow.TargetRegister >> 3]; break;
                    default: contestation = 0xFF; break;
                }

                if(contestation != 1 && contestation != 0xFF)
                {
                    *packetLow.Successful = true;
                    *packetLow.Unsuccessful = false;
                }
                else
                {
                    *packetLow.Successful = false;
                    *packetLow.Unsuccessful = true;
                }
                break;
            }
            case ECommand::CheckWrite:
            {
                u8 contestation;

                switch(packetLow.TargetRegister & 0x7)
                {
                    case 0: contestation = m_RegisterContestationMapBank0[packetLow.TargetRegister >> 3]; break;
                    case 1: contestation = m_RegisterContestationMapBank2[packetLow.TargetRegister >> 3]; break;
                    case 2: contestation = m_RegisterContestationMapBank4[packetLow.TargetRegister >> 3]; break;
                    case 3: contestation = m_RegisterContestationMapBank6[packetLow.TargetRegister >> 3]; break;
                    case 4: contestation = m_RegisterContestationMapBank8[packetLow.TargetRegister >> 3]; break;
                    case 5: contestation = m_RegisterContestationMapBankA[packetLow.TargetRegister >> 3]; break;
                    case 6: contestation = m_RegisterContestationMapBankC[packetLow.TargetRegister >> 3]; break;
                    case 7: contestation = m_RegisterContestationMapBankE[packetLow.TargetRegister >> 3]; break;
                    default: contestation = 0xFF; break;
                }

                if(contestation == 0)
                {
                    *packetLow.Successful = true;
                    *packetLow.Unsuccessful = false;
                }
                else
                {
                    *packetLow.Successful = false;
                    *packetLow.Unsuccessful = true;
                }
                break;
            }
            case ECommand::LockRead:
            {
                u8 contestation;

                switch(packetLow.TargetRegister & 0x7)
                {
                    case 0: contestation = m_RegisterContestationMapBank0[packetLow.TargetRegister >> 3]; break;
                    case 1: contestation = m_RegisterContestationMapBank2[packetLow.TargetRegister >> 3]; break;
                    case 2: contestation = m_RegisterContestationMapBank4[packetLow.TargetRegister >> 3]; break;
                    case 3: contestation = m_RegisterContestationMapBank6[packetLow.TargetRegister >> 3]; break;
                    case 4: contestation = m_RegisterContestationMapBank8[packetLow.TargetRegister >> 3]; break;
                    case 5: contestation = m_RegisterContestationMapBankA[packetLow.TargetRegister >> 3]; break;
                    case 6: contestation = m_RegisterContestationMapBankC[packetLow.TargetRegister >> 3]; break;
                    case 7: contestation = m_RegisterContestationMapBankE[packetLow.TargetRegister >> 3]; break;
                    default: contestation = 0xFF; break;
                }

                if(contestation == 0)
                {
                    contestation = 2;
                }
                else
                {
                    if(contestation == 0xFF)
                    {
                        ConPrinter::PrintLn("Register Read Lock {} has overflowed.", packetLow.TargetRegister);

                        *packetLow.Successful = false;
                        *packetLow.Unsuccessful = true;

                        assert(contestation != 0xFF);
                    }

                    ++contestation;
                }

                switch(packetLow.TargetRegister & 0x7)
                {
                    case 0: m_RegisterContestationMapBank0[packetLow.TargetRegister >> 3] = contestation; break;
                    case 1: m_RegisterContestationMapBank2[packetLow.TargetRegister >> 3] = contestation; break;
                    case 2: m_RegisterContestationMapBank4[packetLow.TargetRegister >> 3] = contestation; break;
                    case 3: m_RegisterContestationMapBank6[packetLow.TargetRegister >> 3] = contestation; break;
                    case 4: m_RegisterContestationMapBank8[packetLow.TargetRegister >> 3] = contestation; break;
                    case 5: m_RegisterContestationMapBankA[packetLow.TargetRegister >> 3] = contestation; break;
                    case 6: m_RegisterContestationMapBankC[packetLow.TargetRegister >> 3] = contestation; break;
                    case 7: m_RegisterContestationMapBankE[packetLow.TargetRegister >> 3] = contestation; break;
                    default: break;
                }

                *packetLow.Successful = true;
                *packetLow.Unsuccessful = false;
                break;
            }
            case ECommand::LockWrite:
            {
                u8 contestation;

                switch(packetLow.TargetRegister & 0x7)
                {
                    case 0: contestation = m_RegisterContestationMapBank0[packetLow.TargetRegister >> 3]; break;
                    case 1: contestation = m_RegisterContestationMapBank2[packetLow.TargetRegister >> 3]; break;
                    case 2: contestation = m_RegisterContestationMapBank4[packetLow.TargetRegister >> 3]; break;
                    case 3: contestation = m_RegisterContestationMapBank6[packetLow.TargetRegister >> 3]; break;
                    case 4: contestation = m_RegisterContestationMapBank8[packetLow.TargetRegister >> 3]; break;
                    case 5: contestation = m_RegisterContestationMapBankA[packetLow.TargetRegister >> 3]; break;
                    case 6: contestation = m_RegisterContestationMapBankC[packetLow.TargetRegister >> 3]; break;
                    case 7: contestation = m_RegisterContestationMapBankE[packetLow.TargetRegister >> 3]; break;
                    default: contestation = 0xFF; break;
                }

                if(contestation != 0)
                {
                    *packetLow.Successful = false;
                    *packetLow.Unsuccessful = true;
                }
                else
                {
                    contestation = 1;

                    switch(packetLow.TargetRegister & 0x7)
                    {
                        case 0: m_RegisterContestationMapBank0[packetLow.TargetRegister >> 3] = contestation; break;
                        case 1: m_RegisterContestationMapBank2[packetLow.TargetRegister >> 3] = contestation; break;
                        case 2: m_RegisterContestationMapBank4[packetLow.TargetRegister >> 3] = contestation; break;
                        case 3: m_RegisterContestationMapBank6[packetLow.TargetRegister >> 3] = contestation; break;
                        case 4: m_RegisterContestationMapBank8[packetLow.TargetRegister >> 3] = contestation; break;
                        case 5: m_RegisterContestationMapBankA[packetLow.TargetRegister >> 3] = contestation; break;
                        case 6: m_RegisterContestationMapBankC[packetLow.TargetRegister >> 3] = contestation; break;
                        case 7: m_RegisterContestationMapBankE[packetLow.TargetRegister >> 3] = contestation; break;
                        default: break;
                    }

                    *packetLow.Successful = true;
                    *packetLow.Unsuccessful = false;
                }
                break;
            }
            case ECommand::Unlock:
            {
                u8 contestation;

                switch(packetLow.TargetRegister & 0x7)
                {
                    case 0: contestation = m_RegisterContestationMapBank0[packetLow.TargetRegister >> 3]; break;
                    case 1: contestation = m_RegisterContestationMapBank2[packetLow.TargetRegister >> 3]; break;
                    case 2: contestation = m_RegisterContestationMapBank4[packetLow.TargetRegister >> 3]; break;
                    case 3: contestation = m_RegisterContestationMapBank6[packetLow.TargetRegister >> 3]; break;
                    case 4: contestation = m_RegisterContestationMapBank8[packetLow.TargetRegister >> 3]; break;
                    case 5: contestation = m_RegisterContestationMapBankA[packetLow.TargetRegister >> 3]; break;
                    case 6: contestation = m_RegisterContestationMapBankC[packetLow.TargetRegister >> 3]; break;
                    case 7: contestation = m_RegisterContestationMapBankE[packetLow.TargetRegister >> 3]; break;
                    default: contestation = 0xFF; break;
                }

                --contestation;

                if(contestation == 0xFF)
                {
                    ConPrinter::PrintLn("Register Unlock {} has underflowed.", packetLow.TargetRegister);

                    *packetLow.Successful = false;
                    *packetLow.Unsuccessful = true;

                    assert(contestation != 0xFF);
                }

                // If that was the last read lock the value will now be 1, which is actually the write lock. Set it to no lock.
                if(contestation == 1)
                {
                    contestation = 0;
                }

                switch(packetLow.TargetRegister & 0x7)
                {
                    case 0: m_RegisterContestationMapBank0[packetLow.TargetRegister >> 3] = contestation; break;
                    case 1: m_RegisterContestationMapBank2[packetLow.TargetRegister >> 3] = contestation; break;
                    case 2: m_RegisterContestationMapBank4[packetLow.TargetRegister >> 3] = contestation; break;
                    case 3: m_RegisterContestationMapBank6[packetLow.TargetRegister >> 3] = contestation; break;
                    case 4: m_RegisterContestationMapBank8[packetLow.TargetRegister >> 3] = contestation; break;
                    case 5: m_RegisterContestationMapBankA[packetLow.TargetRegister >> 3] = contestation; break;
                    case 6: m_RegisterContestationMapBankC[packetLow.TargetRegister >> 3] = contestation; break;
                    case 7: m_RegisterContestationMapBankE[packetLow.TargetRegister >> 3] = contestation; break;
                    default: break;
                }

                *packetLow.Successful = true;
                *packetLow.Unsuccessful = false;
                break;
            }
            case ECommand::Reset:
                // This is used for synchronization in hardware.
                break;
            case ECommand::None: break;
            default:
                ConPrinter::PrintLn("Default case invoked while handling register file command. This should not be possible. {}", static_cast<u8>(packetLow.Command));
                assert(false);
                break;
        }
    }
private:
    u32 m_RegisterBank0[REGISTER_FILE_BANK_REGISTER_COUNT];
    u32 m_RegisterBank1[REGISTER_FILE_BANK_REGISTER_COUNT];
    u32 m_RegisterBank2[REGISTER_FILE_BANK_REGISTER_COUNT];
    u32 m_RegisterBank3[REGISTER_FILE_BANK_REGISTER_COUNT];
    u32 m_RegisterBank4[REGISTER_FILE_BANK_REGISTER_COUNT];
    u32 m_RegisterBank5[REGISTER_FILE_BANK_REGISTER_COUNT];
    u32 m_RegisterBank6[REGISTER_FILE_BANK_REGISTER_COUNT];
    u32 m_RegisterBank7[REGISTER_FILE_BANK_REGISTER_COUNT];
    u32 m_RegisterBank8[REGISTER_FILE_BANK_REGISTER_COUNT];
    u32 m_RegisterBank9[REGISTER_FILE_BANK_REGISTER_COUNT];
    u32 m_RegisterBankA[REGISTER_FILE_BANK_REGISTER_COUNT];
    u32 m_RegisterBankB[REGISTER_FILE_BANK_REGISTER_COUNT];
    u32 m_RegisterBankC[REGISTER_FILE_BANK_REGISTER_COUNT];
    u32 m_RegisterBankD[REGISTER_FILE_BANK_REGISTER_COUNT];
    u32 m_RegisterBankE[REGISTER_FILE_BANK_REGISTER_COUNT];
    u32 m_RegisterBankF[REGISTER_FILE_BANK_REGISTER_COUNT];

    u8 m_RegisterContestationMapBank0[REGISTER_FILE_BANK_REGISTER_COUNT];
    u8 m_RegisterContestationMapBank1[REGISTER_FILE_BANK_REGISTER_COUNT];
    u8 m_RegisterContestationMapBank2[REGISTER_FILE_BANK_REGISTER_COUNT];
    u8 m_RegisterContestationMapBank3[REGISTER_FILE_BANK_REGISTER_COUNT];
    u8 m_RegisterContestationMapBank4[REGISTER_FILE_BANK_REGISTER_COUNT];
    u8 m_RegisterContestationMapBank5[REGISTER_FILE_BANK_REGISTER_COUNT];
    u8 m_RegisterContestationMapBank6[REGISTER_FILE_BANK_REGISTER_COUNT];
    u8 m_RegisterContestationMapBank7[REGISTER_FILE_BANK_REGISTER_COUNT];
    u8 m_RegisterContestationMapBank8[REGISTER_FILE_BANK_REGISTER_COUNT];
    u8 m_RegisterContestationMapBank9[REGISTER_FILE_BANK_REGISTER_COUNT];
    u8 m_RegisterContestationMapBankA[REGISTER_FILE_BANK_REGISTER_COUNT];
    u8 m_RegisterContestationMapBankB[REGISTER_FILE_BANK_REGISTER_COUNT];
    u8 m_RegisterContestationMapBankC[REGISTER_FILE_BANK_REGISTER_COUNT];
    u8 m_RegisterContestationMapBankD[REGISTER_FILE_BANK_REGISTER_COUNT];
    u8 m_RegisterContestationMapBankE[REGISTER_FILE_BANK_REGISTER_COUNT];
    u8 m_RegisterContestationMapBankF[REGISTER_FILE_BANK_REGISTER_COUNT];

    // u32 m_Registers[REGISTER_FILE_REGISTER_COUNT];

    // This contains information about how each register is being used.
    // If the value is zero the register is unused.
    // If the value is one it is locked for writes.
    // Otherwise, the register is locked for reads. Any number of simultaneous reads are allowed.
    // u8 m_RegisterContestationMap[REGISTER_FILE_REGISTER_COUNT];

    // Storage is just because hardware style ports don't work with the transient nature of functions.
    CommandPacket m_Port0High;
    CommandPacket m_Port1High;
    CommandPacket m_Port2High;
    CommandPacket m_Port3High;
    CommandPacket m_Port0Low;
    CommandPacket m_Port1Low;
    CommandPacket m_Port2Low;
    CommandPacket m_Port3Low;
};
