/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <NumTypes.hpp>

namespace pci {

struct PciConfigHeader final
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

struct PciCapabilityHeader final
{
    u8 CapabilityId;
    u8 NextCapabilityPointer;
};

static_assert(sizeof(PciCapabilityHeader) == 2, "PCI Capability Header is not 2 bytes.");

union PowerManagementCapabilitiesRegister final
{
    u16 Packed;
    struct
    {
        u16 Version : 3;
        u16 PmeClock : 1;
        u16 Reserved : 1;
        u16 DeviceSpecificInitialization : 1;
        u16 AuxCurrent : 3;
        u16 D1Support : 1;
        u16 D2Support : 1;
        u16 PmeSupport : 5;
    };
};

static_assert(sizeof(PowerManagementCapabilitiesRegister) == 2, "Power Management Capability Register is not 2 bytes.");

union PowerManagementControlStatusRegister final
{
    u16 Packed;
    struct
    {
        u16 PowerState : 2;
        u16 Reserved0 : 1;
        u16 NoSoftReset : 1;
        u16 Reserved1 : 4;
        u16 PmeEnable : 1;
        u16 DataSelect : 4;
        u16 DataScale : 2;
        u16 PmeStatus : 1;
    };
};

static_assert(sizeof(PowerManagementControlStatusRegister) == 2, "Power Management Control/Status Register is not 2 bytes.");

struct PowerManagementCapabilityStructure final
{
    PciCapabilityHeader Header;
    PowerManagementCapabilitiesRegister PowerManagementCapabilities;
    PowerManagementControlStatusRegister PowerManagementControlStatusRegister;
    u8 BridgeExtensions;
    u8 Data;
};

static_assert(sizeof(PowerManagementCapabilityStructure) == 8, "PCI Power Management Capability Structure is not 8 bytes.");

union MessageSignalledInterruptControlRegister final
{
    u16 Packed;
    struct
    {
        u16 Enabled : 1;
        u16 MultipleMessageCapable : 3;
        u16 MultipleMessageEnabled : 3;
        u16 Capable64Bit : 1;
        u16 PerVectorMasking : 1;
        u16 Reserved : 7;
    };
};

static_assert(sizeof(MessageSignalledInterruptControlRegister) == 2, "Message Signalled Interrupt Control Register is not 2 bytes.");

struct MessageSignalledInterruptCapabilityStructure final
{
    PciCapabilityHeader Header;
    MessageSignalledInterruptControlRegister MessageControl;
    u32 MessageAddress;
    u32 MessageUpperAddress;
    u16 MessageData;
    u16 Reserved;
    u32 MaskBits;
    u32 PendingBits;
};

static_assert(sizeof(MessageSignalledInterruptCapabilityStructure) == 0x18, "Message Signalled Interrupt Capability Structure is not 24 bytes.");

struct MessageSignalledInterruptXCapabilityStructure final
{
    PciCapabilityHeader Header;
    u16 MessageControl;
    u32 MessageUpperAddress;
    u32 TableOffset : 29;
    u32 BIR : 3;
};

static_assert(sizeof(MessageSignalledInterruptXCapabilityStructure) == 0x0C, "Message Signalled Interrupt X Capability Structure is not 12 bytes.");

}
