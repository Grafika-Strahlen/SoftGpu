/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <NumTypes.hpp>

#include "PciProtocol.hpp"

namespace pcie {

#pragma pack(push, 1)

struct PciExtendedCapabilityHeader final
{
    u16 CapabilityId;
    u16 CapabilityVersion : 4;
    u16 NextCapabilityPointer : 12;
};

static_assert(sizeof(PciExtendedCapabilityHeader) == 4, "PCIe Extended Capability Header is not 2 bytes.");

struct PcieCapabilitiesRegister final
{
    u16 CapabilityVersion : 4;
    u16 DeviceType : 4;
    u16 SlotImplemented : 1;
    u16 InterruptMessageNumber : 5;
    u16 ReservedP : 2;
};

static_assert(sizeof(PcieCapabilitiesRegister) == 2, "PCIe Capabilities Register is not 2 bytes.");

struct DeviceCapabilitiesRegister final
{
    u32 MaxPayloadSizeSupported : 3;
    u32 PhantomFunctionsSupported : 2;
    u32 ExtendedTagFieldSupported : 1;
    u32 EndpointL0sAcceptableLatency : 3;
    u32 EndpointL1AcceptableLatency : 3;
    u32 Undefined : 3;
    u32 RoleBasedErrorReporting : 1;
    u32 ReservedP0 : 2;
    u32 CapturedSlotPowerLimitValue : 8;
    u32 CapturedSlotPowerLimitScale : 2;
    u32 ReservedP1 : 4;
};

static_assert(sizeof(DeviceCapabilitiesRegister) == 4, "Device Capabilities Register is not 4 bytes.");

union DeviceControlRegister final
{
    u16 Packed;
    struct
    {
        u16 CorrectableErrorReportingEnable : 1;
        u16 NonFatalErrorReportingEnable : 1;
        u16 FatalErrorReportingEnable : 1;
        u16 UnsupportedRequestReportingEnable : 1;
        u16 EnableRelaxedOrdering : 1;
        u16 MaxPayloadSize : 3;
        u16 ExtendedTagFieldEnable : 1;
        u16 PhantomFunctionsEnable : 1;
        u16 AuxPowerPmEnable : 1;
        u16 EnableSnoopNotRequired : 1;
        u16 MaxReadRequestSize : 3;
        u16 Reserved : 1;
    };
};

static_assert(sizeof(DeviceControlRegister) == 2, "Device Control Register is not 2 bytes.");

struct DeviceStatusRegister final
{
    u16 CorrectableErrorDetected : 1;
    u16 NonFatalErrorDetected : 1;
    u16 FatalErrorDetected : 1;
    u16 UnsupportedRequestDetected : 1;
    u16 AuxPowerDetected : 1;
    u16 TransactionsPending : 1;
    u16 ReservedZ : 10;
};

static_assert(sizeof(DeviceStatusRegister) == 2, "Device Status Register is not 2 bytes.");

struct LinkCapabilitiesRegister final
{
    u32 MaximumLinkSpeed : 4;
    u32 MaximumLinkWidth : 6;
    u32 ASPMSupport : 2;
    u32 L0sExitLatency : 3;
    u32 L1ExitLatency : 3;
    u32 ClockPowerManagement : 1;
    u32 SurpriseDownErrorReportingCapable : 1;
    u32 DataLinkLayerActiveReportingCapable : 1;
    u32 Reserved : 3;
    u32 PortNumber : 8;
};

static_assert(sizeof(LinkCapabilitiesRegister) == 4, "Link Capabilities Register is not 4 bytes.");

union LinkControlRegister final
{
    u16 Packed;
    struct
    {
        u16 ASPMControl : 2;
        u16 ReservedP0 : 1;
        u16 ReadCompletionBoundary : 1;
        u16 LinkDisable : 1;
        u16 RetrainLink : 1;
        u16 CommonClockConfiguration : 1;
        u16 ExtendedSynch : 1;
        u16 EnableClockPowerManagement : 1;
        u16 ReservedP1 : 7;
    };
};

static_assert(sizeof(LinkControlRegister) == 2, "Link Control Register is not 2 bytes.");

struct LinkStatusRegister final
{
    u16 LinkSpeed : 4;
    u16 NegotiatedLinkWidth : 6;
    u16 Undefined : 1;
    u16 LinkTraining : 1;
    u16 SlotClockConfiguration : 1;
    u16 DataLinkLayerActive : 1;
    u16 ReservedZ : 2;
};

static_assert(sizeof(LinkStatusRegister) == 2, "Link Status Register is not 2 bytes.");

struct PcieCapabilityStructure final
{
    pci::PciCapabilityHeader Header;
    PcieCapabilitiesRegister CapabilitiesRegister;
    DeviceCapabilitiesRegister DeviceCapabilities;
    DeviceControlRegister DeviceControl;
    DeviceStatusRegister DeviceStatus;
    LinkCapabilitiesRegister LinkCapabilities;
    LinkControlRegister LinkControl;
    LinkStatusRegister LinkStatus;
};

static_assert(sizeof(PcieCapabilityStructure) == 0x14, "PCIe Capability Structure is not 20 bytes.");

struct AdvancedErrorReportingCapabilityStructure final
{
    PciExtendedCapabilityHeader Header;
    u32 UncorrectableErrorStatusRegister;
    u32 UncorrectableErrorMaskRegister;
    u32 UncorrectableErrorSeverityRegister;
    u32 CorrectableErrorStatusRegister;
    u32 CorrectableErrorMaskRegister;
    u32 AdvancedCapabilitiesAndControlRegister;
    u32 HeaderLogRegister[4];
};

static_assert(sizeof(AdvancedErrorReportingCapabilityStructure) == 0x2C, "Advanced Error Reporting Capability Structure is not 44 bytes.");

struct TlpHeader final
{
    enum EFormat : u32
    {
        FORMAT_3_DW_HEADER_NO_DATA   = 0b00,
        FORMAT_4_DW_HEADER_NO_DATA   = 0b01,
        FORMAT_3_DW_HEADER_WITH_DATA = 0b10,
        FORMAT_4_DW_HEADER_WITH_DATA = 0b11,
    };

    enum EType : u32
    {
        TYPE_MEMORY_REQUEST             = 0b0'0000,
        TYPE_MEMORY_READ_REQUEST_LOCKED = 0b0'0001,
        TYPE_IO_REQUEST                 = 0b0'0010,
        TYPE_CONFIG_TYPE_0_REQUEST      = 0b0'0100,
        TYPE_CONFIG_TYPE_1_REQUEST      = 0b0'0101,
        TYPE_MESSAGE                    = 0b1'0000,
        TYPE_COMPLETION                 = 0b0'1010,
        TYPE_COMPLETION_LOCKED_READ     = 0b0'1011,
    };
public:
    // Bitfields are declared in Big Endian order.
    EType Type : 5;
    EFormat Fmt : 2;
    u32 Reserved0 : 1;
    u32 Reserved1 : 4;
    u32 TC : 3;
    u32 Reserved2 : 1;
    // Length of request in DWORDs
    u32 LengthHigh : 2;
    u32 Reserved3 : 2;
    u32 Attr : 2;
    u32 EP : 1;
    u32 TD : 1;
    u32 LengthLow : 8;

    [[nodiscard]] u32 Length() const noexcept
    {
        return LengthHigh << 8 | LengthLow;
    }

    void Length(const u32 length) noexcept
    {
        LengthLow = length & 0xFF;
        LengthHigh = (length >> 8) & 0x3;
    }
};

static_assert(sizeof(TlpHeader) == 4, "TLP Header was not 4 bytes in length.");

struct TlpTransactionDescriptor final
{
    // Bitfields are declared in Big Endian order.
    u32 BusNumber : 8;
    u32 FunctionNumber : 3;
    u32 DeviceNumber : 5;
    u32 Tag : 8;
    u32 FirstDwordByteEnable : 4;
    u32 LastDwordByteEnable : 4;

    static u32 ByteEnableToByteCount(const u32 byteEnable)
    {
        return ::std::popcount(byteEnable & 0xF);
    }
};

static_assert(sizeof(TlpTransactionDescriptor) == 4, "TLP Transaction Descriptor was not 4 bytes in length.");

struct TlpConfigRequestHeader final
{
    // Bitfields are declared in Big Endian order.
    u32 BusNumber : 8;
    u32 FunctionNumber : 3;
    u32 DeviceNumber : 5;
    u32 ExtendedRegisterNumber : 4;
    u32 Reserved0 : 4;
    u32 Reserved1 : 2;
    u32 RegisterNumber : 6;
};

static_assert(sizeof(TlpConfigRequestHeader) == 4, "TLP Config Request Header was not 4 bytes in length.");

struct TlpCompletionHeader0 final
{
    enum ECompletionStatus : u32
    {
        SuccessfulCompletion            = 0b000,
        UnsupportedRequest              = 0b001,
        ConfigurationRequestRetryStatus = 0b010,
        CompleterAbort                  = 0b100
    };
public:
    // Bitfields are declared in Big Endian order.
    u32 BusNumber : 8;
    u32 FunctionNumber : 3;
    u32 DeviceNumber : 5;
    u32 ByteCountHigh : 4;
    u32 ByteCountModified : 1;
    ECompletionStatus CompletionStatus : 3;
    u32 ByteCountLow : 8;

    [[nodiscard]] u32 ByteCount() const noexcept
    {
        return ByteCountHigh << 8 | ByteCountLow;
    }

    void ByteCount(const u32 byteCount) noexcept
    {
        ByteCountLow = byteCount & 0xFF;
        ByteCountHigh = (byteCount >> 8) & 0xF;
    }
};

static_assert(sizeof(TlpCompletionHeader0) == 4, "TLP Completion Response Header 0 was not 4 bytes in length.");

struct TlpCompletionHeader1 final
{
    // Bitfields are declared in Big Endian order.
    u32 BusNumber : 8;
    u32 FunctionNumber : 3;
    u32 DeviceNumber : 5;
    u32 Tag : 8;
    u32 LowerAddress : 7;
    u32 Reserved0 : 1;
};

static_assert(sizeof(TlpCompletionHeader1) == 4, "TLP Completion Response Header 1 was not 4 bytes in length.");

#pragma pack(pop)

}
