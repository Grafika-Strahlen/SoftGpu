/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

// #include "PCIController.hpp"

inline u32 PciController::ConfigRead(
    const u16 address,
    const u8 size
) noexcept
{
    if(size != 1 && size != 2 && size != 4)
    {
        return 0;
    }

    if(address < PciConfigOffsets::ConfigHeaderEnd)
    {
        if(address > (PciConfigOffsets::ConfigHeaderEnd - 4) && size == 4)
        {
            return 0;
        }

        if(address > (PciConfigOffsets::ConfigHeaderEnd - 2) && size == 2)
        {
            return 0;
        }

        u32 ret;
        (void) ::std::memcpy(&ret, reinterpret_cast<u8*>(&m_ConfigData.ConfigHeader) + address, size);
        return ret;
    }

    if(address < PciConfigOffsets::PciCapabilityOffsetEnd)
    {
        if(address > (PciConfigOffsets::PciCapabilityOffsetEnd - 4) && size == 4)
        {
            return 0;
        }

        if(address > (PciConfigOffsets::PciCapabilityOffsetEnd - 2) && size == 2)
        {
            return 0;
        }

        u32 ret;
        (void) ::std::memcpy(&ret, reinterpret_cast<u8*>(&m_ConfigData.PcieCapability) + (address - PciConfigOffsets::PciCapabilityOffsetBegin), size);
        return ret;
    }

    if(address < PciConfigOffsets::PowerManagementCapabilityOffsetEnd)
    {
        if(address > (PciConfigOffsets::PowerManagementCapabilityOffsetEnd - 4) && size == 4)
        {
            return 0;
        }

        if(address > (PciConfigOffsets::PowerManagementCapabilityOffsetEnd - 2) && size == 2)
        {
            return 0;
        }

        u32 ret;
        (void) ::std::memcpy(&ret, reinterpret_cast<u8*>(&m_ConfigData.PowerManagementCapability) + (address - PciConfigOffsets::PowerManagementCapabilityOffsetBegin), size);
        return ret;
    }

    if(address < PciConfigOffsets::MessageSignalledInterruptsCapabilityOffsetEnd)
    {
        if(address > (PciConfigOffsets::MessageSignalledInterruptsCapabilityOffsetEnd - 4) && size == 4)
        {
            return 0;
        }

        if(address > (PciConfigOffsets::MessageSignalledInterruptsCapabilityOffsetEnd - 2) && size == 2)
        {
            return 0;
        }

        u32 ret;
        (void) ::std::memcpy(&ret, reinterpret_cast<u8*>(&m_ConfigData.MessageSignalledInterruptCapability) + (address - PciConfigOffsets::MessageSignalledInterruptsCapabilityOffsetBegin), size);
        return ret;
    }

    if(address < PciConfigOffsets::PciConfigOffsetEnd)
    {
        if(address > (PciConfigOffsets::PciConfigOffsetEnd - 4) && size == 4)
        {
            return 0;
        }

        if(address > (PciConfigOffsets::PciConfigOffsetEnd - 2) && size == 2)
        {
            return 0;
        }

        u32 ret;
        (void) ::std::memcpy(&ret, &m_ConfigData.PciConfig[address - PciConfigOffsets::PciConfigOffsetBegin], size);
        return ret;
    }

    if(address < PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetEnd)
    {
        if(address > (PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetEnd - 4) && size == 4)
        {
            return 0;
        }

        if(address > (PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetEnd - 2) && size == 2)
        {
            return 0;
        }

        u32 ret;
        (void) ::std::memcpy(&ret, reinterpret_cast<u8*>(&m_ConfigData.AdvancedErrorReportingCapability) + (address - PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin), size);
        return ret;
    }

    if(address < PciConfigOffsets::PciExtendedConfigOffsetEnd)
    {
        if(address > (PciConfigOffsets::PciExtendedConfigOffsetEnd - 4) && size == 4)
        {
            return 0;
        }

        if(address > (PciConfigOffsets::PciExtendedConfigOffsetEnd - 2) && size == 2)
        {
            return 0;
        }

        u32 ret;
        (void) ::std::memcpy(&ret, &m_ConfigData.PciExtendedConfig[address - PciConfigOffsets::PciExtendedConfigOffsetBegin], size);
        return ret;
    }

    return 0;
}

inline void PciController::ConfigWrite(
    const u16 address,
    const u32 size,
    const u32 value
) noexcept
{
    switch(address)
    {
        case 4:
            if(size != 2)
            {
                break;
            }
            // Mask the command to only the RW bits.
            m_ConfigData.ConfigHeader.Command = static_cast<u16>(value & COMMAND_REGISTER_MASK_BITS);
            break;
        case 6:
            if(size != 2)
            {
                break;
            }
            // Mask the status to only the RW bits.
            m_ConfigData.ConfigHeader.Status = static_cast<u16>(value & 0xFB00);
            break;
        case 0xC:
            if(size != 1)
            {
                break;
            }
            m_ConfigData.ConfigHeader.CacheLineSize = static_cast<u8>(value);
            break;
        case 0x10:
            if(size != 4)
            {
                break;
            }
            m_ConfigData.ConfigHeader.BAR0 = (value & BAR0_MASK_BITS) | BAR0_READ_ONLY_BITS;
            break;
        case 0x14:
            if(size != 4)
            {
                break;
            }
            m_ConfigData.ConfigHeader.BAR1 = (value & BAR1_MASK_BITS) | BAR1_READ_ONLY_BITS;
            break;
        case 0x18:
            if(size != 4)
            {
                break;
            }
            m_ConfigData.ConfigHeader.BAR2 = (value & BAR2_MASK_BITS) | BAR2_READ_ONLY_BITS;
            break;
        case 0x1C:
            if(size != 4)
            {
                break;
            }
            m_ConfigData.ConfigHeader.BAR3 = (value & BAR3_MASK_BITS) | BAR3_READ_ONLY_BITS;
            break;
        case 0x20:
            if(size != 4)
            {
                break;
            }
            m_ConfigData.ConfigHeader.BAR4 = (value & BAR4_MASK_BITS) | BAR4_READ_ONLY_BITS;
            break;
        case 0x24:
            if(size != 4)
            {
                break;
            }
            m_ConfigData.ConfigHeader.BAR5 = (value & BAR5_MASK_BITS) | BAR5_READ_ONLY_BITS;
            break;
        case 0x30:
            if(size != 4)
            {
                break;
            }
            m_ConfigData.ConfigHeader.ExpansionROMBaseAddress = (value & EXPANSION_ROM_BAR_MASK_BITS) | EXPANSION_ROM_BAR_READ_ONLY_BITS;
            break;
        case 0x3C:
            if(size != 1)
            {
                break;
            }
            m_ConfigData.ConfigHeader.InterruptLine = static_cast<u8>(value);
            break;
        case offsetof(PciConfigData, PcieCapability) + offsetof(pcie::PcieCapabilityStructure, DeviceControl):
            if(size != 2)
            {
                break;
            }
            m_ConfigData.PcieCapability.DeviceControl.Packed = (value & DEVICE_CONTROL_REGISTER_MASK_BITS) | DEVICE_CONTROL_REGISTER_READ_ONLY_BITS;
            break;
        case offsetof(PciConfigData, PcieCapability) + offsetof(pcie::PcieCapabilityStructure, LinkControl):
            if(size != 2)
            {
                break;
            }
            m_ConfigData.PcieCapability.LinkControl.Packed = (value & LINK_CONTROL_REGISTER_MASK_BITS) | LINK_CONTROL_REGISTER_READ_ONLY_BITS;
            break;
        case offsetof(PciConfigData, PowerManagementCapability) + offsetof(pci::PowerManagementCapabilityStructure, PowerManagementControlStatusRegister):
            if(size != 2)
            {
                break;
            }
            m_ConfigData.PowerManagementCapability.PowerManagementControlStatusRegister.Packed = (value & PM_CONTROL_REGISTER_MASK_BITS) | PM_CONTROL_REGISTER_READ_ONLY_BITS;
            break;
        case offsetof(PciConfigData, MessageSignalledInterruptCapability) + offsetof(pci::MessageSignalledInterruptCapabilityStructure, MessageControl):
            if(size != 2)
            {
                break;
            }
            m_ConfigData.MessageSignalledInterruptCapability.MessageControl.Packed = (value & MESSAGE_CONTROL_REGISTER_MASK_BITS) | MESSAGE_CONTROL_REGISTER_READ_ONLY_BITS;
            break;
        case offsetof(PciConfigData, MessageSignalledInterruptCapability) + offsetof(pci::MessageSignalledInterruptCapabilityStructure, MessageAddress):
            if(size != 4)
            {
                break;
            }
            m_ConfigData.MessageSignalledInterruptCapability.MessageAddress = (value & MESSAGE_ADDRESS_REGISTER_MASK_BITS) | MESSAGE_ADDRESS_REGISTER_READ_ONLY_BITS;
            break;
        case offsetof(PciConfigData, MessageSignalledInterruptCapability) + offsetof(pci::MessageSignalledInterruptCapabilityStructure, MessageUpperAddress):
            if(size != 4)
            {
                break;
            }
            m_ConfigData.MessageSignalledInterruptCapability.MessageUpperAddress = value;
            break;
        case offsetof(PciConfigData, MessageSignalledInterruptCapability) + offsetof(pci::MessageSignalledInterruptCapabilityStructure, MessageData):
            if(size != 2)
            {
                break;
            }
            m_ConfigData.MessageSignalledInterruptCapability.MessageData = static_cast<u16>(value);
            break;
        case offsetof(PciConfigData, MessageSignalledInterruptCapability) + offsetof(pci::MessageSignalledInterruptCapabilityStructure, MaskBits):
            if(size != 4)
            {
                break;
            }
            m_ConfigData.MessageSignalledInterruptCapability.MaskBits = value;
            break;
        default: break;
    }
}

inline void PciController::InitConfigHeader() noexcept
{
    m_ConfigData.ConfigHeader.VendorID = 0xFFFD;
    m_ConfigData.ConfigHeader.DeviceID = 0x0001;
    m_ConfigData.ConfigHeader.Command = 0x0000;
    m_ConfigData.ConfigHeader.Status = 0x0010;
    m_ConfigData.ConfigHeader.RevisionID = 0x01;
    m_ConfigData.ConfigHeader.ClassCode = 0x030001;
    m_ConfigData.ConfigHeader.CacheLineSize = 0x0;
    m_ConfigData.ConfigHeader.MasterLatencyTimer = 0x0;
    m_ConfigData.ConfigHeader.HeaderType = 0x00;
    m_ConfigData.ConfigHeader.BIST = 0x00;
    // Memory, 32 bit, Not Prefetchable.
    m_ConfigData.ConfigHeader.BAR0 = 0x00000000;
    // Memory, 64 bit, Prefetchable.
    m_ConfigData.ConfigHeader.BAR1 = 0x0000000C;
    // Part of BAR1
    m_ConfigData.ConfigHeader.BAR2 = 0x00000000;
    // Unused
    m_ConfigData.ConfigHeader.BAR3 = 0x00000000;
    // Unused
    m_ConfigData.ConfigHeader.BAR4 = 0x00000000;
    // Unused
    m_ConfigData.ConfigHeader.BAR5 = 0x00000000;
    m_ConfigData.ConfigHeader.CardBusCISPointer = 0x0;
    m_ConfigData.ConfigHeader.SubsystemVendorID = 0x0;
    m_ConfigData.ConfigHeader.SubsystemID = 0x0;
    // 32KiB, Not Enabled
    m_ConfigData.ConfigHeader.ExpansionROMBaseAddress = 0x00000000;
    // m_ConfigHeader.CapPointer = 0;
    m_ConfigData.ConfigHeader.CapPointer = offsetof(PciConfigData, PcieCapability);
    m_ConfigData.ConfigHeader.Reserved0 = 0x0;
    m_ConfigData.ConfigHeader.Reserved1 = 0x0;
    m_ConfigData.ConfigHeader.InterruptLine = 0x0;
    m_ConfigData.ConfigHeader.InterruptPin = 0x0;
    m_ConfigData.ConfigHeader.MinGnt = 0x00;
    m_ConfigData.ConfigHeader.MaxLat = 0x00;
}

inline void PciController::InitPcieCapabilityStructure() noexcept
{
    // PCI Express Capability ID
    m_ConfigData.PcieCapability.Header.CapabilityId = 0x10;
    m_ConfigData.PcieCapability.Header.NextCapabilityPointer = offsetof(PciConfigData, PowerManagementCapability);
    m_ConfigData.PcieCapability.CapabilitiesRegister.CapabilityVersion = 0x01;
    // Legacy PCI Express Endpoint device, this is what my 3070 Ti reports, and as what makes the most sense based on its description.
    m_ConfigData.PcieCapability.CapabilitiesRegister.DeviceType = 0b0001;
    m_ConfigData.PcieCapability.CapabilitiesRegister.SlotImplemented = 0b0;
    m_ConfigData.PcieCapability.CapabilitiesRegister.InterruptMessageNumber = 0b00000;
    // 256 Bytes
    m_ConfigData.PcieCapability.DeviceCapabilities.MaxPayloadSizeSupported = 0b001;
    m_ConfigData.PcieCapability.DeviceCapabilities.PhantomFunctionsSupported = 0b00;
    m_ConfigData.PcieCapability.DeviceCapabilities.ExtendedTagFieldSupported = 0b1;
    // No limit
    m_ConfigData.PcieCapability.DeviceCapabilities.EndpointL0sAcceptableLatency = 0b111;
    // Maximum of 64 us
    m_ConfigData.PcieCapability.DeviceCapabilities.EndpointL1AcceptableLatency = 0b110;
    m_ConfigData.PcieCapability.DeviceCapabilities.Undefined = 0b000;
    m_ConfigData.PcieCapability.DeviceCapabilities.RoleBasedErrorReporting = 0b1;
    m_ConfigData.PcieCapability.DeviceCapabilities.ReservedP0 = 0b00;
    m_ConfigData.PcieCapability.DeviceCapabilities.CapturedSlotPowerLimitValue = 0x00;
    // 1.0x
    m_ConfigData.PcieCapability.DeviceCapabilities.CapturedSlotPowerLimitScale = 0b00;
    m_ConfigData.PcieCapability.DeviceCapabilities.ReservedP1 = 0x0;

    m_ConfigData.PcieCapability.DeviceControl.CorrectableErrorReportingEnable = 0b0;
    m_ConfigData.PcieCapability.DeviceControl.NonFatalErrorReportingEnable = 0b0;
    m_ConfigData.PcieCapability.DeviceControl.UnsupportedRequestReportingEnable = 0b0;
    m_ConfigData.PcieCapability.DeviceControl.EnableRelaxedOrdering = 0b1;
    m_ConfigData.PcieCapability.DeviceControl.MaxPayloadSize = 0b000;
    m_ConfigData.PcieCapability.DeviceControl.ExtendedTagFieldEnable = 0b0;
    m_ConfigData.PcieCapability.DeviceControl.PhantomFunctionsEnable = 0b0;
    m_ConfigData.PcieCapability.DeviceControl.AuxPowerPmEnable = 0b0;
    m_ConfigData.PcieCapability.DeviceControl.EnableSnoopNotRequired = 0b1;
    // 512 bytes. This is the defined default.
    m_ConfigData.PcieCapability.DeviceControl.MaxReadRequestSize = 0b010;
    m_ConfigData.PcieCapability.DeviceControl.Reserved = 0b0;

    m_ConfigData.PcieCapability.DeviceStatus.CorrectableErrorDetected = 0b0;
    m_ConfigData.PcieCapability.DeviceStatus.NonFatalErrorDetected = 0b0;
    m_ConfigData.PcieCapability.DeviceStatus.FatalErrorDetected = 0b0;
    m_ConfigData.PcieCapability.DeviceStatus.UnsupportedRequestDetected = 0b0;
    m_ConfigData.PcieCapability.DeviceStatus.TransactionsPending = 0b0;

    m_ConfigData.PcieCapability.LinkCapabilities.MaximumLinkSpeed = 0b0001;
    m_ConfigData.PcieCapability.LinkCapabilities.MaximumLinkWidth = 0b001000;
    m_ConfigData.PcieCapability.LinkCapabilities.ASPMSupport = 0b01;
    m_ConfigData.PcieCapability.LinkCapabilities.L0sExitLatency = 0b100;
    m_ConfigData.PcieCapability.LinkCapabilities.L1ExitLatency = 0b010;
    m_ConfigData.PcieCapability.LinkCapabilities.ClockPowerManagement = 0b1;
    m_ConfigData.PcieCapability.LinkCapabilities.SurpriseDownErrorReportingCapable = 0b0;
    m_ConfigData.PcieCapability.LinkCapabilities.DataLinkLayerActiveReportingCapable = 0b0;
    m_ConfigData.PcieCapability.LinkCapabilities.Reserved = 0x0;
    m_ConfigData.PcieCapability.LinkCapabilities.PortNumber = 0x00;

    m_ConfigData.PcieCapability.LinkControl.ASPMControl = 0b00;
    m_ConfigData.PcieCapability.LinkControl.ReservedP0 = 0b0;
    m_ConfigData.PcieCapability.LinkControl.ReadCompletionBoundary = 0b0;
    m_ConfigData.PcieCapability.LinkControl.LinkDisable = 0b0;
    m_ConfigData.PcieCapability.LinkControl.RetrainLink = 0b0;
    m_ConfigData.PcieCapability.LinkControl.CommonClockConfiguration = 0b0;
    m_ConfigData.PcieCapability.LinkControl.ExtendedSynch = 0b0;
    m_ConfigData.PcieCapability.LinkControl.EnableClockPowerManagement = 0b0;
    m_ConfigData.PcieCapability.LinkControl.ReservedP1 = 0b0000000;

    m_ConfigData.PcieCapability.LinkStatus.LinkSpeed = 0b0001;
    m_ConfigData.PcieCapability.LinkStatus.NegotiatedLinkWidth = 0b001000;
    m_ConfigData.PcieCapability.LinkStatus.Undefined = 0b0;
    m_ConfigData.PcieCapability.LinkStatus.LinkTraining = 0b0;
    m_ConfigData.PcieCapability.LinkStatus.SlotClockConfiguration = 0b0;
    m_ConfigData.PcieCapability.LinkStatus.DataLinkLayerActive = 0b0;
    m_ConfigData.PcieCapability.LinkStatus.ReservedZ = 0x0;
}

inline void PciController::InitPowerManagementCapabilityStructure() noexcept
{
    // PCI Power Management Capability ID
    m_ConfigData.PowerManagementCapability.Header.CapabilityId = 0x01;
    m_ConfigData.PowerManagementCapability.Header.NextCapabilityPointer = offsetof(PciConfigData, MessageSignalledInterruptCapability);
    // The defined default version in PCI Bus Power Management Interface Specification Rev 1.2
    m_ConfigData.PowerManagementCapability.PowerManagementCapabilities.Version = 0b011;
    // PCI Express Base 1.1 requires this to be hardwired to 0
    m_ConfigData.PowerManagementCapability.PowerManagementCapabilities.PmeClock = 0b0;
    m_ConfigData.PowerManagementCapability.PowerManagementCapabilities.Reserved = 0b0;
    m_ConfigData.PowerManagementCapability.PowerManagementCapabilities.DeviceSpecificInitialization = 0b0;
    m_ConfigData.PowerManagementCapability.PowerManagementCapabilities.AuxCurrent = 0b000;
    m_ConfigData.PowerManagementCapability.PowerManagementCapabilities.D1Support = 0b0;
    m_ConfigData.PowerManagementCapability.PowerManagementCapabilities.D2Support = 0b0;
    m_ConfigData.PowerManagementCapability.PowerManagementCapabilities.PmeSupport = 0b00000;

    m_ConfigData.PowerManagementCapability.PowerManagementControlStatusRegister.PowerState = 0b00;
    m_ConfigData.PowerManagementCapability.PowerManagementControlStatusRegister.Reserved0 = 0b0;
    m_ConfigData.PowerManagementCapability.PowerManagementControlStatusRegister.NoSoftReset = 0b0;
    m_ConfigData.PowerManagementCapability.PowerManagementControlStatusRegister.Reserved1 = 0x0;
    m_ConfigData.PowerManagementCapability.PowerManagementControlStatusRegister.PmeEnable = 0b0;
    m_ConfigData.PowerManagementCapability.PowerManagementControlStatusRegister.DataSelect = 0x0;
    m_ConfigData.PowerManagementCapability.PowerManagementControlStatusRegister.DataScale = 0b00;
    m_ConfigData.PowerManagementCapability.PowerManagementControlStatusRegister.PmeStatus = 0b0;
}

inline void PciController::InitMessageSignalledInterruptCapabilityStructure() noexcept
{
    // The defined ID for the MSI Capability in the PCI Local Bus 3.0 spec.
    m_ConfigData.MessageSignalledInterruptCapability.Header.CapabilityId = 0x0005;
    m_ConfigData.MessageSignalledInterruptCapability.Header.NextCapabilityPointer = 0x0;

    m_ConfigData.MessageSignalledInterruptCapability.MessageControl.Packed = MESSAGE_CONTROL_REGISTER_READ_ONLY_BITS;
    m_ConfigData.MessageSignalledInterruptCapability.MessageAddress = 0x00000000;
    m_ConfigData.MessageSignalledInterruptCapability.MessageUpperAddress = 0x00000000;
    m_ConfigData.MessageSignalledInterruptCapability.MessageData = 0x0000;
    m_ConfigData.MessageSignalledInterruptCapability.Reserved = 0x0000;
    m_ConfigData.MessageSignalledInterruptCapability.MaskBits = 0x00000000;
    m_ConfigData.MessageSignalledInterruptCapability.PendingBits = 0x00000000;
}

inline void PciController::InitAdvancedErrorReportingCapabilityStructure() noexcept
{
    // The defined ID for the Advanced Error Reporting Capability in the PCI Express Base 1.1 spec.
    m_ConfigData.AdvancedErrorReportingCapability.Header.CapabilityId = 0x0001;
    m_ConfigData.AdvancedErrorReportingCapability.Header.CapabilityVersion = 0x1;
    m_ConfigData.AdvancedErrorReportingCapability.Header.NextCapabilityPointer = 0x0;

    m_ConfigData.AdvancedErrorReportingCapability.UncorrectableErrorStatusRegister = 0x00000000;
    m_ConfigData.AdvancedErrorReportingCapability.UncorrectableErrorMaskRegister = 0x00000000;
    m_ConfigData.AdvancedErrorReportingCapability.UncorrectableErrorSeverityRegister = 0x00062030;
    m_ConfigData.AdvancedErrorReportingCapability.CorrectableErrorStatusRegister = 0x00000000;
    m_ConfigData.AdvancedErrorReportingCapability.CorrectableErrorMaskRegister = 0x00002000;
    m_ConfigData.AdvancedErrorReportingCapability.AdvancedCapabilitiesAndControlRegister = 0x00000000;
    m_ConfigData.AdvancedErrorReportingCapability.HeaderLogRegister[0] = 0;
    m_ConfigData.AdvancedErrorReportingCapability.HeaderLogRegister[1] = 0;
    m_ConfigData.AdvancedErrorReportingCapability.HeaderLogRegister[2] = 0;
    m_ConfigData.AdvancedErrorReportingCapability.HeaderLogRegister[3] = 0;
}
