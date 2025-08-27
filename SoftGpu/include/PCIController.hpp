/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <Objects.hpp>
#include <NumTypes.hpp>
#include <ConPrinter.hpp>
#include <Common.hpp>
#include <PcieProtocol.hpp>
#include <riscv/DualClockFIFO/DualClockFIFO.hpp>
#include "VirtualBoxPciPhy.hpp"

#include <cstring>
#include <mutex>
#include <semaphore>
#include <functional>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#ifdef DeviceCapabilities
#undef DeviceCapabilities
#endif

class Processor;

#pragma pack(push, 1)
struct PciConfigData final
{
    pci::PciConfigHeader ConfigHeader;
    pcie::PcieCapabilityStructure PcieCapability;
    pci::PowerManagementCapabilityStructure PowerManagementCapability;
    pci::MessageSignalledInterruptCapabilityStructure MessageSignalledInterruptCapability;
    u8 PciConfig[256 - sizeof(ConfigHeader) - sizeof(PcieCapability) - sizeof(PowerManagementCapability) - sizeof(MessageSignalledInterruptCapability)];
    pcie::AdvancedErrorReportingCapabilityStructure AdvancedErrorReportingCapability;
    u8 PciExtendedConfig[4096 - 256 - sizeof(AdvancedErrorReportingCapability)];
};

static_assert(sizeof(PciConfigData) == 4096, "PCIe Config Data Structure is not 4096 bytes.");
#pragma pack(pop)

class PciControllerReceiverSample
{
public:
    void ReceivePciController_TriggerResetN(const StdLogic reset_n) noexcept { }
};

class PciController final
{
    DEFAULT_DESTRUCT(PciController);
    DELETE_CM(PciController);
public:
    static inline constexpr u16 COMMAND_REGISTER_MASK_BITS = 0x0446;

    static inline constexpr u16 COMMAND_REGISTER_MEMORY_SPACE_BIT = 0x0002;
    static inline constexpr u16 COMMAND_REGISTER_BUS_MASTER_BIT   = 0x0004;

    static inline constexpr u32 BAR0_MASK_BITS = 0xFF000000;
    static inline constexpr u32 BAR1_MASK_BITS = 0xE0000000;
    static inline constexpr u32 BAR2_MASK_BITS = 0xFFFFFFFF;
    // static inline constexpr u32 BAR1_MASK_BITS = 0x00000000;
    // static inline constexpr u32 BAR2_MASK_BITS = 0xFFFFFFFE;
    static inline constexpr u32 BAR3_MASK_BITS = 0x00000000;
    static inline constexpr u32 BAR4_MASK_BITS = 0x00000000;
    static inline constexpr u32 BAR5_MASK_BITS = 0x00000000;

    static inline constexpr u32 BAR0_READ_ONLY_BITS = 0x00000000;
    static inline constexpr u32 BAR1_READ_ONLY_BITS = 0x0000000C;
    static inline constexpr u32 BAR2_READ_ONLY_BITS = 0x00000000;
    static inline constexpr u32 BAR3_READ_ONLY_BITS = 0x00000000;
    static inline constexpr u32 BAR4_READ_ONLY_BITS = 0x00000000;
    static inline constexpr u32 BAR5_READ_ONLY_BITS = 0x00000000;

    static inline constexpr u32 EXPANSION_ROM_BAR_MASK_BITS         = 0xFFFF8001;
    static inline constexpr u32 EXPANSION_ROM_BAR_ADDRESS_MASK_BITS = 0xFFFF8000;
    static inline constexpr u32 EXPANSION_ROM_BAR_READ_ONLY_BITS    = 0x00000000;
    static inline constexpr u32 EXPANSION_ROM_BAR_ENABLE_BIT        = 0x00000001;
    static inline constexpr u32 EXPANSION_ROM_SIZE = 32 * 1024;

    static inline constexpr u16 DEVICE_CONTROL_REGISTER_MASK_BITS      = 0x7CFF;
    static inline constexpr u16 DEVICE_CONTROL_REGISTER_READ_ONLY_BITS = 0x0000;

    static inline constexpr u16 LINK_CONTROL_REGISTER_MASK_BITS      = 0x01C3;
    static inline constexpr u16 LINK_CONTROL_REGISTER_READ_ONLY_BITS = 0x0000;

    static inline constexpr u16 PM_CONTROL_REGISTER_MASK_BITS      = 0x0003;
    static inline constexpr u16 PM_CONTROL_REGISTER_READ_ONLY_BITS = 0x0000;

    static inline constexpr u16 MESSAGE_CONTROL_REGISTER_MASK_BITS      = 0x0071;
    static inline constexpr u16 MESSAGE_CONTROL_REGISTER_READ_ONLY_BITS = 0x0080;
    static inline constexpr u32 MESSAGE_ADDRESS_REGISTER_MASK_BITS      = 0xFFFFFFFC;
    static inline constexpr u32 MESSAGE_ADDRESS_REGISTER_READ_ONLY_BITS = 0x00000000;

    static inline constexpr u8 EXPANSION_ROM_BAR_ID = 0x7F;

    using InterruptCallback_f = ::std::function<void(const u16 messageData)>;
    using BusMasterReadCallback_f = ::std::function<void(const u64 address, const u16 size, void* const buffer)>;
    using BusMasterWriteCallback_f = ::std::function<void(const u64 address, const u16 size, const void* const buffer)>;
private:
    using Receiver = PciControllerReceiverSample;

    SENSITIVITY_DECL(p_Reset_n, p_Clock, p_RxClock);

    SIGNAL_ENTITIES();
public:
    explicit PciController(Receiver* const parent) noexcept
        : m_ConfigData{ }
        , m_Parent(parent)
        , p_Clock(0)
        , p_Reset_n(0)
        , p_Pad0{}
        , p_RxClock(0)
        , p_Pad1{}
        , m_ReadRequestActive(false)
        , m_ReadRequestAddress(0)
        , m_ReadRequestSize(0)
        , m_ReadRequestResponseData(nullptr)
        , m_ReadCountResponse(nullptr)
        , m_WriteRequestActive(false)
        , m_WriteRequestAddress(0)
        , m_WriteRequestSize(0)
        , m_WriteRequestData(nullptr)
        , m_InterruptCallback(nullptr)
        , m_BusMasterReadCallback(nullptr)
        , m_BusMasterWriteCallback(nullptr)
        , m_ReadState(0)
        , m_WriteState(0)
        , m_InterruptSet(0)
        , m_Pad0{}
        , m_PhyInputFifo(this, 0)
        , m_PhyOutputFifo(this, 1)
        , m_PciPhy(this)
#ifdef _WIN32
        , m_SimulationSyncEvent(INVALID_HANDLE_VALUE)
#endif
        , m_SimulationSyncBinarySemaphore(0)
        , m_ReadDataMutex()
        , m_WriteDataMutex()
    {
        InitConfigHeader();
        InitPcieCapabilityStructure();
        InitPowerManagementCapabilityStructure();
        InitMessageSignalledInterruptCapabilityStructure();
        InitAdvancedErrorReportingCapabilityStructure();
    }

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

    void Clock(bool risingEdge = true) noexcept
    {
        if(risingEdge)
        {
            ExecuteMemRead();
        }
        else
        {
            ExecuteInterrupt();
            ExecuteMemWrite();
        }
    }

    [[nodiscard]] u32 ConfigRead(u16 address, u8 size) noexcept;

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

    void PciMemReadSet(const u64 address, const u16 size, u32* const data, u16* const readResponse) noexcept
    {
        ::std::lock_guard lock(m_ReadDataMutex);
        m_ReadRequestActive = true;
        m_ReadRequestAddress = address;
        m_ReadRequestSize = size;
        m_ReadRequestResponseData = data;
        m_ReadCountResponse = readResponse;
    }

    void PciMemWriteSet(const u64 address, const u16 size, const u32* const data) noexcept
    {
        ::std::lock_guard lock(m_WriteDataMutex);
        m_WriteRequestActive = true;
        m_WriteRequestAddress = address;
        m_WriteRequestSize = size;
        m_WriteRequestData = data;
    }

    void PciBusMasterRead(const u64 address, const u16 sizeInWords, u32 transferBlock[1024]) noexcept
    {
        if((m_ConfigData.ConfigHeader.Command & COMMAND_REGISTER_BUS_MASTER_BIT) != COMMAND_REGISTER_BUS_MASTER_BIT)
        {
            return;
        }

        if(!m_BusMasterReadCallback)
        {
            return;
        }

        m_BusMasterReadCallback(address, sizeInWords * sizeof(u32), transferBlock);
    }

    void PciBusMasterWrite(const u64 address, const u16 sizeInWords, const u32 transferBlock[1024]) noexcept
    {
        if((m_ConfigData.ConfigHeader.Command & COMMAND_REGISTER_BUS_MASTER_BIT) != COMMAND_REGISTER_BUS_MASTER_BIT)
        {
            return;
        }

        if(m_BusMasterWriteCallback)
        {
            return;
        }

        m_BusMasterWriteCallback(address, sizeInWords * sizeof(u32), transferBlock);
    }

    void SetInterrupt(const u32 messageType) noexcept
    {
        (void) messageType;

        m_InterruptSet = true;
    }

    [[nodiscard]] u8 GetBARFromAddress(const u64 address) const noexcept
    {
        if(address < 0xFFFFFFFF)
        {
            if(address >= (m_ConfigData.ConfigHeader.BAR0 & BAR0_MASK_BITS) && address < ((m_ConfigData.ConfigHeader.BAR0 & BAR0_MASK_BITS) + 16 * 1024 * 1024))
            {
                return 0;
            }

            if(
                address >= (m_ConfigData.ConfigHeader.ExpansionROMBaseAddress & EXPANSION_ROM_BAR_ADDRESS_MASK_BITS) &&
                address < ((m_ConfigData.ConfigHeader.ExpansionROMBaseAddress & EXPANSION_ROM_BAR_ADDRESS_MASK_BITS) + EXPANSION_ROM_SIZE)
            )
            {
                return EXPANSION_ROM_BAR_ID;
            }
        }

        const u64 bar1 = (static_cast<u64>(m_ConfigData.ConfigHeader.BAR2 & BAR2_MASK_BITS) << 32) | (m_ConfigData.ConfigHeader.BAR1 & BAR1_MASK_BITS);

        if(address >= bar1 && address < bar1 + (2ull * 1024ull * 1024ull * 1024ull))
        {
            return 1;
        }

        return 0xFF;
    }

    [[nodiscard]] u64 GetBAROffset(const u64 address, const u8 bar) const noexcept
    {
        if(bar == 0)
        {
            return address - (m_ConfigData.ConfigHeader.BAR0 & BAR0_MASK_BITS);
        }

        if(bar == 1)
        {
            const u64 bar1 = (static_cast<u64>(m_ConfigData.ConfigHeader.BAR2 & BAR2_MASK_BITS) << 32) | (m_ConfigData.ConfigHeader.BAR1 & BAR1_MASK_BITS);

            return address - bar1;
        }

        if(bar == EXPANSION_ROM_BAR_ID)
        {
            return address - (m_ConfigData.ConfigHeader.ExpansionROMBaseAddress & EXPANSION_ROM_BAR_ADDRESS_MASK_BITS);
        }

        return address;
    }

    [[nodiscard]] u16 CommandRegister() const noexcept { return m_ConfigData.ConfigHeader.Command; }
    [[nodiscard]] bool ExpansionRomEnable() const noexcept
    {
        return (m_ConfigData.ConfigHeader.ExpansionROMBaseAddress & EXPANSION_ROM_BAR_ENABLE_BIT) == EXPANSION_ROM_BAR_ENABLE_BIT;
    }

#ifdef _WIN32
    // Intended only for VBDevice.
    void SetSimulationSyncEvent(HANDLE event) noexcept
    {
        m_SimulationSyncEvent = event;
    }
#endif

    [[nodiscard]] InterruptCallback_f& InterruptCallback() noexcept { return m_InterruptCallback; }
    [[nodiscard]] BusMasterReadCallback_f& BusMasterReadCallback() noexcept { return m_BusMasterReadCallback; }
    [[nodiscard]] BusMasterWriteCallback_f& BusMasterWriteCallback() noexcept { return m_BusMasterWriteCallback; }

    [[nodiscard]] VirtualBoxPciPhy<PciController>& VirtualBoxPciPhy() noexcept { return m_PciPhy; }
public:
    // PIPE Data Interface
    void ReceiveVirtualBoxPciPhy_RxData(const u32 index, const SerDesData& data) noexcept
    {
        (void) index;
        m_PhyInputFifo.SetWriteData(data.Data());
    }

    // PIPE Data Interface - SerDes
    void ReceiveVirtualBoxPciPhy_RxClock(const u32 index, const bool clock) noexcept
    {
        (void) index;
        p_RxClock = BOOL_TO_BIT(clock);

        {
            ::std::lock_guard lock(m_PhyInputFifoMutex);
            m_PhyInputFifo.SetWriteClock(clock);
        }

        TRIGGER_SENSITIVITY(p_RxClock);
    }

    // PIPE Command Interface
    void ReceiveVirtualBoxPciPhy_RefClockRequired(const u32 index, const bool required) noexcept { }
    void ReceiveVirtualBoxPciPhy_RxStandbyStatus(const u32 index, const bool status) noexcept { }

    // PIPE Status Interface
    void ReceiveVirtualBoxPciPhy_RxValid(const u32 index, const bool rxValid) noexcept
    {
        (void) index;
        m_PhyInputFifo.SetWriteIncoming(rxValid);
    }

    void ReceiveVirtualBoxPciPhy_PhyStatus(const u32 index, const bool phyStatus) noexcept { }
    void ReceiveVirtualBoxPciPhy_RxElectricalIdle(const u32 index, const bool electricalIdle) noexcept { }
    void ReceiveVirtualBoxPciPhy_RxStatus(const u32 index, const u8 rxStatus) noexcept { }
    void ReceiveVirtualBoxPciPhy_PowerPresent(const u32 index, const bool powerPresent) noexcept { }
    void ReceiveVirtualBoxPciPhy_ClockChangeOk(const u32 index, const bool clockChangeOk) noexcept { }

    // PIPE Message Bus Interface
    void ReceiveVirtualBoxPciPhy_P2M_MessageBus(const u32 index, const u8 p2mMessageBus) noexcept { }


    void ReceiveDualClockFIFO_WriteFull(const u32 index, const bool writeFull) noexcept { }

    void ReceiveDualClockFIFO_ReadData(const u32 index, const u32 data) noexcept
    {
    }

    void ReceiveDualClockFIFO_ReadEmpty(const u32 index, const bool readEmpty) noexcept
    {
    }

    void ReceiveDualClockFIFO_WriteAddress(const u32 index, const u64 writeAddress) noexcept { }
    void ReceiveDualClockFIFO_ReadAddress(const u32 index, const u64 readAddress) noexcept { }
private:
    PROCESSES_DECL()
    {
        PROCESS_ENTER(ClockHandler, p_Reset_n, p_Clock);
        PROCESS_ENTER(RxClockHandler, p_Reset_n, p_RxClock);
    }

    PROCESS_DECL(ClockHandler)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            InitConfigHeader();
            InitPcieCapabilityStructure();
            InitPowerManagementCapabilityStructure();
            InitMessageSignalledInterruptCapabilityStructure();
            InitAdvancedErrorReportingCapabilityStructure();
        }
        else if(RISING_EDGE(p_Clock))
        {

        }
    }

    PROCESS_DECL(RxClockHandler)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {

        }
        else if(RISING_EDGE(p_RxClock))
        {
        }
    }
private:
    /**
     * @brief Initializes the PCI configuration header.
     *
     * This method sets up the PCI configuration header with the appropriate default values:
     * - VendorID: Set to 0xFFFD. This is a unique identifier assigned by the PCI SIG to the vendor of the device.
     * - DeviceID: Set to 0x0001. This is a unique identifier assigned by the vendor to the device.
     * - Command: Set to 0x0000. This field is used to control the device's behavior.
     * - Status: Set to 0x0010. This field provides status information about the device.
     * - RevisionID: Set to 0x01. This field specifies the revision ID of the device.
     * - ClassCode: Set to 0x030001. This field specifies the class and subclass of the device.
     * - CacheLineSize: Set to 0x0. This field specifies the system cache line size in units of DWORDs.
     * - MasterLatencyTimer: Set to 0x0. This field specifies how long a device can retain control of the PCI bus.
     * - HeaderType: Set to 0x00. This field specifies the layout of the rest of the header.
     * - BIST: Set to 0x00. This field provides a mechanism for software to test the device.
     * - BAR0 to BAR5: These fields specify the base addresses for the device's memory and I/O ranges.
     *   - BAR0: Set to 0x00000000. This is a 32-bit non-prefetchable memory space.
     *   - BAR1: Set to 0x0000000C. This is a 64-bit prefetchable memory space.
     *   - BAR2: Set to 0x00000000. This is part of the 64-bit prefetchable memory space defined by BAR1.
     *   - BAR3 to BAR5: Set to 0x00000000. These are unused in this configuration.
     * - CardBusCISPointer: Set to 0x0. This field provides a pointer to the Card Information Structure for CardBus devices.
     * - SubsystemVendorID: Set to 0x0. This field specifies the vendor of the subsystem.
     * - SubsystemID: Set to 0x0. This field specifies the ID of the subsystem.
     * - ExpansionROMBaseAddress: Set to 0x00000000. This field specifies the base address of the expansion ROM.
     * - CapPointer: Set to the offset of the PcieCapability structure. This field provides a pointer to the device's capabilities list.
     * - Reserved0 and Reserved1: Set to 0x0. These fields are reserved.
     * - InterruptLine: Set to 0x0. This field specifies the interrupt line the device is connected to.
     * - InterruptPin: Set to 0x0. This field specifies the interrupt pin the device uses.
     * - MinGnt: Set to 0x00. This field specifies the minimum burst period of the device in units of PCI bus clocks.
     * - MaxLat: Set to 0x00. This field specifies the maximum latency of the device in units of PCI bus clocks.
     *
     * @note This method is called in the constructor of the PciController class.
     *
     * @return void
     */
    void InitConfigHeader() noexcept
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

    void InitPcieCapabilityStructure() noexcept
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

    void InitPowerManagementCapabilityStructure() noexcept
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

    void InitMessageSignalledInterruptCapabilityStructure() noexcept
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

    void InitAdvancedErrorReportingCapabilityStructure() noexcept
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

    void ExecuteMemRead() noexcept;
    void ExecuteMemWrite() noexcept;

    void ExecuteInterrupt() noexcept
    {
        if(!m_ConfigData.MessageSignalledInterruptCapability.MessageControl.Enabled)
        {
            return;
        }

        if(!m_InterruptSet)
        {
            return;
        }

        m_InterruptSet = false;

        if(!m_InterruptCallback)
        {
            return;
        }

        m_InterruptCallback(m_ConfigData.MessageSignalledInterruptCapability.MessageData);
    }
private:
    PciConfigData m_ConfigData;

    Receiver* m_Parent;

    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    u32 p_Pad0 : 30;

    //   This needs to be a separate word since it's being accessed from
    // another thread.
    u32 p_RxClock : 1;
    i32 p_Pad1 : 31;

    bool m_ReadRequestActive;
    u64 m_ReadRequestAddress;
    u16 m_ReadRequestSize;
    u32* m_ReadRequestResponseData;
    u16* m_ReadCountResponse;
    bool m_WriteRequestActive;
    u64 m_WriteRequestAddress;
    u16 m_WriteRequestSize;
    const u32* m_WriteRequestData;
    InterruptCallback_f m_InterruptCallback;
    BusMasterReadCallback_f m_BusMasterReadCallback;
    BusMasterWriteCallback_f m_BusMasterWriteCallback;

    u32 m_ReadState : 1;
    u32 m_WriteState : 1;
    u32 m_InterruptSet : 1;
    u32 m_Pad0 : 29; // NOLINT(clang-diagnostic-unused-private-field)

    riscv::fifo::DualClockFIFO<PciController, u32, 5> m_PhyInputFifo;
    riscv::fifo::DualClockFIFO<PciController, u32, 2> m_PhyOutputFifo;

    ::VirtualBoxPciPhy<PciController> m_PciPhy;

    #ifdef _WIN32
    HANDLE m_SimulationSyncEvent;
    #endif

    ::std::binary_semaphore m_SimulationSyncBinarySemaphore;
    ::std::mutex m_ReadDataMutex;
    ::std::mutex m_WriteDataMutex;
    ::std::mutex m_PhyInputFifoMutex;
};

/**
 * \brief A class of offsets for the all of the PCI headers.
 *
 * This is a class so that it can be a friend of the PciController.
 */
class PciConfigOffsets final
{
    DELETE_CONSTRUCT(PciConfigOffsets);
    DELETE_DESTRUCT(PciConfigOffsets);
    DELETE_CM(PciConfigOffsets);
public:
    static inline constexpr u16 ConfigHeaderBegin = 0;
    static inline constexpr u16 ConfigHeaderEnd = ConfigHeaderBegin + sizeof(PciConfigData::ConfigHeader);

    static inline constexpr u16 PciCapabilityOffsetBegin = offsetof(PciConfigData, PcieCapability);
    static inline constexpr u16 PciCapabilityOffsetEnd = PciCapabilityOffsetBegin + sizeof(PciConfigData::PcieCapability);

    static inline constexpr u16 PowerManagementCapabilityOffsetBegin = offsetof(PciConfigData, PowerManagementCapability);
    static inline constexpr u16 PowerManagementCapabilityOffsetEnd = PowerManagementCapabilityOffsetBegin + sizeof(PciConfigData::PowerManagementCapability);

    static inline constexpr u16 MessageSignalledInterruptsCapabilityOffsetBegin = offsetof(PciConfigData, MessageSignalledInterruptCapability);
    static inline constexpr u16 MessageSignalledInterruptsCapabilityOffsetEnd =
        MessageSignalledInterruptsCapabilityOffsetBegin + sizeof(PciConfigData::MessageSignalledInterruptCapability);

    static inline constexpr u16 PciConfigOffsetBegin = offsetof(PciConfigData, PciConfig);
    static inline constexpr u16 PciConfigOffsetEnd = PciConfigOffsetBegin + sizeof(PciConfigData::PciConfig);

    static inline constexpr u16 AdvancedErrorReportingCapabilityOffsetBegin = offsetof(PciConfigData, AdvancedErrorReportingCapability);
    static inline constexpr u16 AdvancedErrorReportingCapabilityOffsetEnd =
        AdvancedErrorReportingCapabilityOffsetBegin + sizeof(PciConfigData::AdvancedErrorReportingCapability);

    static inline constexpr u16 PciExtendedConfigOffsetBegin = offsetof(PciConfigData, PciExtendedConfig);
    static inline constexpr u16 PciExtendedConfigOffsetEnd = PciExtendedConfigOffsetBegin + sizeof(PciConfigData::PciExtendedConfig);
};

inline u32 PciController::ConfigRead(const u16 address, const u8 size) noexcept
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
