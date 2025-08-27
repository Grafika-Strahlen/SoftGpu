/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <Common.hpp>
#include <PcieProtocol.hpp>
#include <riscv/DualClockFIFO/DualClockFIFO.hpp>

#include <semaphore>

struct SerDesData final
{
    DEFAULT_CONSTRUCT_PU(SerDesData);
    DEFAULT_DESTRUCT(SerDesData);
    DEFAULT_CM_PU(SerDesData);
public:
    u64 Data0 : 8;
    u64 DataExtra0 : 2;
    u64 Data1 : 8;
    u64 DataExtra1 : 2;
    u64 Data2 : 8;
    u64 DataExtra2 : 2;
    u64 Data3 : 8;
    u64 DataExtra3 : 2;
    u64 Pad0 : 24;

    SerDesData(const u32 data) noexcept
        : Data0(data & 0xFF)
        , DataExtra0(0)
        , Data1((data >> 8) & 0xFF)
        , DataExtra1(0)
        , Data2((data >> 16) & 0xFF)
        , DataExtra2(0)
        , Data3((data >> 24) & 0xFF)
        , DataExtra3(0)
        , Pad0{}
    { }

    SerDesData& operator=(const u32 data) noexcept
    {
        Data0 = data & 0xFF;
        Data1 = (data >> 8) & 0xFF;
        Data2 = (data >> 16) & 0xFF;
        Data3 = (data >> 24) & 0xFF;

        return *this;
    }

    [[nodiscard]] u32 Data() const noexcept
    {
        return Data3 << 24 | Data2 << 16 | Data1 << 8 | Data0;
    }
};

class VirtualBoxPciPhyReceiverSample
{
public:
    // PIPE Data Interface
    void ReceiveVirtualBoxPciPhy_RxData(const u32 index, const SerDesData& data) noexcept { }

    // PIPE Data Interface - SerDes
    void ReceiveVirtualBoxPciPhy_RxClock(const u32 index, const bool clock) noexcept { }

    // PIPE Command Interface
    void ReceiveVirtualBoxPciPhy_RefClockRequired(const u32 index, const bool required) noexcept { }
    void ReceiveVirtualBoxPciPhy_RxStandbyStatus(const u32 index, const bool status) noexcept { }

    // PIPE Status Interface
    void ReceiveVirtualBoxPciPhy_RxValid(const u32 index, const bool rxValid) noexcept { }
    void ReceiveVirtualBoxPciPhy_PhyStatus(const u32 index, const bool phyStatus) noexcept { }
    void ReceiveVirtualBoxPciPhy_RxElectricalIdle(const u32 index, const bool electricalIdle) noexcept { }
    void ReceiveVirtualBoxPciPhy_RxStatus(const u32 index, const u8 rxStatus) noexcept { }
    void ReceiveVirtualBoxPciPhy_PowerPresent(const u32 index, const bool powerPresent) noexcept { }
    void ReceiveVirtualBoxPciPhy_ClockChangeOk(const u32 index, const bool clockChangeOk) noexcept { }

    // PIPE Message Bus Interface
    void ReceiveVirtualBoxPciPhy_P2M_MessageBus(const u32 index, const u8 p2mMessageBus) noexcept { }
};

template<typename Receiver = VirtualBoxPciPhyReceiverSample>
class VirtualBoxPciPhy final
{
    DEFAULT_DESTRUCT(VirtualBoxPciPhy);
    DELETE_CM(VirtualBoxPciPhy);
public:
    static inline constexpr u8 MESSAGE_BUS_COMMAND_NOP               = 0b0000;
    static inline constexpr u8 MESSAGE_BUS_COMMAND_WRITE_UNCOMMITTED = 0b0001;
    static inline constexpr u8 MESSAGE_BUS_COMMAND_WRITE_COMMITTED   = 0b0010;
    static inline constexpr u8 MESSAGE_BUS_COMMAND_READ              = 0b0011;
    static inline constexpr u8 MESSAGE_BUS_COMMAND_READ_COMPLETION   = 0b0100;
    static inline constexpr u8 MESSAGE_BUS_COMMAND_WRITE_ACKNOWLEDGE = 0b0101;
public:
    SENSITIVITY_DECL(p_Reset_n, p_Clock);

    SIGNAL_ENTITIES();
public:
    explicit VirtualBoxPciPhy(
        Receiver* const parent,
        const u32 index = 0
    ) noexcept
        : m_Parent(parent)
        , m_Index(index)
        , p_Clock(0)
        , p_Reset_n(0)
        , m_Pad0{}
        , p_TxData{}
        , p_TxDataValid(0)
        , m_Pad1{}
        , p_TxDetectRx(0)
        , p_TxElectricalIdle(0)
        , p_PowerDown(0)
        , p_RxElectricalDetectDisable(0)
        , p_TxCommonModeDisable(0)
        , p_DataRate(0)
        , p_TxWidth(0)
        , p_ClockRate(0)
        , p_RxTermination(0)
        , p_RxStandby(0)
        , p_RxWidth(0)
        , m_Pad2{}
        , p_ClockChangeAcknowledge(0)
        , p_AsyncPowerChangeAcknowledge(0)
        , m_Pad3{}
        , p_M2P_MessageBus(0)
        , m_M2P_MessageBusCommand(0)
        , m_M2P_MessageBusAddressHigh(0)
        , m_M2P_MessageBusAddressLow(0)
        , m_M2P_MessageBusData(0)
        , m_M2P_MessageBusState(0)
        , m_Pad4{}
        , m_TxConstructWord(0)
        , m_TxConstructWordState(0)
        , m_PciSendReadEmpty(0)
        , m_Pad5{}
        , m_TxResponseDwordFromFifo(0)
        , m_PciSendFifo(this, 1)
        , m_SimulationSyncBinarySemaphore(0)
        , m_ReadDataMutex()
        , m_PhyOutDataMutex()
    { }

    void SetResetN(const bool reset_n) noexcept
    {
        p_Reset_n = BOOL_TO_BIT(reset_n);

        m_PciSendFifo.SetWriteResetN(reset_n);

        TRIGGER_SENSITIVITY(p_Reset_n);
    }

    void SetClock(const bool clock) noexcept
    {
        p_Clock = BOOL_TO_BIT(clock);

        {
            // We need to interlock on the clock events for the FIFOs.
            ::std::lock_guard lock(m_PhyOutDataMutex);
            m_PciSendFifo.SetWriteClock(clock);
        }

        TRIGGER_SENSITIVITY(p_Clock);
    }

    void SetTxData(const SerDesData txData) noexcept
    {
        p_TxData = txData;
    }

    void SetTxDataValid(const bool txDataValid) noexcept
    {
        p_TxDataValid = BOOL_TO_BIT(txDataValid);
    }

    void SetTxDetectRx(const bool txDetectRx) noexcept
    {
        p_TxDetectRx = BOOL_TO_BIT(txDetectRx);
    }

    void SetTxElectricalIdle(const u32 txElectricalIdle) noexcept
    {
        p_TxElectricalIdle = txElectricalIdle;
    }

    void SetPowerDown(const u32 powerDown) noexcept
    {
        p_PowerDown = powerDown;
    }

    void SetRxElectricalDetectDisable(const bool rxElectricalDetectDisable) noexcept
    {
        p_RxElectricalDetectDisable = BOOL_TO_BIT(rxElectricalDetectDisable);
    }

    void SetTxCommonModeDisable(const bool txCommonModeDisable) noexcept
    {
        p_TxCommonModeDisable = BOOL_TO_BIT(txCommonModeDisable);
    }

    void SetDataRate(const u32 dataRate) noexcept
    {
        p_DataRate = dataRate;
    }

    void SetTxWidth(const u32 txWidth) noexcept
    {
        p_TxWidth = txWidth;
    }

    void SetClockRate(const u32 clockRate) noexcept
    {
        p_ClockRate = clockRate;
    }

    void SetRxTermination(const bool rxTermination) noexcept
    {
        p_RxTermination = BOOL_TO_BIT(rxTermination);
    }

    void SetRxStandBy(const bool rxStandBy) noexcept
    {
        p_RxStandby = BOOL_TO_BIT(rxStandBy);
    }

    void SetRxWidth(const u32 rxWidth) noexcept
    {
        p_RxWidth = rxWidth;
    }

    void SetClockChangeAcknowledge(const bool clockChangeAcknowledge) noexcept
    {
        p_ClockChangeAcknowledge = BOOL_TO_BIT(clockChangeAcknowledge);
    }

    void SetAsyncPowerChangeAcknowledge(const bool asyncPowerChangeAcknowledge) noexcept
    {
        p_AsyncPowerChangeAcknowledge = BOOL_TO_BIT(asyncPowerChangeAcknowledge);
    }

    void SetM2P_MessageBus(const u8 messageBus) noexcept
    {
        p_M2P_MessageBus = messageBus;
    }

    void SignalSimulationSyncEvent() noexcept
    {
        m_SimulationSyncBinarySemaphore.release();
    }
public:
    void VirtualBoxMemReadSet(const u64 address, const u16 size, u32* const data, u16* const readResponse)
    {
        SendReadToSoftGpu(
            address,
            size
        );

        if(!m_SimulationSyncBinarySemaphore.try_acquire_for(::std::chrono::milliseconds(250)))
        {
            return;
        }

        SpinOnPciSendFifo();

        // Read the first word from a transaction.
        ReadWordFromPciSendFifo<true>();

        // Cast the first word to the TLP Header.
        const auto header = ::std::bit_cast<pcie::TlpHeader>(m_TxResponseDwordFromFifo);

        // If it isn't a completion header we have a problem.
        if(header.Type != pcie::TlpHeader::TYPE_COMPLETION && header.Type != pcie::TlpHeader::TYPE_COMPLETION_LOCKED_READ)
        {
            assert(header.Type == pcie::TlpHeader::TYPE_COMPLETION || header.Type == pcie::TlpHeader::TYPE_COMPLETION_LOCKED_READ);
            return;
        }

        // There should never be a 4 DW header for completion responses.
        if(header.Fmt == pcie::TlpHeader::FORMAT_4_DW_HEADER_NO_DATA || header.Fmt == pcie::TlpHeader::FORMAT_4_DW_HEADER_WITH_DATA)
        {
            assert(header.Fmt != pcie::TlpHeader::FORMAT_4_DW_HEADER_NO_DATA && header.Fmt != pcie::TlpHeader::FORMAT_4_DW_HEADER_WITH_DATA);
            return;
        }

        // Read 2 more words of the header.
        SpinOnPciSendFifo();
        ReadWordFromPciSendFifo();
        SpinOnPciSendFifo();
        ReadWordFromPciSendFifo<true>();

        if(header.Fmt == pcie::TlpHeader::FORMAT_3_DW_HEADER_NO_DATA)
        {
            // With no data we can exit.
            return;
        }
        else if(header.Fmt == pcie::TlpHeader::FORMAT_3_DW_HEADER_WITH_DATA)
        {
            for(u16 i = 0; i < header.Length; ++i)
            {
                SpinOnPciSendFifo();
                ReadWordFromPciSendFifo();

                data[i] = m_TxResponseDwordFromFifo;
            }

            StopReadingPciSendFifo();
            *readResponse = header.Length;
        }
        else
        {
            assert(false);
        }
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    void VirtualBoxMemWriteSet(const u64 address, const u16 size, const u32* const data) noexcept
    {
        SendWriteToSoftGpu(
            address,
            size,
            data
        );
    }
public:
    void ReceiveDualClockFIFO_WriteFull(const u32 index, const bool writeFull) noexcept { }

    void ReceiveDualClockFIFO_ReadData(const u32 index, const u32 data) noexcept
    {
        if(index == 1)
        {
            m_TxResponseDwordFromFifo = data;
        }
    }

    void ReceiveDualClockFIFO_ReadEmpty(const u32 index, const bool readEmpty) noexcept
    {
        if(index == 1)
        {
            m_PciSendReadEmpty = BOOL_TO_BIT(readEmpty);
        }
    }

    void ReceiveDualClockFIFO_WriteAddress(const u32 index, const u64 writeAddress) noexcept { }
    void ReceiveDualClockFIFO_ReadAddress(const u32 index, const u64 readAddress) noexcept { }
private:
    PROCESSES_DECL()
    {
        PROCESS_ENTER(M2P_MessageBusHandler, p_Reset_n, p_Clock);
        PROCESS_ENTER(TxWriteHandler, p_Reset_n, p_Clock);
    }

    PROCESS_DECL(M2P_MessageBusHandler)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_M2P_MessageBusCommand = 0;
            m_M2P_MessageBusAddressHigh = 0;
            m_M2P_MessageBusAddressLow = 0;
            m_M2P_MessageBusState = 0;
        }
        else if(RISING_EDGE(p_Clock))
        {
            switch(m_M2P_MessageBusState)
            {
                case 0:
                    if((p_M2P_MessageBus >> 4) != MESSAGE_BUS_COMMAND_NOP)
                    {
                        m_M2P_MessageBusCommand = p_M2P_MessageBus >> 4;
                        m_M2P_MessageBusAddressHigh = p_M2P_MessageBus & 0xF;
                        m_M2P_MessageBusState = 1;
                    }
                    break;
                case 1:
                    m_M2P_MessageBusAddressLow = p_M2P_MessageBus;
                    m_M2P_MessageBusState = 2;
                    break;
                case 2:
                    m_M2P_MessageBusData = p_M2P_MessageBus;
                    m_M2P_MessageBusState = 0;
                    break;
                default: break;
            }
        }
    }

    PROCESS_DECL(TxWriteHandler)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_TxConstructWord = 0;
            m_TxConstructWordState = 0;
        }
        else if(RISING_EDGE(p_Clock))
        {
            if(BIT_TO_BOOL(p_TxDataValid))
            {
                if(p_TxWidth == 2)
                {
                    const u32 data = (p_TxData.Data3 << 24) | (p_TxData.Data2 << 16) | (p_TxData.Data1 << 8) | (p_TxData.Data0);

                    m_PciSendFifo.SetWriteIncoming(true);
                    m_PciSendFifo.SetWriteData(data);
                }
                else if(p_TxWidth == 1)
                {
                    if(m_TxConstructWordState == 0)
                    {
                        m_TxConstructWord = (p_TxData.Data1 << 8) | (p_TxData.Data0);
                        m_TxConstructWordState = 1;
                    }
                    else
                    {
                        const u32 data = (p_TxData.Data2 << 24) | (p_TxData.Data1 << 16) | m_TxConstructWord;

                        m_PciSendFifo.SetWriteIncoming(true);
                        m_PciSendFifo.SetWriteData(data);

                        m_TxConstructWord = 0;
                        m_TxConstructWordState = 0;
                    }
                }
                else if(p_TxWidth == 0)
                {
                    switch(m_TxConstructWordState)
                    {
                        case 0:
                            m_TxConstructWord = p_TxData.Data0;
                            m_TxConstructWordState = 1;
                            break;
                        case 1:
                            m_TxConstructWord |= p_TxData.Data0 << 8;
                            m_TxConstructWordState = 2;
                            break;
                        case 2:
                            m_TxConstructWord |= p_TxData.Data0 << 16;
                            m_TxConstructWordState = 3;
                            break;
                        case 3:
                        {
                            const u32 data = (p_TxData.Data0 << 24) | m_TxConstructWord;

                            m_PciSendFifo.SetWriteIncoming(true);
                            m_PciSendFifo.SetWriteData(data);

                            m_TxConstructWord = 0;
                            m_TxConstructWordState = 0;
                            break;
                        }
                        default: break;
                    }
                }
            }
            else
            {
                m_PciSendFifo.SetWriteIncoming(false);
            }
        }
    }
private:
    /**
     * Spins on the PciSendFifo until it has data.
     *   This has to be done since technically this code is operating from
     * VirtualBox's CPU's PHY.
     */
    void SpinOnPciSendFifo()
    {
        //   I don't think this can trigger affect any behaviour in the
        // write domain without a clock event in the read domain.
        m_PciSendFifo.SetReadIncoming(false);

        // Wait until we have data to read.
        while(m_PciSendReadEmpty)
        {
            ::std::lock_guard lock(m_PhyOutDataMutex);
            m_PciSendFifo.SetReadClock(true);
            m_PciSendFifo.SetReadClock(false);
        }
    }

    /**
     * Reads one word from the PciSendFifo.
     *   This has to be done since technically this code is operating from
     * VirtualBox's CPU's PHY.
     */
    template<bool CloseOnExit = false>
    void ReadWordFromPciSendFifo()
    {
        // This always needs to be set since we always have to spin before reading.
        //   I don't think this can trigger affect any behaviour in the
        // write domain without a clock event in the read domain.
        m_PciSendFifo.SetReadIncoming(true);

        {
            ::std::lock_guard lock(m_PhyOutDataMutex);

            m_PciSendFifo.SetReadClock(true);
            m_PciSendFifo.SetReadClock(false);
        }

        // If requested we'll make sure that there is not another read incoming.
        if constexpr(CloseOnExit)
        {
            //   I don't think this can trigger affect any behaviour in the
            // write domain without a clock event in the read domain.
            m_PciSendFifo.SetReadIncoming(false);
        }
    }

    void StopReadingPciSendFifo()
    {
        //   I don't think this can trigger affect any behaviour in the
        // write domain without a clock event in the read domain.
        // ::std::lock_guard lock(m_PhyOutDataMutex);
        m_PciSendFifo.SetReadIncoming(false);
    }

    void SendMemoryRequestHeaderToSoftGpu(
        const pcie::TlpHeader::EFormat fmt,
        const u64 address,
        const u16 size
    ) const noexcept
    {
        pcie::TlpHeader header {};
        header.Fmt = fmt;
        header.TC = 0;
        header.Attr = 0;
        header.TD = 0;
        header.EP = 0;
        header.Length = size;

        SerDesData serDesData = ::std::bit_cast<u32>(header);

        m_Parent->ReceiveVirtualBoxPciPhy_RxData(m_Index, serDesData);
        m_Parent->ReceiveVirtualBoxPciPhy_RxValid(m_Index, true);
        m_Parent->ReceiveVirtualBoxPciPhy_RxClock(m_Index, true);
        m_Parent->ReceiveVirtualBoxPciPhy_RxClock(m_Index, false);

        serDesData = 0;

        m_Parent->ReceiveVirtualBoxPciPhy_RxData(m_Index, serDesData);
        m_Parent->ReceiveVirtualBoxPciPhy_RxClock(m_Index, true);
        m_Parent->ReceiveVirtualBoxPciPhy_RxClock(m_Index, false);

        if(address <= 0xFFFFFFFF)
        {
            serDesData = static_cast<u32>(address & 0xFFFFFFFC);

            m_Parent->ReceiveVirtualBoxPciPhy_RxData(m_Index, serDesData);
            m_Parent->ReceiveVirtualBoxPciPhy_RxClock(m_Index, true);
            m_Parent->ReceiveVirtualBoxPciPhy_RxClock(m_Index, false);
        }
        else
        {
            serDesData = static_cast<u32>(address >> 32);

            m_Parent->ReceiveVirtualBoxPciPhy_RxData(m_Index, serDesData);
            m_Parent->ReceiveVirtualBoxPciPhy_RxClock(m_Index, true);
            m_Parent->ReceiveVirtualBoxPciPhy_RxClock(m_Index, false);

            serDesData = static_cast<u32>(address & 0xFFFFFFFC);

            m_Parent->ReceiveVirtualBoxPciPhy_RxData(m_Index, serDesData);
            m_Parent->ReceiveVirtualBoxPciPhy_RxClock(m_Index, true);
            m_Parent->ReceiveVirtualBoxPciPhy_RxClock(m_Index, false);
        }
    }

    void SendReadToSoftGpu(
        const u64 address,
        const u16 size
    ) const noexcept
    {
        if(address <= 0xFFFFFFFF)
        {
            SendMemoryRequestHeaderToSoftGpu(
                pcie::TlpHeader::FORMAT_3_DW_HEADER_NO_DATA,
                address,
                size
            );
        }
        else
        {
            SendMemoryRequestHeaderToSoftGpu(
                pcie::TlpHeader::FORMAT_4_DW_HEADER_NO_DATA,
                address,
                size
            );
        }

        m_Parent->ReceiveVirtualBoxPciPhy_RxValid(m_Index, false);
    }

    void SendWriteToSoftGpu(
        const u64 address,
        const u16 size,
        const u32* const data
    ) const noexcept
    {
        if(address <= 0xFFFFFFFF)
        {
            SendMemoryRequestHeaderToSoftGpu(
                pcie::TlpHeader::FORMAT_3_DW_HEADER_WITH_DATA,
                address,
                size
            );
        }
        else
        {
            SendMemoryRequestHeaderToSoftGpu(
                pcie::TlpHeader::FORMAT_4_DW_HEADER_WITH_DATA,
                address,
                size
            );
        }

        for(u16 i = 0; i < size; ++i)
        {
            const SerDesData serDesData = data[i];

            m_Parent->ReceiveVirtualBoxPciPhy_RxData(m_Index, serDesData);
            m_Parent->ReceiveVirtualBoxPciPhy_RxClock(m_Index, true);
            m_Parent->ReceiveVirtualBoxPciPhy_RxClock(m_Index, false);
        }

        m_Parent->ReceiveVirtualBoxPciPhy_RxValid(m_Index, false);
    }
private:
    Receiver* m_Parent;
    u32 m_Index;

    u32 p_Clock : 1;
    u32 p_Reset_n : 1;
    u32 m_Pad0 : 32;

    // PIPE Data Interface
    SerDesData p_TxData;
    u32 p_TxDataValid : 1;
    u32 m_Pad1 : 31;

    // PIPE Command Interface
    u32 p_TxDetectRx : 1;
    u32 p_TxElectricalIdle : 4;
    u32 p_PowerDown : 4;
    u32 p_RxElectricalDetectDisable : 1;
    u32 p_TxCommonModeDisable : 1;
    u32 p_DataRate : 4;
    u32 p_TxWidth : 2; // 0 - 10 bits, 1 - 20 bits, 2 - 40 bits
    u32 p_ClockRate : 5;
    u32 p_RxTermination : 1;
    u32 p_RxStandby : 1;
    // PIPE Command Interface - SerDes
    u32 p_RxWidth : 2; // 0 - 10 bits, 1 - 20 bits, 2 - 40 bits
    u32 m_Pad2 : 6;

    // PIPE Status Interface
    u32 p_ClockChangeAcknowledge : 1;
    u32 p_AsyncPowerChangeAcknowledge : 1;
    u32 m_Pad3 : 30;

    // PIPE Message Bus Interface
    u8 p_M2P_MessageBus;

    u32 m_M2P_MessageBusCommand : 4;
    u32 m_M2P_MessageBusAddressHigh : 4;
    u32 m_M2P_MessageBusAddressLow : 8;
    u32 m_M2P_MessageBusData : 8;
    u32 m_M2P_MessageBusState : 2;
    u32 m_Pad4 : 6;

    u32 m_TxConstructWord;
    u32 m_TxConstructWordState : 2;
    u32 m_PciSendReadEmpty : 1;
    u32 m_Pad5 : 29;

    u32 m_TxResponseDwordFromFifo;

    riscv::fifo::DualClockFIFO<VirtualBoxPciPhy, u32, 5> m_PciSendFifo;

    ::std::binary_semaphore m_SimulationSyncBinarySemaphore;
    ::std::mutex m_ReadDataMutex;
    ::std::mutex m_PhyOutDataMutex;
};
