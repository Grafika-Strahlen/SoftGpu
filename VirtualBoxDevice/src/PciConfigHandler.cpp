#define LOG_GROUP LOG_GROUP_MISC

#define RT_STRICT
#include "PciConfigHandler.hpp"
#include <VBox/com/Guid.h>
#include <VBox/vmm/pdmdev.h>
#include <VBox/err.h>
#include <VBox/msi.h>

#include <VBox/com/VirtualBox.h>

#include <iprt/assert.h>

#include <Processor.hpp>
#include "ConLogger.hpp"
#include "VBoxDeviceFunction.hpp"

template<bool Reading>
static void LogConfigName(const uint32_t address, const unsigned size, const u32 value) noexcept;

DECLCALLBACK(VBOXSTRICTRC) SoftGpuConfigRead(const PPDMDEVINS deviceInstance, const PPDMPCIDEV pPciDev, const uint32_t uAddress, const unsigned cb, uint32_t* const pu32Value)
{
    (void) pPciDev;

    SoftGpuDevice* device = PDMDEVINS_2_DATA(deviceInstance, SoftGpuDevice*);
    SoftGpuDeviceFunction& pciFunction = device->pciFunction;

    if(uAddress >= 4096)
    {
        ConLogLn("SoftGpu/[{}]: Read address {XP0} was greater than the config size of 4096.", pciFunction.iFun, uAddress);
        return VERR_OUT_OF_SELECTOR_BOUNDS;
    }

    if(cb != 1 && cb != 2 && cb != 4)
    {
        ConLogLn("SoftGpu/[{}]: Read size {} was not one of 1, 2, or 4.", pciFunction.iFun, cb);
        return VERR_OUT_OF_SELECTOR_BOUNDS;
    }

    const u32 readValue = pciFunction.processor.PciConfigRead(static_cast<u16>(uAddress), static_cast<u8>(cb));
    LogConfigName<true>(uAddress, cb, readValue);
    ConLogLn("SoftGpu/[{}]: Config Read Address={XP0}, cb={}: {XP0}", pciFunction.iFun, uAddress, cb, readValue);

    *pu32Value = readValue;

    return VINF_SUCCESS;
}

DECLCALLBACK(VBOXSTRICTRC) SoftGpuConfigWrite(const PPDMDEVINS deviceInstance, const PPDMPCIDEV pPciDev, const uint32_t uAddress, const unsigned cb, const uint32_t u32Value)
{
    SoftGpuDevice* device = PDMDEVINS_2_DATA(deviceInstance, SoftGpuDevice*);
    SoftGpuDeviceFunction& pciFunction = device->pciFunction;

    if(uAddress >= 4096)
    {
        ConLogLn("SoftGpu/[{}]: Write address {XP0} was greater than the config size of 4096.", pciFunction.iFun, uAddress);
        return VERR_OUT_OF_SELECTOR_BOUNDS;
    }

    if(cb != 1 && cb != 2 && cb != 4)
    {
        ConLogLn("SoftGpu/[{}]: Write size {} was not one of 1, 2, or 4.", pciFunction.iFun, cb);
        return VERR_OUT_OF_SELECTOR_BOUNDS;
    }

    pciFunction.processor.PciConfigWrite(static_cast<u16>(uAddress), static_cast<u8>(cb), u32Value);

    const u32 readValue = pciFunction.processor.PciConfigRead(static_cast<u16>(uAddress), static_cast<u8>(cb));
    LogConfigName<false>(uAddress, cb, u32Value);
    ConLogLn("SoftGpu/[{}]: Config Write Address={XP0}, cb={}: {XP0} [{XP0}]", pciFunction.iFun, uAddress, cb, u32Value, readValue);

    (void) ::std::memcpy(&pPciDev->abConfig[uAddress], &readValue, cb);

    return VINF_SUCCESS;
}

template<bool Reading>
static void LogConfigName(const uint32_t address, const unsigned size, const u32 value) noexcept
{
    (void) value;

    constexpr const c8* ReadWriteStr = Reading ? u8"Reading" : u8"Writing";

    switch(address)
    {
        case 0x0:
            if(size == 2)
            {
                ConLogLn(u8"{} PCI Vendor ID.", ReadWriteStr);
            }
            else if(size == 4)
            {
                ConLogLn(u8"{} PCI Vendor ID & Device ID.", ReadWriteStr);
            }
            break;
        case 0x2:
            if(size == 2)
            {
                ConLogLn(u8"{} PCI Device ID.", ReadWriteStr);
            }
            break;
        case 0x4:
            if(size == 2)
            {
                ConLogLn(u8"{} PCI Command Register.", ReadWriteStr);
            }
            break;
        case 0x6:
            if(size == 2)
            {
                ConLogLn(u8"{} PCI Status Register.", ReadWriteStr);
            }
            break;
        case 0x8:
            if(size == 1)
            {
                ConLogLn(u8"{} PCI Revision ID.", ReadWriteStr);
            }
            else if(size == 4)
            {
                ConLogLn(u8"{} PCI Revision ID & Class Code.", ReadWriteStr);
            }
            break;
        case 0xC:
            if(size == 1)
            {
                ConLogLn(u8"{} PCI Cache Line Size.", ReadWriteStr);
            }
            else if(size == 2)
            {
                ConLogLn(u8"{} PCI Cache Line Size & Master Latency Timer.", ReadWriteStr);
            }
            else if(size == 4)
            {
                ConLogLn(u8"{} PCI Cache Line Size, Master Latency Timer, Header Type, and Built-In Self Test Register.", ReadWriteStr);
            }
            break;
        case 0xD:
            if(size == 1)
            {
                ConLogLn(u8"{} PCI Master Latency Timer.", ReadWriteStr);
            }
            break;
        case 0xE:
            if(size == 1)
            {
                ConLogLn(u8"{} PCI Header Type.", ReadWriteStr);
            }
            else if(size == 2)
            {
                ConLogLn(u8"{} PCI Header Type & Built-In Self Test Register.", ReadWriteStr);
            }
            break;
        case 0xF:
            if(size == 1)
            {
                ConLogLn(u8"{} PCI Built-In Self Test Register.", ReadWriteStr);
            }
            break;
        case 0x10:
            if(size == 4)
            {
                ConLogLn(u8"{} PCI BAR 0.", ReadWriteStr);
            }
            break;
        case 0x14:
            if(size == 4)
            {
                ConLogLn(u8"{} PCI BAR 1.", ReadWriteStr);
            }
            break;
        case 0x18:
            if(size == 4)
            {
                ConLogLn(u8"{} PCI BAR 2.", ReadWriteStr);
            }
            break;
        case 0x1C:
            if(size == 4)
            {
                ConLogLn(u8"{} PCI BAR 3.", ReadWriteStr);
            }
            break;
        case 0x20:
            if(size == 4)
            {
                ConLogLn(u8"{} PCI BAR 4.", ReadWriteStr);
            }
            break;
        case 0x24:
            if(size == 4)
            {
                ConLogLn(u8"{} PCI BAR 5.", ReadWriteStr);
            }
            break;
        case 0x28:
            if(size == 4)
            {
                ConLogLn(u8"{} PCI CardBus CIS Pointer.", ReadWriteStr);
            }
            break;
        case 0x2C:
            if(size == 2)
            {
                ConLogLn(u8"{} PCI Subsystem Vendor ID.", ReadWriteStr);
            }
            else if(size == 4)
            {
                ConLogLn(u8"{} PCI Subsystem Vendor ID & Subsystem ID.", ReadWriteStr);
            }
            break;
        case 0x2E:
            if(size == 2)
            {
                ConLogLn(u8"{} PCI Subsystem ID.", ReadWriteStr);
            }
            break;
        case 0x30:
            if(size == 4)
            {
                ConLogLn(u8"{} PCI Expansion ROM Base Address.", ReadWriteStr);
            }
            break;
        case 0x34:
            ConLogLn(u8"{} PCI Capability Pointer.", ReadWriteStr);
            break;
        case 0x3C:
            if(size == 1)
            {
                ConLogLn(u8"{} PCI Interrupt Line.", ReadWriteStr);
            }
            else if(size == 2)
            {
                ConLogLn(u8"{} PCI Interrupt Line & Interrupt Pin.", ReadWriteStr);
            }
            else if(size == 4)
            {
                ConLogLn(u8"{} PCI Interrupt Line, Interrupt Pin, Min GNT, and Max LAT.", ReadWriteStr);
            }
            break;
        case 0x3D:
            if(size == 1)
            {
                ConLogLn(u8"{} PCI Interrupt Pin.", ReadWriteStr);
            }
            break;
        case 0x3E:
            if(size == 1)
            {
                ConLogLn(u8"{} PCI Min GNT.", ReadWriteStr);
            }
            else if(size == 2)
            {
                ConLogLn(u8"{} PCI Min GNT & Max LAT.", ReadWriteStr);
            }
            break;
        case 0x3F:
            if(size == 1)
            {
                ConLogLn(u8"{} PCI Max LAT.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::PciCapabilityOffsetBegin + offsetof(PcieCapabilityStructure, Header) + offsetof(PciCapabilityHeader, CapabilityId):
            if(size == 1)
            {
                ConLogLn(u8"{} PCIe Capability ID.", ReadWriteStr);
            }
            else if(size == 2)
            {
                ConLogLn(u8"{} PCIe Capability ID & Next Capability Pointer.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::PciCapabilityOffsetBegin + offsetof(PcieCapabilityStructure, Header) + offsetof(PciCapabilityHeader, NextCapabilityPointer):
            if(size == 1)
            {
                ConLogLn(u8"{} PCIe Capability Next Capability Pointer.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::PciCapabilityOffsetBegin + offsetof(PcieCapabilityStructure, CapabilitiesRegister):
            if(size == 2)
            {
                ConLogLn(u8"{} PCIe Capability Capabilities Register.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::PciCapabilityOffsetBegin + offsetof(PcieCapabilityStructure, DeviceCapabilities):
            if(size == 4)
            {
                ConLogLn(u8"{} PCIe Capability Device Capabilities Register.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::PciCapabilityOffsetBegin + offsetof(PcieCapabilityStructure, DeviceControl):
            if(size == 2)
            {
                ConLogLn(u8"{} PCIe Capability Device Control Register.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::PciCapabilityOffsetBegin + offsetof(PcieCapabilityStructure, DeviceStatus):
            if(size == 2)
            {
                ConLogLn(u8"{} PCIe Capability Device Status Register.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::PowerManagementCapabilityOffsetBegin + offsetof(PowerManagementCapabilityStructure, Header) + offsetof(PciCapabilityHeader, CapabilityId):
            if(size == 1)
            {
                ConLogLn(u8"{} PCIe Power Management Capability ID.", ReadWriteStr);
            }
            else if(size == 2)
            {
                ConLogLn(u8"{} PCIe Power Management Capability ID & Next Capability Pointer.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::PowerManagementCapabilityOffsetBegin + offsetof(PowerManagementCapabilityStructure, Header) + offsetof(PciCapabilityHeader, NextCapabilityPointer):
            if(size == 1)
            {
                ConLogLn(u8"{} PCIe Power Management Capability Next Capability Pointer.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::PowerManagementCapabilityOffsetBegin + offsetof(PowerManagementCapabilityStructure, PowerManagementCapabilities):
            if(size == 2)
            {
                ConLogLn(u8"{} PCIe Power Management Capability Capabilities.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::PowerManagementCapabilityOffsetBegin + offsetof(PowerManagementCapabilityStructure, PowerManagementControlStatusRegister):
            if(size == 2)
            {
                ConLogLn(u8"{} PCIe Power Management Capability Status Register.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::PowerManagementCapabilityOffsetBegin + offsetof(PowerManagementCapabilityStructure, BridgeExtensions):
            if(size == 1)
            {
                ConLogLn(u8"{} PCIe Power Management Capability Bridge Extensions.", ReadWriteStr);
            }
            else if(size == 2)
            {
                ConLogLn(u8"{} PCIe Power Management Capability Bridge Extensions & Data.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::PowerManagementCapabilityOffsetBegin + offsetof(PowerManagementCapabilityStructure, Data):
            if(size == 1)
            {
                ConLogLn(u8"{} PCIe Power Management Capability Data.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, Header) + offsetof(PciExtendedCapabilityHeader, CapabilityId):
            if(size == 2)
            {
                ConLogLn(u8"{} PCIe Advanced Error Reporting Capability ID.", ReadWriteStr);
            }
            else if(size == 4)
            {
                ConLogLn(u8"{} PCIe Advanced Error Reporting Capability ID, Version & Next Capability Pointer.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, Header) + offsetof(PciExtendedCapabilityHeader, CapabilityId) + 2:
            if(size == 2)
            {
                ConLogLn(u8"{} PCIe Advanced Error Reporting Capability Version & Next Capability Pointer.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, UncorrectableErrorStatusRegister):
            if(size == 4)
            {
                ConLogLn(u8"{} PCIe Advanced Error Reporting Capability Uncorrectable Error Status Register.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, UncorrectableErrorMaskRegister):
            if(size == 4)
            {
                ConLogLn(u8"{} PCIe Advanced Error Reporting Capability Uncorrectable Error Mask Register.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, UncorrectableErrorSeverityRegister):
            if(size == 4)
            {
                ConLogLn(u8"{} PCIe Advanced Error Reporting Capability Uncorrectable Error Severity Register.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, CorrectableErrorStatusRegister):
            if(size == 4)
            {
                ConLogLn(u8"{} PCIe Advanced Error Reporting Capability Correctable Error Status Register.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, CorrectableErrorMaskRegister):
            if(size == 4)
            {
                ConLogLn(u8"{} PCIe Advanced Error Reporting Capability Correctable Error Mask Register.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, AdvancedCapabilitiesAndControlRegister):
            if(size == 4)
            {
                ConLogLn(u8"{} PCIe Advanced Error Reporting Capability Advanced Capabilities And Control Register.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, HeaderLogRegister):
            if(size == 4)
            {
                ConLogLn(u8"{} PCIe Advanced Error Reporting Capability Header Log Register 0.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, HeaderLogRegister) + 4:
            if(size == 4)
            {
                ConLogLn(u8"{} PCIe Advanced Error Reporting Capability Header Log Register 1.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, HeaderLogRegister) + 8:
            if(size == 4)
            {
                ConLogLn(u8"{} PCIe Advanced Error Reporting Capability Header Log Register 2.", ReadWriteStr);
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, HeaderLogRegister) + 12:
            if(size == 4)
            {
                ConLogLn(u8"{} PCIe Advanced Error Reporting Capability Header Log Register 3.", ReadWriteStr);
            }
            break;
        default: break;
    }
}
