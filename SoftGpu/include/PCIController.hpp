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
    static inline constexpr bool WriteCompletionsOnWrites = false;
    static inline constexpr bool Write0xCCToBuffers = true;
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

    enum class EPciReadState : u32
    {
        Reset = 0,
        ReadTlpHeader = 0,
        ReadTransactionDescriptor,
        ReadHeaderDW3,
        ReadHeaderDW4,
        ReadData,
        Response
    };
public:
    explicit PciController(Receiver* const parent) noexcept
        : m_ConfigData{ }
        , m_Parent(parent)
        , p_Reset_n(0)
        , p_Clock(0)
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
        , m_PhyInputFifoEmpty(0)
        , m_PhyInputReceivedDataThisCycle(0)
        , m_PhyInputReceivingDataNextCycle(0)
        , m_PhyInputReadState(EPciReadState::Reset)
        , m_PhyInputDataBlobIndex(0)
        , m_Pad1{}
        , m_PhyInputData(0)
        , m_PhyInputRequestHeader()
        , m_PhyInputTransactionDescriptor(0)
        , m_PhyInputAddress(0)
        , m_PhyInputDataBlob{}
        , m_PhyOutputWriteState(0)
        , m_PhyOutputWriteLength(0)
        , m_Pad2{}
        , m_PhyOutputBuffer{}
        , m_PhyInputFifo(this, 0)
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

        m_PciPhy.SetTxWidth(2);
        m_PciPhy.SetRxWidth(2);

        m_PhyInputFifo.SetWriteResetN(true);
    }

    void SetResetN(const bool reset_n) noexcept
    {
        p_Reset_n = BOOL_TO_BIT(reset_n);

        m_PciPhy.SetResetN(reset_n);
        m_PhyInputFifo.SetReadResetN(reset_n);

        TRIGGER_SENSITIVITY(p_Reset_n);
    }

    void SetClock(const bool clock) noexcept
    {
        p_Clock = BOOL_TO_BIT(clock);

        m_PciPhy.SetClock(p_Clock);
        m_PhyInputFifo.SetReadClock(clock);

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

    void ConfigWrite(const u16 address, const u32 size, const u32 value) noexcept;

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
        if(index == 0)
        {
            m_PhyInputData = data;
        }
    }

    void ReceiveDualClockFIFO_ReadEmpty(const u32 index, const bool readEmpty) noexcept
    {
        if(index == 0)
        {
            m_PhyInputFifoEmpty = BOOL_TO_BIT(readEmpty);
        }
    }

    void ReceiveDualClockFIFO_WriteAddress([[maybe_unused]] const u32 index, [[maybe_unused]] const u64 writeAddress) noexcept { }
    void ReceiveDualClockFIFO_ReadAddress([[maybe_unused]] const u32 index, [[maybe_unused]] const u64 readAddress) noexcept { }
private:
    PROCESSES_DECL()
    {
        PROCESS_ENTER(ConfigResetHandler, p_Reset_n);
        PROCESS_ENTER(ClockHandler, p_Reset_n, p_Clock);
        PROCESS_ENTER(PhyOutputHandler, p_Reset_n, p_Clock);
    }

    PROCESS_DECL(ConfigResetHandler)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            InitConfigHeader();
            InitPcieCapabilityStructure();
            InitPowerManagementCapabilityStructure();
            InitMessageSignalledInterruptCapabilityStructure();
            InitAdvancedErrorReportingCapabilityStructure();
        }
    }

    PROCESS_DECL(ClockHandler)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_PhyInputReadState = EPciReadState::ReadTlpHeader;
            m_PhyInputReceivedDataThisCycle = 0;
            m_PhyInputFifo.SetReadIncoming(false);
        }
        else if(RISING_EDGE(p_Clock))
        {
            // Update whether we'll have an active word from the PHY next clock cycle.
            if(!BIT_TO_BOOL(m_PhyInputFifoEmpty))
            {
                m_PhyInputFifo.SetReadIncoming(true);
                m_PhyInputReceivedDataThisCycle = 1;
            }
            else
            {
                m_PhyInputFifo.SetReadIncoming(false);
                m_PhyInputReceivedDataThisCycle = 0;
            }

            if(BIT_TO_BOOL(m_PhyInputReceivedDataThisCycle) || m_PhyInputReadState == EPciReadState::Response)
            {
                switch(m_PhyInputReadState)
                {
                    case EPciReadState::ReadTlpHeader:
                        m_PhyInputRequestHeader = ::std::bit_cast<pcie::TlpHeader>(m_PhyInputData);
                        m_PhyInputReadState = EPciReadState::ReadTransactionDescriptor;
                        break;
                    case EPciReadState::ReadTransactionDescriptor:
                        m_PhyInputTransactionDescriptor = ::std::bit_cast<pcie::TlpTransactionDescriptor>(m_PhyInputData);
                        m_PhyInputReadState = EPciReadState::ReadHeaderDW3;
                        break;
                    case EPciReadState::ReadHeaderDW3:
                        m_PhyInputDW3 = m_PhyInputData;

                        if(m_PhyInputRequestHeader.Fmt == pcie::TlpHeader::FORMAT_3_DW_HEADER_NO_DATA)
                        {
                            // If we only have a 3 word header then we skip reading header word 4.
                            m_PhyInputReadState = EPciReadState::Response;
                        }
                        else if(m_PhyInputRequestHeader.Fmt == pcie::TlpHeader::FORMAT_3_DW_HEADER_WITH_DATA)
                        {
                            // If we only have a 3 word header then we skip reading header word 4.
                            m_PhyInputReadState = EPciReadState::ReadData;
                        }
                        else
                        {
                            m_PhyInputReadState = EPciReadState::ReadHeaderDW4;
                        }
                        break;
                    case EPciReadState::ReadHeaderDW4:
                        m_PhyInputDW4 = m_PhyInputData;
                        m_PhyInputReadState = EPciReadState::ReadData;
                        break;
                    case EPciReadState::ReadData:
                    {
                        m_PhyInputDataBlob[m_PhyInputDataBlobIndex] = m_PhyInputData;
                        ++m_PhyInputDataBlobIndex;

                        if(m_PhyInputDataBlobIndex >= m_PhyInputRequestHeader.Length())
                        {
                            m_PhyInputReadState = EPciReadState::Response;
                        }
                    }
                    case EPciReadState::Response:
                    {
                        if(m_PhyOutputWriteLength != 0)
                        {
                            break;
                        }

                        if(m_PhyInputRequestHeader.Fmt == pcie::TlpHeader::FORMAT_3_DW_HEADER_NO_DATA &&
                           m_PhyInputRequestHeader.Type == pcie::TlpHeader::TYPE_CONFIG_TYPE_0_REQUEST)
                        {
                            HandleConfigRead();
                            m_PhyInputReadState = EPciReadState::Reset;
                        }
                        else if(m_PhyInputRequestHeader.Fmt == pcie::TlpHeader::FORMAT_3_DW_HEADER_WITH_DATA &&
                                m_PhyInputRequestHeader.Type == pcie::TlpHeader::TYPE_CONFIG_TYPE_0_REQUEST)
                        {
                            HandleConfigWrite();
                            m_PhyInputReadState = EPciReadState::Reset;
                        }
                        else if((m_PhyInputRequestHeader.Fmt == pcie::TlpHeader::FORMAT_3_DW_HEADER_NO_DATA ||
                                 m_PhyInputRequestHeader.Fmt == pcie::TlpHeader::FORMAT_4_DW_HEADER_NO_DATA) &&
                                m_PhyInputRequestHeader.Type == pcie::TlpHeader::TYPE_MEMORY_REQUEST)
                        {
                            m_PhyInputReadState = EPciReadState::Reset;
                        }
                        else if((m_PhyInputRequestHeader.Fmt == pcie::TlpHeader::FORMAT_3_DW_HEADER_WITH_DATA ||
                                 m_PhyInputRequestHeader.Fmt == pcie::TlpHeader::FORMAT_4_DW_HEADER_WITH_DATA) &&
                                m_PhyInputRequestHeader.Type == pcie::TlpHeader::TYPE_MEMORY_REQUEST)
                        {
                            m_PhyInputReadState = EPciReadState::Reset;
                        }
                        else
                        {
                            m_PhyInputReadState = EPciReadState::Reset;
                        }

                        if constexpr(Write0xCCToBuffers)
                        {
                            m_PhyInputRequestHeader = ::std::bit_cast<pcie::TlpHeader>(0xCCCCCCCC);
                            m_PhyInputTransactionDescriptor = ::std::bit_cast<pcie::TlpTransactionDescriptor>(0xCCCCCCCC);
                            m_PhyInputDW3 = 0xCCCCCCCC;
                            m_PhyInputDW4 = 0xCCCCCCCC;
                            (void) ::std::memset(m_PhyInputDataBlob, 0xCC, sizeof(m_PhyInputDataBlob));
                        }
                        break;
                    }
                    default:
                        m_PhyInputReadState = EPciReadState::Reset;
                        break;
                }
            }
        }
    }

    PROCESS_DECL(PhyOutputHandler)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_PhyOutputWriteState = 0;
            m_PhyOutputWriteLength = 0;

            if constexpr(true)
            {
                (void) ::std::memset(m_PhyOutputBuffer, 0xCC, sizeof(m_PhyOutputBuffer));
            }
        }
        else if(RISING_EDGE(p_Clock))
        {
            if(m_PhyOutputWriteLength == 0)
            {
                m_PciPhy.SetTxDataValid(false);
                if constexpr(Write0xCCToBuffers)
                {
                    m_PciPhy.SetTxData(0xCCCCCCCC);
                }

                // Trigger the simulation event on the next clock cycle.
                if(m_PhyOutputWriteState != 0)
                {
                    m_PciPhy.SignalSimulationSyncEvent();
                    m_PhyOutputWriteState = 0;
                }
                return;
            }

            m_PciPhy.SetTxData(m_PhyOutputBuffer[m_PhyOutputWriteState]);
            m_PciPhy.SetTxDataValid(true);
            ++m_PhyOutputWriteState;

            if(m_PhyOutputWriteState >= m_PhyOutputWriteLength)
            {
                m_PhyOutputWriteLength = 0;

                if constexpr(Write0xCCToBuffers)
                {
                    (void) ::std::memset(m_PhyOutputBuffer, 0xCC, sizeof(m_PhyOutputBuffer));
                }
            }
        }
    }

    void HandleConfigRead() noexcept
    {
        pcie::TlpHeader responseHeader { };
        responseHeader.Fmt = pcie::TlpHeader::FORMAT_3_DW_HEADER_WITH_DATA;
        responseHeader.Type = pcie::TlpHeader::TYPE_COMPLETION;
        responseHeader.TC = 0;
        responseHeader.TD = 0;
        responseHeader.EP = 0;
        responseHeader.Attr = 0;
        responseHeader.Length(1);

        m_PhyOutputBuffer[0] = ::std::bit_cast<u32>(responseHeader);

        {
            pcie::TlpCompletionHeader0 configHeader { };
            configHeader.BusNumber = 0;
            configHeader.DeviceNumber = 0;
            configHeader.FunctionNumber = 0;
            configHeader.CompletionStatus = pcie::TlpCompletionHeader0::SuccessfulCompletion;
            configHeader.ByteCountModified = 0;
            configHeader.ByteCount(
                pcie::TlpTransactionDescriptor::ByteEnableToByteCount(m_PhyInputTransactionDescriptor.FirstDwordByteEnable)
            );

            m_PhyOutputBuffer[1] = ::std::bit_cast<u32>(configHeader);
        }

        {
            pcie::TlpCompletionHeader1 configHeader { };
            configHeader.BusNumber = m_PhyInputTransactionDescriptor.BusNumber;
            configHeader.DeviceNumber = m_PhyInputTransactionDescriptor.DeviceNumber;
            configHeader.FunctionNumber = m_PhyInputTransactionDescriptor.FunctionNumber;
            configHeader.Tag = m_PhyInputTransactionDescriptor.Tag;
            configHeader.LowerAddress = 0;

            m_PhyOutputBuffer[2] = ::std::bit_cast<u32>(configHeader);
        }

        {
            const u32 configValue = ConfigRead(
                m_PhyInputConfigRequestHeader.ExtendedRegisterNumber << 6 | m_PhyInputConfigRequestHeader.RegisterNumber,
                pcie::TlpTransactionDescriptor::ByteEnableToByteCount(m_PhyInputTransactionDescriptor.FirstDwordByteEnable)
            );

            m_PhyOutputBuffer[3] = configValue;
        }

        m_PhyOutputWriteLength = 4;
    }

    void HandleConfigWrite() noexcept
    {
        if constexpr(WriteCompletionsOnWrites)
        {
            pcie::TlpHeader responseHeader { };
            responseHeader.Fmt = pcie::TlpHeader::FORMAT_3_DW_HEADER_NO_DATA;
            responseHeader.Type = pcie::TlpHeader::TYPE_COMPLETION;
            responseHeader.TC = 0;
            responseHeader.TD = 0;
            responseHeader.EP = 0;
            responseHeader.Attr = 0;
            responseHeader.Length(1);

            m_PhyOutputBuffer[0] = ::std::bit_cast<u32>(responseHeader);

            {
                pcie::TlpCompletionHeader0 configHeader { };
                configHeader.BusNumber = 0;
                configHeader.DeviceNumber = 0;
                configHeader.FunctionNumber = 0;
                configHeader.CompletionStatus = pcie::TlpCompletionHeader0::SuccessfulCompletion;
                configHeader.ByteCountModified = 0;
                configHeader.ByteCount(
                    pcie::TlpTransactionDescriptor::ByteEnableToByteCount(m_PhyInputTransactionDescriptor.FirstDwordByteEnable)
                );

                m_PhyOutputBuffer[1] = ::std::bit_cast<u32>(configHeader);
            }

            {
                pcie::TlpCompletionHeader1 configHeader { };
                configHeader.BusNumber = m_PhyInputTransactionDescriptor.BusNumber;
                configHeader.DeviceNumber = m_PhyInputTransactionDescriptor.DeviceNumber;
                configHeader.FunctionNumber = m_PhyInputTransactionDescriptor.FunctionNumber;
                configHeader.Tag = m_PhyInputTransactionDescriptor.Tag;
                configHeader.LowerAddress = 0;

                m_PhyOutputBuffer[2] = ::std::bit_cast<u32>(configHeader);
            }

            m_PhyOutputWriteLength = 3;
        }
        else
        {
            m_PhyOutputWriteLength = 0;
        }

        // For config writes we should've only received one word.
        assert(m_PhyInputDataBlobIndex == 1);

        {
            ConfigWrite(
                m_PhyInputConfigRequestHeader.ExtendedRegisterNumber << 6 | m_PhyInputConfigRequestHeader.RegisterNumber,
                pcie::TlpTransactionDescriptor::ByteEnableToByteCount(m_PhyInputTransactionDescriptor.FirstDwordByteEnable),
                m_PhyInputDataBlob[0]
            );
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
    void InitConfigHeader() noexcept;

    void InitPcieCapabilityStructure() noexcept;

    void InitPowerManagementCapabilityStructure() noexcept;

    void InitMessageSignalledInterruptCapabilityStructure() noexcept;

    void InitAdvancedErrorReportingCapabilityStructure() noexcept;

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
    [[maybe_unused]] u32 p_Pad0 : 30;

    //   This needs to be a separate word since it's being accessed from
    // another thread.
    u32 p_RxClock : 1;
    [[maybe_unused]] i32 p_Pad1 : 31;

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
    [[maybe_unused]] u32 m_Pad0 : 29;

    u32 m_PhyInputFifoEmpty : 1;
    u32 m_PhyInputReceivedDataThisCycle : 1;
    u32 m_PhyInputReceivingDataNextCycle : 1;
    EPciReadState m_PhyInputReadState : 3;
    u32 m_PhyInputDataBlobIndex : 10;
    [[maybe_unused]] u32 m_Pad1 : 16;

    u32 m_PhyInputData;
    pcie::TlpHeader m_PhyInputRequestHeader;
    pcie::TlpTransactionDescriptor m_PhyInputTransactionDescriptor;
    union
    {
        u64 m_PhyInputAddress;
        pcie::TlpConfigRequestHeader m_PhyInputConfigRequestHeader;
        struct
        {
            u32 m_PhyInputDW3;
            u32 m_PhyInputDW4;
        };
    };

    u32 m_PhyInputDataBlob[1024];

    u32 m_PhyOutputWriteState : 11;
    u32 m_PhyOutputWriteLength : 11;
    u32 m_Pad2 : 10;

    u32 m_PhyOutputBuffer[1024 + 4];

    riscv::fifo::DualClockFIFO<PciController, u32, 11> m_PhyInputFifo;

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

#include "PCIController.inl"
