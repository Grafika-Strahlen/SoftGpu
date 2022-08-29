#pragma once

#include <NumTypes.hpp>
#include <Objects.hpp>

#include <cstring>

struct PciConfigHeader
{
    u16 VendorID;
    u16 DeviceID;
    u16 Command;
    u16 Status;
    u32 RevisionID : 8;
    u32 ClassCode : 24;
    u8 CacheLineSize;
    u8 MasterLatencyTimer;
    u8 HeaderType;
    u8 BIST;
    u32 BAR0;
    u32 BAR1;
    u32 BAR2;
    u32 BAR3;
    u32 BAR4;
    u32 BAR5;
    u32 CardBusCISPointer;
    u16 SubsystemVendorID;
    u16 SubsystemID;
    u32 ExpansionROMBaseAddress;
    u32 CapPointer : 8;
    u32 Reserved0 : 24;
    u32 Reserved1;
    u8 InterruptLine;
    u8 InterruptPin;
    u8 MinGnt;
    u8 MaxLat;
};

static_assert(sizeof(PciConfigHeader) == 64, "PCI Config Header is not 64 bytes.");

class PciController final
{
    DEFAULT_DESTRUCT(PciController);
    DELETE_CM(PciController);
public:
    static inline constexpr u32 BAR0_MASK_BITS = 0xFF000000;
    static inline constexpr u32 BAR1_MASK_BITS = 0x80000000;
    static inline constexpr u32 BAR2_MASK_BITS = 0xFFFFFFFF;
    static inline constexpr u32 BAR3_MASK_BITS = 0xFFFFFFFF;
    static inline constexpr u32 BAR4_MASK_BITS = 0xFFFFFFFF;
    static inline constexpr u32 BAR5_MASK_BITS = 0xFFFFFFFF;
public:
    PciController() noexcept
        : m_PciConfig{ 0 }
        , m_PciExtendedConfig{ 0 }
    {
        m_ConfigHeader.VendorID = 0xFFFD;
        m_ConfigHeader.DeviceID = 0x0001;
        m_ConfigHeader.Command = 0x0000;
        m_ConfigHeader.Status = 0x0010;
        m_ConfigHeader.RevisionID = 0x01;
        m_ConfigHeader.ClassCode = 0x030001;
        m_ConfigHeader.CacheLineSize = 0x0;
        m_ConfigHeader.MasterLatencyTimer = 0x0;
        m_ConfigHeader.HeaderType = 0x00;
        m_ConfigHeader.BIST = 0x00;
        // Memory, 32 bit, Not Prefetchable.
        m_ConfigHeader.BAR0 = 0x00000000;
        // Memory, 64 bit, Prefetchable.
        m_ConfigHeader.BAR1 = 0x00001100;
        // Part of BAR1
        m_ConfigHeader.BAR2 = 0x00000000;
        // Unused
        m_ConfigHeader.BAR3 = 0x00000000;
        // Unused
        m_ConfigHeader.BAR4 = 0x00000000;
        // Unused
        m_ConfigHeader.BAR5 = 0x00000000;
        m_ConfigHeader.CardBusCISPointer = 0x0;
        m_ConfigHeader.SubsystemVendorID = 0x0;
        m_ConfigHeader.SubsystemID = 0x0;
        m_ConfigHeader.ExpansionROMBaseAddress = 0x0;
        m_ConfigHeader.CapPointer = 0x40;
        m_ConfigHeader.Reserved0 = 0x0;
        m_ConfigHeader.Reserved1 = 0x0;
        m_ConfigHeader.InterruptLine = 0x0;
        m_ConfigHeader.InterruptPin = 0x0;
        m_ConfigHeader.MinGnt = 0x00;
        m_ConfigHeader.MaxLat = 0x00;
    }

    [[nodiscard]] u32 ConfigRead(const u16 address, const u8 size) noexcept
    {
        if(size != 1 && size != 2 && size != 4)
        {
            return 0;
        }

        if(address < 64)
        {
            if(address > 60 && size == 4)
            {
                return 0;
            }

            if(address > 62 && size == 2)
            {
                return 0;
            }

            u32 ret;
            (void) ::std::memcpy(&ret, reinterpret_cast<u8*>(&m_ConfigHeader) + address, size);
            return ret;
        }

        if(address < 256)
        {
            if(address > 252 && size == 4)
            {
                return 0;
            }

            if(address > 254 && size == 2)
            {
                return 0;
            }

            u32 ret;
            (void) ::std::memcpy(&ret, &m_PciConfig[address], size);
            return ret;
        }

        if(address < 4096)
        {
            if(address > 4092 && size == 4)
            {
                return 0;
            }

            if(address > 4094 && size == 2)
            {
                return 0;
            }

            u32 ret;
            (void) ::std::memcpy(&ret, &m_PciExtendedConfig[address], size);
            return ret;
        }

        return 0;
    }

    void ConfigWrite(const u16 address, const u32 size, const u32 value) noexcept
    {
        switch(address)
        {
            case 4:
                if(size != 2)
                {
                    break;
                }
                // Mask the command to only the RW bits.
                m_ConfigHeader.Command = static_cast<u16>(value & 0x0A44);
                break;
            case 6:
                if(size != 2)
                {
                    break;
                }
                // Mask the command to only the RW bits.
                m_ConfigHeader.Command = static_cast<u16>(value & 0xFB00);
                break;
            case 0xC:
                if(size != 1)
                {
                    break;
                }
                m_ConfigHeader.Command = static_cast<u8>(value);
                break;
            case 0x10:
                if(size != 4)
                {
                    break;
                }
                m_ConfigHeader.BAR0 = value & BAR0_MASK_BITS;
                break;
            case 0x14:
                if(size != 4)
                {
                    break;
                }
                m_ConfigHeader.BAR1 = value & BAR1_MASK_BITS;
                break;
            case 0x18:
                if(size != 4)
                {
                    break;
                }
                m_ConfigHeader.BAR2 = value & BAR2_MASK_BITS;
                break;
            case 0x1C:
                if(size != 4)
                {
                    break;
                }
                m_ConfigHeader.BAR3 = value & BAR3_MASK_BITS;
                break;
            case 0x20:
                if(size != 4)
                {
                    break;
                }
                m_ConfigHeader.BAR4 = value & BAR4_MASK_BITS;
                break;
            case 0x24:
                if(size != 4)
                {
                    break;
                }
                m_ConfigHeader.BAR5 = value & BAR5_MASK_BITS;
                break;
            case 0x3C:
                if(size != 1)
                {
                    break;
                }
                m_ConfigHeader.InterruptLine = static_cast<u8>(value);
                break;
            default: break;
        }
    }

    [[nodiscard]] u8 GetBARFromAddress(const u64 address) noexcept
    {
        if(address < 0xFFFFFFFF)
        {
            if(address >= (m_ConfigHeader.BAR0 & BAR0_MASK_BITS) && address < ((m_ConfigHeader.BAR0 & BAR0_MASK_BITS) + 16 * 1024 * 1024))
            {
                return 0;
            }
        }

        const u64 bar1 = (static_cast<u64>(m_ConfigHeader.BAR2 & BAR2_MASK_BITS) << 32) | (m_ConfigHeader.BAR1 & BAR1_MASK_BITS);

        if(address >= bar1 && address < bar1 + (2ull * 1024ull * 1024ull * 1024ull))
        {
            return 1;
        }

        return 0xFF;
    }

    [[nodiscard]] u64 GetBAROffset(const u64 address, const u8 bar) noexcept
    {
        if(bar == 0)
        {
            return address - (m_ConfigHeader.BAR0 & BAR0_MASK_BITS);
        }

        if(bar == 1)
        {
            const u64 bar1 = (static_cast<u64>(m_ConfigHeader.BAR2 & BAR2_MASK_BITS) << 32) | (m_ConfigHeader.BAR1 & BAR1_MASK_BITS);

            return address - bar1;
        }

        return address;
    }
private:
    PciConfigHeader m_ConfigHeader;
    u8 m_PciConfig[256 - 64];
    u8 m_PciExtendedConfig[4096 - 256];
};
