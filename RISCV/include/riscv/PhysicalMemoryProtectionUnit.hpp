/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <Common.hpp>
#include <BitVector.hpp>
#include <algorithm>
#include "ControlBus.hpp"

namespace riscv {

class PMPReceiverSample
{
public:
    void ReceivePhysicalMemoryProtectionUnit_CSR([[maybe_unused]] const u32 index, [[maybe_unused]] const u32 csr) noexcept { }
    void ReceivePhysicalMemoryProtectionUnit_Fault([[maybe_unused]] const u32 index, [[maybe_unused]] const bool fault) noexcept { }
};


template<
    typename Receiver = PMPReceiverSample,
    u32 TRegionCount = 16,
    u32 TGranularity = 4096,
    bool EnableTopOfRange = true,
    bool EnableNaturallyAligned = true
>
class PhysicalMemoryProtectionUnit final
{
    DEFAULT_DESTRUCT(PhysicalMemoryProtectionUnit);
    DELETE_CM(PhysicalMemoryProtectionUnit);
public:
    // using Receiver = PMPReceiverSample;
    constexpr static inline u32 MaxRegionCount = 16;
    constexpr static inline u32 RegionCount = ::std::clamp<u32>(TRegionCount, 0, MaxRegionCount);
    constexpr static inline u32 Granularity = TGranularity < 4 ? 4 : 1 << log2i(TGranularity);
    constexpr static inline u32 GranularityIndex = log2i(Granularity);

    constexpr static inline bool PrivilegeModeMachine = true;
    constexpr static inline bool PrivilegeModeUser = true;

    constexpr static inline u32 Config0 = 0x3A0;
    constexpr static inline u32 Config1 = 0x3A1;
    constexpr static inline u32 Config2 = 0x3A2;
    constexpr static inline u32 Config3 = 0x3A3;

    constexpr static inline u32 Address0  = 0x3B0;
    constexpr static inline u32 Address1  = 0x3B1;
    constexpr static inline u32 Address2  = 0x3B2;
    constexpr static inline u32 Address3  = 0x3B3;
    constexpr static inline u32 Address4  = 0x3B4;
    constexpr static inline u32 Address5  = 0x3B5;
    constexpr static inline u32 Address6  = 0x3B6;
    constexpr static inline u32 Address7  = 0x3B7;
    constexpr static inline u32 Address8  = 0x3B8;
    constexpr static inline u32 Address9  = 0x3B9;
    constexpr static inline u32 Address10 = 0x3BA;
    constexpr static inline u32 Address11 = 0x3BB;
    constexpr static inline u32 Address12 = 0x3BC;
    constexpr static inline u32 Address13 = 0x3BD;
    constexpr static inline u32 Address14 = 0x3BE;
    constexpr static inline u32 Address15 = 0x3BF;

    enum Mode : u8
    {
        ModeOff = 0b00,
        ModeTopOfRange = 0b01,
        ModeNaturallyAligned4 = 0b10,
        ModeNaturallyAlignedPowerOf2 = 0b11
    };

    #pragma pack(push, 1)
    struct ConfigData final
    {
        u8 ReadPermit : 1;
        u8 WritePermit : 1;
        u8 ExecutePermit : 1;
        Mode Mode : 2;
        u8 Pad0 : 2;
        u8 LockedEntry : 1;
    };
    #pragma pack(pop)
private:
    SENSITIVITY_DECL(p_Reset_n, p_Clock);

    SIGNAL_ENTITIES();
public:
    PhysicalMemoryProtectionUnit(Receiver* const parent, const u32 index = 0) noexcept
        : m_Parent(parent)
        , m_Index(index)
        , p_Reset_n(0)
        , p_Clock(0)
        , m_ConfigWriteEnable(0)
        , m_AddressWriteEnable(0)
        , m_Pad0(0)
        , p_ControlBus()
        , p_Address(0)
        , m_Config{ }
        , m_Addresses{ }
        , m_AddressMasks{ }
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

    void SetControlBus(const ControlBus& controlBus) noexcept
    {
        const bool triggerReadAccessChange = p_ControlBus.CSR_Address != controlBus.CSR_Address;

        p_ControlBus = controlBus;

        UpdateConfigWriteEnable();
        UpdateAddressWriteEnable();

        if(triggerReadAccessChange)
        {
            UpdateReadAccess();
        }
    }

    void SetAddress(const u32 address) noexcept
    {
        p_Address = address;
    }
private:
    void UpdateConfigWriteEnable() noexcept
    {
        m_ConfigWriteEnable = 0;

        if(p_ControlBus.CSR_Address >> 2 == Config0 >> 2 && BIT_TO_BOOL(p_ControlBus.CSR_WriteEnable))
        {
            m_ConfigWriteEnable = 1 << (p_ControlBus.CSR_Address & 0b11);
        }
    }

    void UpdateAddressWriteEnable() noexcept
    {
        m_AddressWriteEnable = 0;

        if(p_ControlBus.CSR_Address >> 4 == Address0 >> 4 && BIT_TO_BOOL(p_ControlBus.CSR_WriteEnable))
        {
            m_AddressWriteEnable = 1 << (p_ControlBus.CSR_Address & 0b1111);
        }
    }

    void UpdateReadAccess() const noexcept
    {
        if(((p_ControlBus.CSR_Address >> 5) & 0b111'1111) == ((Config0 >> 5) & 0b111'1111))
        {
            // PMP Configuration CSR
            if(!BIT_TO_BOOL((p_ControlBus.CSR_Address >> 4) & 0x1))
            {
                m_Parent->ReceivePhysicalMemoryProtectionUnit_CSR(m_Index, ConfigReadBack32(p_ControlBus.CSR_Address & 0b11));
            }
            else // PMP Address CSR
            {
                m_Parent->ReceivePhysicalMemoryProtectionUnit_CSR(m_Index, AddressReadBack(p_ControlBus.CSR_Address & 0b1111));
            }
        }
        else
        {
            m_Parent->ReceivePhysicalMemoryProtectionUnit_CSR(m_Index, 0);
        }
    }

    [[nodiscard]] u8 ConfigReadBack(const u32 index) const noexcept
    {
        assert(index < MaxRegionCount);

        // ReSharper disable once CppDFAConstantConditions
        if(index >= RegionCount)
        {
            return 0;
        }

        return ::std::bit_cast<u8>(m_Config[index]);
    }

    [[nodiscard]] u32 ConfigReadBack32(const u32 index) const noexcept
    {
        assert(index < 4);

        u32 ret = 0;

        ret |= ConfigReadBack(index * 4 + 3) << 24;
        ret |= ConfigReadBack(index * 4 + 2) << 16;
        ret |= ConfigReadBack(index * 4 + 1) << 8;
        ret |= ConfigReadBack(index * 4);

        return ret;
    }

    [[nodiscard]] u32 AddressReadBack(const u32 index) const noexcept
    {
        assert(index < RegionCount);

        constexpr u32 Mask = (0x1FFF'FFFF >> GranularityIndex) << GranularityIndex;

        u32 ret = m_Addresses[index] & Mask;

        // Bit G-1 reads as 0 in Top of Range or Off Mode
        if constexpr(RegionCount == 8)
        {
            if constexpr(EnableTopOfRange)
            {
                if((m_Config[index].Mode & 0b10) == 0)
                {
                    ret = SetBit(ret, GranularityIndex, 0);
                }
            }
        }
        else if constexpr(RegionCount > 8)
        {
            // In NAPOT mode bits G-2:0 must read as one.
            if constexpr(EnableNaturallyAligned)
            {
                ret |= (1 << (RegionCount - 2)) - 1;
            }
            if constexpr(EnableTopOfRange)
            {
                // Bit G-1 reads as 0 in Top of Range or Off Mode
                if((m_Config[index].Mode & 0b10) == 0)
                {
                    ret |= (1 << (RegionCount - 1)) - 1;
                }
            }
        }

        return ret;
    }

    [[nodiscard]] u32 AccessAddress() const noexcept
    {
        if(BIT_TO_BOOL(p_ControlBus.LSU_Enable))
        {
            return p_Address;
        }
        else
        {
            return p_ControlBus.PC_Next;
        }
    }

    [[nodiscard]] bool AccessPrivilege() const noexcept
    {
        if(BIT_TO_BOOL(p_ControlBus.LSU_Enable))
        {
            return p_ControlBus.LSU_Privileged;
        }
        else
        {
            return p_ControlBus.CPU_Privileged;
        }
    }

    [[nodiscard]] u32 AddressMaskNAPOT(const u32 index) const noexcept
    {
        assert(index < RegionCount);

        if constexpr(EnableNaturallyAligned)
        {
            u32 ret = 0;
            for(u32 i = GranularityIndex + 1; i < 32; ++i)
            {
                ret = SetBit(ret, i, GetBit(ret, i) | BOOL_TO_BIT(!GetBitBool(m_Addresses[index], i - 3)));
            }

            return ret;
        }
        else
        {
            return 0;
        }
    }

    [[nodiscard]] bool CompareNaturallyAligned(const u32 index) const noexcept
    {
        assert(index < RegionCount);

        if constexpr(EnableNaturallyAligned)
        {
            constexpr u32 Mask0 = (0xFFFFFFFF >> GranularityIndex) << GranularityIndex;
            constexpr u32 Mask1 = (0xFFFFFFFF >> GranularityIndex) << (GranularityIndex - 2);

            const u32 maskedAccessAddress = (AccessAddress() & Mask0) & m_AddressMasks[index];
            const u32 maskedAddress = ((m_Addresses[index] & Mask1) << 2) & m_AddressMasks[index];

            return maskedAccessAddress == maskedAddress;
        }
        else
        {
            return false;
        }
    }

    [[nodiscard]] bool CompareGreaterEqual(const u32 index) const noexcept
    {
        assert(index < RegionCount);

        if(index == 0)
        {
            if constexpr(EnableTopOfRange)
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        if constexpr(EnableTopOfRange)
        {
            constexpr u32 Mask0 = (0xFFFFFFFF >> GranularityIndex) << GranularityIndex;
            constexpr u32 Mask1 = (0xFFFFFFFF >> GranularityIndex) << (GranularityIndex - 2);

            const u32 accessAddress = AccessAddress() & Mask0;
            const u32 address = (m_Addresses[index - 1] & Mask1) << 2;

            return accessAddress >= address;
        }
        else
        {
            return false;
        }
    }

    [[nodiscard]] bool CompareLessThan(const u32 index) const noexcept
    {
        assert(index < RegionCount);

        if(index == 0)
        {
            return false;
        }

        if constexpr(EnableTopOfRange)
        {
            constexpr u32 Mask0 = (0xFFFFFFFF >> GranularityIndex) << GranularityIndex;
            constexpr u32 Mask1 = (0xFFFFFFFF >> GranularityIndex) << (GranularityIndex - 2);

            const u32 accessAddress = AccessAddress() & Mask0;
            const u32 address = (m_Addresses[index] & Mask1) << 2;

            return accessAddress < address;
        }
        else
        {
            return false;
        }
    }

    [[nodiscard]] bool Match(const u32 index) const noexcept
    {
        assert(index < RegionCount);

        if(m_Config[index].Mode == ModeTopOfRange)
        {
            if constexpr(EnableTopOfRange)
            {
                if(index == RegionCount - 1)
                {
                    return CompareGreaterEqual(index) && CompareLessThan(index);
                }
                else
                {
                    // This saves a lot of comparisons.
                    return CompareGreaterEqual(index) && !CompareLessThan(index + 1);
                }
            }
        }
        else if(m_Config[index].Mode == ModeNaturallyAligned4 || m_Config[index].Mode == ModeNaturallyAlignedPowerOf2)
        {
            if constexpr(EnableNaturallyAligned)
            {
                return CompareNaturallyAligned(index);
            }
        }

        return false;
    }

    [[nodiscard]] bool Allow(const u32 index) const noexcept
    {
        assert(index < RegionCount);

        if(!BIT_TO_BOOL(p_ControlBus.LSU_Enable))
        {
            if(AccessPrivilege() == PrivilegeModeMachine)
            {
                // Machine mode always allowed if not locked.
                return BIT_TO_BOOL(m_Config[index].ExecutePermit) || !BIT_TO_BOOL(m_Config[index].LockedEntry);
            }
            else
            {
                return BIT_TO_BOOL(m_Config[index].ExecutePermit);
            }
        }
        else if(!BIT_TO_BOOL(p_ControlBus.LSU_ReadWrite))
        {
            if(AccessPrivilege() == PrivilegeModeMachine)
            {
                // Machine mode always allowed if not locked.
                return BIT_TO_BOOL(m_Config[index].ReadPermit) || !BIT_TO_BOOL(m_Config[index].LockedEntry);
            }
            else
            {
                return BIT_TO_BOOL(m_Config[index].ReadPermit);
            }
        }
        else
        {
            if(AccessPrivilege() == PrivilegeModeMachine)
            {
                // Machine mode always allowed if not locked.
                return BIT_TO_BOOL(m_Config[index].WritePermit) || !BIT_TO_BOOL(m_Config[index].LockedEntry);
            }
            else
            {
                return BIT_TO_BOOL(m_Config[index].WritePermit);
            }
        }
    }

    [[nodiscard]] bool Fail(const u32 index) const noexcept
    {
        assert(index <= RegionCount);

        if(index == RegionCount)
        {
            return AccessPrivilege() != PrivilegeModeMachine;
        }

        if(Match(index))
        {
            return !Allow(index);
        }

        return Fail(index + 1);
    }
private:
    PROCESSES_DECL()
    {
        PROCESS_ENTER(ConfigHandler, p_Reset_n, p_Clock);
        PROCESS_ENTER(AddressHandler, p_Reset_n, p_Clock);
        PROCESS_ENTER(AddressMasking, p_Reset_n, p_Clock);
        PROCESS_ENTER(FaultCheck, p_Reset_n, p_Clock);
    }

    PROCESS_DECL(ConfigHandler)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            (void) ::std::memset(m_Config, 0, sizeof(m_Config));
        }
        else if(RISING_EDGE(p_Clock))
        {
            for(uSys i = 0; i < RegionCount; ++i)
            {
                // If unlocked write access.
                if(BIT_TO_BOOL((m_ConfigWriteEnable >> (i / 4)) & 0x1) && !BIT_TO_BOOL(m_Config[i].LockedEntry))
                {
                    m_Config[i].ReadPermit = (p_ControlBus.CSR_WriteData >> ((i % 4) * 8)) & 0x1;
                    m_Config[i].WritePermit = (p_ControlBus.CSR_WriteData >> ((i % 4) * 8 + 1)) & 0x1;
                    m_Config[i].ExecutePermit = (p_ControlBus.CSR_WriteData >> ((i % 4) * 8 + 2)) & 0x1;

                    const u8 mode = (p_ControlBus.CSR_WriteData >> ((i % 4) * 8 + 3)) & 0b11;
                    if constexpr(!EnableTopOfRange)
                    {
                        if(mode == ModeTopOfRange)
                        {
                            m_Config[i].Mode = ModeOff;
                        }
                    }
                    else if constexpr(!EnableNaturallyAligned)
                    {
                        if(mode == ModeNaturallyAligned4 || mode == ModeNaturallyAlignedPowerOf2)
                        {
                            m_Config[i].Mode = ModeOff;
                        }
                    }
                    else
                    {
                        m_Config[i].Mode = static_cast<Mode>(mode);
                    }

                    m_Config[i].Pad0 = 0;
                    m_Config[i].LockedEntry = (p_ControlBus.CSR_WriteData >> ((i % 4) * 8 + 7)) & 0b1;
                }
            }

            UpdateReadAccess();
        }
    }

    PROCESS_DECL(AddressHandler)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            (void) ::std::memset(m_Addresses, 0, sizeof(m_Addresses));
        }
        else if(RISING_EDGE(p_Clock))
        {
            for(uSys i = 0; i < RegionCount; ++i)
            {
                // If unlocked write access.
                if(BIT_TO_BOOL((m_AddressWriteEnable >> i) & 0x1) && !BIT_TO_BOOL(m_Config[i].LockedEntry))
                {
                    if(i < RegionCount - 1)
                    {
                        if(!BIT_TO_BOOL(m_Config[i + 1].LockedEntry) || m_Config[i].Mode != ModeTopOfRange)
                        {
                            m_Addresses[i] = p_ControlBus.CSR_WriteEnable & 0x1FFF'FFFF;
                        }
                    }
                    else // Last Entry
                    {
                        m_Addresses[i] = p_ControlBus.CSR_WriteEnable & 0x1FFF'FFFF;
                    }

                    m_Config[i].Pad0 = 0;
                    m_Config[i].LockedEntry = (p_ControlBus.CSR_WriteData >> ((i % 4) * 8 + 7)) & 0b1;
                }
            }
            // We don't need to call UpdateReadAccess here because these changes don't affect the CSR.
        }
    }

    PROCESS_DECL(AddressMasking)
    {
        if constexpr(EnableNaturallyAligned)
        {
            if(!BIT_TO_BOOL(p_Reset_n))
            {
                (void) ::std::memset(m_AddressMasks, 0, sizeof(m_AddressMasks));
            }
            else if(RISING_EDGE(p_Clock))
            {
                for(u32 i = 0; i < RegionCount; ++i)
                {
                    if((m_Config[i].Mode & 0b01) == 1)  // NAPOT
                    {
                        m_AddressMasks[i] = AddressMaskNAPOT(i);
                    }
                    else // NA4
                    {
                        constexpr u32 Mask = (0xFFFFFFFF >> GranularityIndex) << GranularityIndex;
                        m_AddressMasks[i] = Mask;
                    }
                }
            }
        }
    }

    PROCESS_DECL(FaultCheck)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_Parent->ReceivePhysicalMemoryProtectionUnit_Fault(m_Index, false);
        }
        else if(RISING_EDGE(p_Clock))
        {
            m_Parent->ReceivePhysicalMemoryProtectionUnit_Fault(m_Index, !BIT_TO_BOOL(p_ControlBus.CPU_Debug) && Fail(0)); // Ignore PMP rules when in debug mode.
        }
    }
private:
    Receiver* m_Parent;
    u32 m_Index;

    u32 p_Reset_n : 1;
    u32 p_Clock : 1;

    u32 m_ConfigWriteEnable : 4;
    u32 m_AddressWriteEnable : 16;
    [[maybe_unused]] u32 m_Pad0 : 10;

    ControlBus p_ControlBus;
    u32 p_Address;

    ConfigData m_Config[RegionCount];
    u32 m_Addresses[RegionCount];
    u32 m_AddressMasks[RegionCount];
};

}
