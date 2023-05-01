#define LOG_GROUP LOG_GROUP_MISC

#define RT_STRICT
#define PDMPCIDEV_INCLUDE_PRIVATE
#include "PciConfigHandler.hpp"
#include <VBox/vmm/pdmdev.h>
#include <VBox/err.h>
#include <VBox/msi.h>

#include <VBox/com/VirtualBox.h>

#include <iprt/assert.h>

#include <Processor.hpp>
#include "ConLogger.hpp"
#include "VBoxDeviceFunction.hpp"

static void SoftGpuUpdateMappings(const PPDMDEVINS deviceInstance, PPDMPCIDEV pPciDev) noexcept;

template<bool Reading>
static void LogConfigName(const uint32_t address, const unsigned size, const u32 value, const u8 functionId) noexcept;

static bool IsBar(const u32 address) noexcept
{
    return (address >= 0x10 && address < 0x28) || (address >= 0x30 && address < 0x34);
}

DECLCALLBACK(VBOXSTRICTRC) SoftGpuConfigRead(const PPDMDEVINS deviceInstance, const PPDMPCIDEV pPciDev, const uint32_t uAddress, const unsigned cb, uint32_t* const pu32Value)
{
    (void) pPciDev;

    SoftGpuDevice* device = PDMDEVINS_2_DATA(deviceInstance, SoftGpuDevice*);
    SoftGpuDeviceFunction& pciFunction = device->pciFunction;

    if(uAddress >= 4096)
    {
        ConLogLn("SoftGpu/[{}]: Read address {XP0} was greater than the config size of 4096.", pciFunction.FunctionId, uAddress);
        return VERR_OUT_OF_SELECTOR_BOUNDS;
    }

    if(cb != 1 && cb != 2 && cb != 4)
    {
        ConLogLn("SoftGpu/[{}]: Read size {} was not one of 1, 2, or 4.", pciFunction.FunctionId, cb);
        return VERR_OUT_OF_SELECTOR_BOUNDS;
    }

    const u32 readValue = pciFunction.processor.PciConfigRead(static_cast<u16>(uAddress), static_cast<u8>(cb));
    // u32 readValue;
    //
    // switch(cb)
    // {
    //     case 1:
    //         readValue = PDMPciDevGetByte(pPciDev, uAddress);
    //         break;
    //     case 2:
    //         readValue = PDMPciDevGetWord(pPciDev, uAddress);
    //         break;
    //     case 4:
    //         readValue = PDMPciDevGetDWord(pPciDev, uAddress);
    //         break;
    //     default:
    //         readValue = 0xCAFEBABE;
    //         break;
    // }

    LogConfigName<true>(uAddress, cb, readValue, pciFunction.FunctionId);
    ConLogLn("SoftGpu/[{}]: Config Read Address={XP0}, cb={}: {XP0}", pciFunction.FunctionId, uAddress, cb, readValue);

    static uSys readIndex = 0;
    ++readIndex;
    if(readIndex == 329)
    {
        ConLogLn("SoftGpu Pre blocking");
        // Sleep(5000);
        // for(uSys i = 0; i < 1024; ++i)
        // {
        //     ConLogLn("SoftGpu blocking");
        // }
    }

    *pu32Value = readValue;

    return VINF_SUCCESS;
}

DECLCALLBACK(VBOXSTRICTRC) SoftGpuConfigWrite(const PPDMDEVINS deviceInstance, const PPDMPCIDEV pPciDev, const uint32_t uAddress, const unsigned cb, const uint32_t u32Value)
{
    SoftGpuDevice* device = PDMDEVINS_2_DATA(deviceInstance, SoftGpuDevice*);
    SoftGpuDeviceFunction& pciFunction = device->pciFunction;

    if(uAddress >= 4096)
    {
        ConLogLn("SoftGpu/[{}]: Write address {XP0} was greater than the config size of 4096.", pciFunction.FunctionId, uAddress);
        return VERR_OUT_OF_SELECTOR_BOUNDS;
    }

    if(cb != 1 && cb != 2 && cb != 4)
    {
        ConLogLn("SoftGpu/[{}]: Write size {} was not one of 1, 2, or 4.", pciFunction.FunctionId, cb);
        return VERR_OUT_OF_SELECTOR_BOUNDS;
    }

    pciFunction.processor.PciConfigWrite(static_cast<u16>(uAddress), static_cast<u8>(cb), u32Value);

    // PDMPciDevSetDWord(pPciDev, uAddress, u32Value);
    //
    // u32 readValue;
    //
    // switch(cb)
    // {
    //     case 1:
    //         readValue = PDMPciDevGetByte(pPciDev, uAddress);
    //         break;
    //     case 2:
    //         readValue = PDMPciDevGetWord(pPciDev, uAddress);
    //         break;
    //     case 4:
    //         readValue = PDMPciDevGetDWord(pPciDev, uAddress);
    //         break;
    //     default:
    //         readValue = 0xCAFEBABE;
    //         break;
    // }

    const u32 readValue = pciFunction.processor.PciConfigRead(static_cast<u16>(uAddress), static_cast<u8>(cb));
    LogConfigName<false>(uAddress, cb, u32Value, pciFunction.FunctionId);
    ConLogLn("SoftGpu/[{}]: Config Write Address={XP0}, cb={}: {XP0} [{XP0}]", pciFunction.FunctionId, uAddress, cb, u32Value, readValue);

    (void) ::std::memcpy(&pPciDev->abConfig[uAddress], &readValue, cb);

    if(IsBar(uAddress))
    {
        SoftGpuUpdateMappings(deviceInstance, pPciDev);
    }

    return VINF_SUCCESS;
}

static u16 ComputeRegionOffset(const uSys iRegion) noexcept
{
    return iRegion == VBOX_PCI_ROM_SLOT
        ? VBOX_PCI_ROM_ADDRESS : (VBOX_PCI_BASE_ADDRESS_0 + iRegion * 4);
}

#define INVALID_PCI_ADDRESS     UINT32_MAX

static int SoftGpuR3UnmapRegion(PPDMPCIDEV pDev, int currentBar) noexcept;

static void SoftGpuUpdateMappings(const PPDMDEVINS deviceInstance, PPDMPCIDEV pPciDev) noexcept
{
    SoftGpuDevice* device = PDMDEVINS_2_DATA(deviceInstance, SoftGpuDevice*);
    SoftGpuDeviceFunction& pciFunction = device->pciFunction;

    const u16 commandRegister = pciFunction.processor.CommandRegister();
    ConLogLn(u8"SoftGpuUpdateMappings: dev {}/{} ({}): u16Cmd=0x{XP0}", pPciDev->uDevFn >> VBOX_PCI_DEVFN_DEV_SHIFT, pPciDev->uDevFn & VBOX_PCI_DEVFN_FUN_MASK, pPciDev->pszNameR3, commandRegister);

    for(uSys currentBar = 0; currentBar < VBOX_PCI_NUM_REGIONS; ++currentBar)
    {
        PCIIOREGION* const pciIoRegion = &pPciDev->Int.s.aIORegions[currentBar];
        const u64 regionSize = pciIoRegion->size;

        if(regionSize != 0)
        {
            const u16 barOffset = ComputeRegionOffset(currentBar);
            const bool is64Bit = (pciIoRegion->type & (static_cast<u8>(PCI_ADDRESS_SPACE_BAR64 | PCI_ADDRESS_SPACE_IO))) == PCI_ADDRESS_SPACE_BAR64;
            u64 newAddress = INVALID_PCI_ADDRESS;

            if(pciIoRegion->type & PCI_ADDRESS_SPACE_IO)
            {
                ConLogLn(u8"SoftGpuUpdateMappings: dev {}/{} ({}): Cannot map IO space for BAR{}.", 
                    pPciDev->uDevFn >> VBOX_PCI_DEVFN_DEV_SHIFT, pPciDev->uDevFn & VBOX_PCI_DEVFN_FUN_MASK, pPciDev->pszNameR3, currentBar);
            }
            else if(commandRegister & VBOX_PCI_COMMAND_MEMORY || true)
            {
                u64 memBase = pciFunction.processor.PciConfigRead(barOffset, 4);
                if(is64Bit)
                {
                    memBase |= static_cast<u64>(pciFunction.processor.PciConfigRead(barOffset + 4, 4)) << 32;
                }

                if(currentBar == PCI_ROM_SLOT)
                {
                    ConLogLn(u8"SoftGpuUpdateMappings: dev {}/{} ({}): Expansion ROM: Size = 0x{XP0}, 64 Bit = {}, Mem Base = 0x{XP0}.", 
                        pPciDev->uDevFn >> VBOX_PCI_DEVFN_DEV_SHIFT, pPciDev->uDevFn & VBOX_PCI_DEVFN_FUN_MASK, pPciDev->pszNameR3, regionSize, is64Bit ? "true" : "false", memBase);
                }
                else
                {
                    ConLogLn(u8"SoftGpuUpdateMappings: dev {}/{} ({}): BAR{}: Size = 0x{XP0}, 64 Bit = {}, Mem Base = 0x{XP0}.", 
                        pPciDev->uDevFn >> VBOX_PCI_DEVFN_DEV_SHIFT, pPciDev->uDevFn & VBOX_PCI_DEVFN_FUN_MASK, pPciDev->pszNameR3, currentBar, regionSize, is64Bit ? "true" : "false", memBase);
                }

                if(currentBar != PCI_ROM_SLOT || (memBase & RT_BIT_32(0))) /* ROM enable bit. */
                {
                    memBase &= ~(regionSize - 1);

                    u64 last = memBase + regionSize - 1;
                    if(memBase < last && memBase > 0)
                    {
                        if((memBase > 0xFFFFFFFF || last < 0xFFFF0000) && memBase < 0xFFFFFFFF00000000ull)
                        {
                            newAddress = memBase;
                        }
                        else
                        {
                            if(currentBar == PCI_ROM_SLOT)
                            {
                                ConLogLn(u8"SoftGpuUpdateMappings: dev {}/{} ({}): Expansion ROM: Rejecting address range: 0x{XP0}..0x{XP0}!",
                                    pPciDev->uDevFn >> VBOX_PCI_DEVFN_DEV_SHIFT, pPciDev->uDevFn & VBOX_PCI_DEVFN_FUN_MASK,
                                    pPciDev->pszNameR3, memBase, last);
                            }
                            else
                            {
                                ConLogLn(u8"SoftGpuUpdateMappings: dev {}/{} ({}): BAR{}: Rejecting address range: 0x{XP0}..0x{XP0}!",
                                    pPciDev->uDevFn >> VBOX_PCI_DEVFN_DEV_SHIFT, pPciDev->uDevFn & VBOX_PCI_DEVFN_FUN_MASK,
                                    pPciDev->pszNameR3, currentBar, memBase, last);
                            }
                        }
                    }
                    else
                    {
                        if(currentBar == PCI_ROM_SLOT)
                        {
                            ConLogLn(u8"SoftGpuUpdateMappings: dev {}/{} ({}): Expansion ROM: Disregarding invalid address range: 0x{XP0}..0x{XP0}!",
                                pPciDev->uDevFn >> VBOX_PCI_DEVFN_DEV_SHIFT, pPciDev->uDevFn & VBOX_PCI_DEVFN_FUN_MASK,
                                pPciDev->pszNameR3, memBase, last);
                        }
                        else
                        {
                            ConLogLn(u8"SoftGpuUpdateMappings: dev {}/{} ({}): BAR{}: Disregarding invalid address range: 0x{XP0}..0x{XP0}!",
                                pPciDev->uDevFn >> VBOX_PCI_DEVFN_DEV_SHIFT, pPciDev->uDevFn & VBOX_PCI_DEVFN_FUN_MASK,
                                pPciDev->pszNameR3, currentBar, memBase, last);
                        }
                    }
                }
            }

            /*
             * Do real unmapping and/or mapping if the address change.
             */
            if(currentBar == PCI_ROM_SLOT)
            {
                ConLogLn(u8"SoftGpuUpdateMappings: dev {}/{} ({}): Expansion ROM Address=0x{XP0} New Address=0x{XP0}",
                    pPciDev->uDevFn >> VBOX_PCI_DEVFN_DEV_SHIFT, pPciDev->uDevFn & VBOX_PCI_DEVFN_FUN_MASK, pPciDev->pszNameR3, pciIoRegion->addr, newAddress);
            }
            else
            {
                ConLogLn(u8"SoftGpuUpdateMappings: dev {}/{} ({}): BAR{} Address=0x{XP0} New Address=0x{XP0}",
                    pPciDev->uDevFn >> VBOX_PCI_DEVFN_DEV_SHIFT, pPciDev->uDevFn & VBOX_PCI_DEVFN_FUN_MASK, pPciDev->pszNameR3,
                    currentBar, pciIoRegion->addr, newAddress);
            }
            if(newAddress != pciIoRegion->addr)
            {
                if(currentBar == PCI_ROM_SLOT)
                {
                    ConLogLn(u8"PCI: config dev {}/{} ({}) Expansion ROM: 0x{XP0} -> 0x{XP0} (LB 0x{XP0} ({}))\n",
                        pPciDev->uDevFn >> VBOX_PCI_DEVFN_DEV_SHIFT, pPciDev->uDevFn & VBOX_PCI_DEVFN_FUN_MASK,
                        pPciDev->pszNameR3, pciIoRegion->addr, newAddress, regionSize, regionSize);
                }
                else
                {
                    ConLogLn(u8"PCI: config dev {}/{} ({}) BAR{}: 0x{XP0} -> 0x{XP0} (LB 0x{XP0} ({}))\n",
                        pPciDev->uDevFn >> VBOX_PCI_DEVFN_DEV_SHIFT, pPciDev->uDevFn & VBOX_PCI_DEVFN_FUN_MASK,
                        pPciDev->pszNameR3, currentBar, pciIoRegion->addr, newAddress, regionSize, regionSize);
                }

                int rc = SoftGpuR3UnmapRegion(pPciDev, currentBar);
                AssertLogRelRC(rc);
                pciIoRegion->addr = newAddress;
                if(newAddress != INVALID_PCI_ADDRESS)
                {
                    /* The callout is optional (typically not used): */
                    if(!pciIoRegion->pfnMap)
                    {
                        rc = VINF_SUCCESS;
                    }
                    else
                    {
                        rc = pciIoRegion->pfnMap(pPciDev->Int.s.pDevInsR3, pPciDev, currentBar, newAddress, regionSize, static_cast<PCIADDRESSSPACE>(pciIoRegion->type));
                        AssertLogRelRC(rc);
                    }

                    /* We do the mapping for most devices: */
                    if(pciIoRegion->hHandle != UINT64_MAX && rc != VINF_PCI_MAPPING_DONE)
                    {
                        switch(pciIoRegion->fFlags & PDMPCIDEV_IORGN_F_HANDLE_MASK)
                        {
                            case PDMPCIDEV_IORGN_F_IOPORT_HANDLE:
                                rc = PDMDevHlpIoPortMap(pPciDev->Int.s.pDevInsR3, pciIoRegion->hHandle, static_cast<RTIOPORT>(newAddress));
                                AssertLogRelRC(rc);
                                break;

                            case PDMPCIDEV_IORGN_F_MMIO_HANDLE:
                                rc = PDMDevHlpMmioMap(pPciDev->Int.s.pDevInsR3, pciIoRegion->hHandle, newAddress);
                                AssertLogRelRC(rc);
                                break;

                            case PDMPCIDEV_IORGN_F_MMIO2_HANDLE:
                                rc = PDMDevHlpMmio2Map(pPciDev->Int.s.pDevInsR3, pciIoRegion->hHandle, newAddress);
                                AssertRC(rc);
                                break;

                            default:
                                AssertLogRelFailed();
                        }
                    }
                }
            }
        }
    }
}

static int SoftGpuR3UnmapRegion(PPDMPCIDEV pDev, const int currentBar) noexcept
{
    PPCIIOREGION pRegion = &pDev->Int.s.aIORegions[currentBar];
    AssertReturn(pRegion->size != 0, VINF_SUCCESS);

    int rc = VINF_SUCCESS;
    if(pRegion->addr != INVALID_PCI_ADDRESS)
    {
        /*
         * Do callout first (optional), then do the unmapping via handle if we've been handed one.
         */
        if(pRegion->pfnMap)
        {
            rc = pRegion->pfnMap(pDev->Int.s.pDevInsR3, pDev, currentBar,
                NIL_RTGCPHYS, pRegion->size, (PCIADDRESSSPACE) (pRegion->type));
            AssertRC(rc);
        }

        switch(pRegion->fFlags & PDMPCIDEV_IORGN_F_HANDLE_MASK)
        {
            case PDMPCIDEV_IORGN_F_IOPORT_HANDLE:
                rc = PDMDevHlpIoPortUnmap(pDev->Int.s.pDevInsR3, (IOMIOPORTHANDLE) pRegion->hHandle);
                AssertRC(rc);
                break;

            case PDMPCIDEV_IORGN_F_MMIO_HANDLE:
                rc = PDMDevHlpMmioUnmap(pDev->Int.s.pDevInsR3, (IOMMMIOHANDLE) pRegion->hHandle);
                AssertRC(rc);
                break;

            case PDMPCIDEV_IORGN_F_MMIO2_HANDLE:
                rc = PDMDevHlpMmio2Unmap(pDev->Int.s.pDevInsR3, (PGMMMIO2HANDLE) pRegion->hHandle);
                AssertRC(rc);
                break;

            case PDMPCIDEV_IORGN_F_NO_HANDLE:
                Assert(pRegion->fFlags & PDMPCIDEV_IORGN_F_NEW_STYLE);
                Assert(pRegion->hHandle == UINT64_MAX);
                break;

            default:
                AssertLogRelFailed();
        }
        pRegion->addr = INVALID_PCI_ADDRESS;
    }
    return rc;
}

template<bool Reading>
static void LogConfigName(const uint32_t address, const unsigned size, const u32 value, const u8 functionId) noexcept
{
    (void) value;
    
    ConLog("SoftGpu/[{}]: {} ", functionId, Reading ? u8"Reading" : u8"Writing");

    switch(address)
    {
        case 0x0:
            if(size == 1)
            {
                ConLogLn(u8"PCI Vendor ID Low.");
            }
            else if(size == 2)
            {
                ConLogLn(u8"PCI Vendor ID.");
            }
            else if(size == 4)
            {
                ConLogLn(u8"PCI Vendor ID & Device ID.");
            }
            break;
        case 0x1:
            if(size == 1)
            {
                ConLogLn(u8"PCI Vendor ID High.");
            }
            break;
        case 0x2:
            if(size == 1)
            {
                ConLogLn(u8"PCI Device ID Low.");
            }
            else if(size == 2)
            {
                ConLogLn(u8"PCI Device ID.");
            }
            break;
        case 0x3:
            if(size == 1)
            {
                ConLogLn(u8"PCI Device ID High.");
            }
            break;
        case 0x4:
            if(size == 1)
            {
                ConLogLn(u8"PCI Command Register Low.");
            }
            else if(size == 2)
            {
                ConLogLn(u8"PCI Command Register.");
            }
            break;
        case 0x5:
            if(size == 1)
            {
                ConLogLn(u8"PCI Command Register High.");
            }
            break;
        case 0x6:
            if(size == 1)
            {
                ConLogLn(u8"PCI Status Register Low.");
            }
            else if(size == 2)
            {
                ConLogLn(u8"PCI Status Register.");
            }
            break;
        case 0x7:
            if(size == 1)
            {
                ConLogLn(u8"PCI Status Register High.");
            }
            break;
        case 0x8:
            if(size == 1)
            {
                ConLogLn(u8"PCI Revision ID.");
            }
            else if(size == 4)
            {
                ConLogLn(u8"PCI Revision ID & Class Code.");
            }
            break;
        case 0x9:
            if(size == 1)
            {
                ConLogLn(u8"PCI Class Code Byte 0.");
            }
            break;
        case 0xA:
            if(size == 1)
            {
                ConLogLn(u8"PCI Class Code Byte 1.");
            }
            break;
        case 0xB:
            if(size == 1)
            {
                ConLogLn(u8"PCI Class Code Byte 1.");
            }
            break;
        case 0xC:
            if(size == 1)
            {
                ConLogLn(u8"PCI Cache Line Size.");
            }
            else if(size == 2)
            {
                ConLogLn(u8"PCI Cache Line Size & Master Latency Timer.");
            }
            else if(size == 4)
            {
                ConLogLn(u8"PCI Cache Line Size, Master Latency Timer, Header Type, and Built-In Self Test Register.");
            }
            break;
        case 0xD:
            if(size == 1)
            {
                ConLogLn(u8"PCI Master Latency Timer.");
            }
            break;
        case 0xE:
            if(size == 1)
            {
                ConLogLn(u8"PCI Header Type.");
            }
            else if(size == 2)
            {
                ConLogLn(u8"PCI Header Type & Built-In Self Test Register.");
            }
            break;
        case 0xF:
            if(size == 1)
            {
                ConLogLn(u8"PCI Built-In Self Test Register.");
            }
            break;
        case 0x10:
            if(size == 1)
            {
                ConLogLn(u8"PCI BAR 0 Byte 0.");
            }
            else if(size == 4)
            {
                ConLogLn(u8"PCI BAR 0.");
            }
            break;
        case 0x14:

            if(size == 1)
            {
                ConLogLn(u8"PCI BAR 1 Byte 0.");
            }
            else if(size == 4)
            {
                ConLogLn(u8"PCI BAR 1.");
            }
            break;
        case 0x18:

            if(size == 1)
            {
                ConLogLn(u8"PCI BAR 2 Byte 0.");
            }
            else if(size == 4)
            {
                ConLogLn(u8"PCI BAR 2.");
            }
            break;
        case 0x1C:

            if(size == 1)
            {
                ConLogLn(u8"PCI BAR 3 Byte 0.");
            }
            else if(size == 4)
            {
                ConLogLn(u8"PCI BAR 3.");
            }
            break;
        case 0x20:

            if(size == 1)
            {
                ConLogLn(u8"PCI BAR 4 Byte 0.");
            }
            else if(size == 4)
            {
                ConLogLn(u8"PCI BAR 4.");
            }
            break;
        case 0x24:

            if(size == 1)
            {
                ConLogLn(u8"PCI BAR 5 Byte 0.");
            }
            else if(size == 4)
            {
                ConLogLn(u8"PCI BAR 5.");
            }
            break;
        case 0x28:
            if(size == 4)
            {
                ConLogLn(u8"PCI CardBus CIS Pointer.");
            }
            break;
        case 0x2C:
            if(size == 2)
            {
                ConLogLn(u8"PCI Subsystem Vendor ID.");
            }
            else if(size == 4)
            {
                ConLogLn(u8"PCI Subsystem Vendor ID & Subsystem ID.");
            }
            break;
        case 0x2E:
            if(size == 2)
            {
                ConLogLn(u8"PCI Subsystem ID.");
            }
            break;
        case 0x30:

            if(size == 1)
            {
                ConLogLn(u8"PCI Expansion ROM Base Address Byte 0.");
            }
            else if(size == 4)
            {
                ConLogLn(u8"PCI Expansion ROM Base Address.");
            }
            break;
        case 0x34:
            ConLogLn(u8"PCI Capability Pointer.");
            break;
        case 0x3C:
            if(size == 1)
            {
                ConLogLn(u8"PCI Interrupt Line.");
            }
            else if(size == 2)
            {
                ConLogLn(u8"PCI Interrupt Line & Interrupt Pin.");
            }
            else if(size == 4)
            {
                ConLogLn(u8"PCI Interrupt Line, Interrupt Pin, Min GNT, and Max LAT.");
            }
            break;
        case 0x3D:
            if(size == 1)
            {
                ConLogLn(u8"PCI Interrupt Pin.");
            }
            break;
        case 0x3E:
            if(size == 1)
            {
                ConLogLn(u8"PCI Min GNT.");
            }
            else if(size == 2)
            {
                ConLogLn(u8"PCI Min GNT & Max LAT.");
            }
            break;
        case 0x3F:
            if(size == 1)
            {
                ConLogLn(u8"PCI Max LAT.");
            }
            break;
        case PciConfigOffsets::PciCapabilityOffsetBegin + offsetof(PcieCapabilityStructure, Header) + offsetof(PciCapabilityHeader, CapabilityId):
            if(size == 1)
            {
                ConLogLn(u8"PCIe Capability ID.");
            }
            else if(size == 2)
            {
                ConLogLn(u8"PCIe Capability ID & Next Capability Pointer.");
            }
            break;
        case PciConfigOffsets::PciCapabilityOffsetBegin + offsetof(PcieCapabilityStructure, Header) + offsetof(PciCapabilityHeader, NextCapabilityPointer):
            if(size == 1)
            {
                ConLogLn(u8"PCIe Capability Next Capability Pointer.");
            }
            break;
        case PciConfigOffsets::PciCapabilityOffsetBegin + offsetof(PcieCapabilityStructure, CapabilitiesRegister):
            if(size == 2)
            {
                ConLogLn(u8"PCIe Capability Capabilities Register.");
            }
            break;
        case PciConfigOffsets::PciCapabilityOffsetBegin + offsetof(PcieCapabilityStructure, DeviceCapabilities):
            if(size == 4)
            {
                ConLogLn(u8"PCIe Capability Device Capabilities Register.");
            }
            break;
        case PciConfigOffsets::PciCapabilityOffsetBegin + offsetof(PcieCapabilityStructure, DeviceControl):
            if(size == 2)
            {
                ConLogLn(u8"PCIe Capability Device Control Register.");
            }
            break;
        case PciConfigOffsets::PciCapabilityOffsetBegin + offsetof(PcieCapabilityStructure, DeviceStatus):
            if(size == 2)
            {
                ConLogLn(u8"PCIe Capability Device Status Register.");
            }
            break;
        case PciConfigOffsets::PowerManagementCapabilityOffsetBegin + offsetof(PowerManagementCapabilityStructure, Header) + offsetof(PciCapabilityHeader, CapabilityId):
            if(size == 1)
            {
                ConLogLn(u8"PCIe Power Management Capability ID.");
            }
            else if(size == 2)
            {
                ConLogLn(u8"PCIe Power Management Capability ID & Next Capability Pointer.");
            }
            break;
        case PciConfigOffsets::PowerManagementCapabilityOffsetBegin + offsetof(PowerManagementCapabilityStructure, Header) + offsetof(PciCapabilityHeader, NextCapabilityPointer):
            if(size == 1)
            {
                ConLogLn(u8"PCIe Power Management Capability Next Capability Pointer.");
            }
            break;
        case PciConfigOffsets::PowerManagementCapabilityOffsetBegin + offsetof(PowerManagementCapabilityStructure, PowerManagementCapabilities):
            if(size == 2)
            {
                ConLogLn(u8"PCIe Power Management Capability Capabilities.");
            }
            break;
        case PciConfigOffsets::PowerManagementCapabilityOffsetBegin + offsetof(PowerManagementCapabilityStructure, PowerManagementControlStatusRegister):
            if(size == 2)
            {
                ConLogLn(u8"PCIe Power Management Capability Status Register.");
            }
            break;
        case PciConfigOffsets::PowerManagementCapabilityOffsetBegin + offsetof(PowerManagementCapabilityStructure, BridgeExtensions):
            if(size == 1)
            {
                ConLogLn(u8"PCIe Power Management Capability Bridge Extensions.");
            }
            else if(size == 2)
            {
                ConLogLn(u8"PCIe Power Management Capability Bridge Extensions & Data.");
            }
            break;
        case PciConfigOffsets::PowerManagementCapabilityOffsetBegin + offsetof(PowerManagementCapabilityStructure, Data):
            if(size == 1)
            {
                ConLogLn(u8"PCIe Power Management Capability Data.");
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, Header) + offsetof(PciExtendedCapabilityHeader, CapabilityId):
            if(size == 2)
            {
                ConLogLn(u8"PCIe Advanced Error Reporting Capability ID.");
            }
            else if(size == 4)
            {
                ConLogLn(u8"PCIe Advanced Error Reporting Capability ID, Version & Next Capability Pointer.");
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, Header) + offsetof(PciExtendedCapabilityHeader, CapabilityId) + 2:
            if(size == 2)
            {
                ConLogLn(u8"PCIe Advanced Error Reporting Capability Version & Next Capability Pointer.");
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, UncorrectableErrorStatusRegister):
            if(size == 4)
            {
                ConLogLn(u8"PCIe Advanced Error Reporting Capability Uncorrectable Error Status Register.");
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, UncorrectableErrorMaskRegister):
            if(size == 4)
            {
                ConLogLn(u8"PCIe Advanced Error Reporting Capability Uncorrectable Error Mask Register.");
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, UncorrectableErrorSeverityRegister):
            if(size == 4)
            {
                ConLogLn(u8"PCIe Advanced Error Reporting Capability Uncorrectable Error Severity Register.");
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, CorrectableErrorStatusRegister):
            if(size == 4)
            {
                ConLogLn(u8"PCIe Advanced Error Reporting Capability Correctable Error Status Register.");
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, CorrectableErrorMaskRegister):
            if(size == 4)
            {
                ConLogLn(u8"PCIe Advanced Error Reporting Capability Correctable Error Mask Register.");
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, AdvancedCapabilitiesAndControlRegister):
            if(size == 4)
            {
                ConLogLn(u8"PCIe Advanced Error Reporting Capability Advanced Capabilities And Control Register.");
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, HeaderLogRegister):
            if(size == 4)
            {
                ConLogLn(u8"PCIe Advanced Error Reporting Capability Header Log Register 0.");
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, HeaderLogRegister) + 4:
            if(size == 4)
            {
                ConLogLn(u8"PCIe Advanced Error Reporting Capability Header Log Register 1.");
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, HeaderLogRegister) + 8:
            if(size == 4)
            {
                ConLogLn(u8"PCIe Advanced Error Reporting Capability Header Log Register 2.");
            }
            break;
        case PciConfigOffsets::AdvancedErrorReportingCapabilityOffsetBegin + offsetof(AdvancedErrorReportingCapabilityStructure, HeaderLogRegister) + 12:
            if(size == 4)
            {
                ConLogLn(u8"PCIe Advanced Error Reporting Capability Header Log Register 3.");
            }
            break;
        default: break;
    }
}
