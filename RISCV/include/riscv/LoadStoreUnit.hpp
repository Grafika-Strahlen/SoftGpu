/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <Common.hpp>
#include <Objects.hpp>
#include <NumTypes.hpp>
#include "ControlBus.hpp"
#include "MemoryBus.hpp"

namespace riscv {

class LoadStoreUnitReceiverSample
{
public:
    void ReceiveLoadStoreUnit_ReadData([[maybe_unused]] const u32 index, [[maybe_unused]] const u32 readData) noexcept { }
    void ReceiveLoadStoreUnit_MemoryAddress([[maybe_unused]] const u32 index, [[maybe_unused]] const u32 memoryAddress) noexcept { }
    void ReceiveLoadStoreUnit_Wait([[maybe_unused]] const u32 index, [[maybe_unused]] const bool wait) noexcept { }
    void ReceiveLoadStoreUnit_Error([[maybe_unused]] const u32 index, [[maybe_unused]] const u32 error) noexcept { }
    void ReceiveLoadStoreUnit_MemoryRequest([[maybe_unused]] const u32 index, [[maybe_unused]] const MemoryBusRequest& memoryRequest) noexcept { }
};

template<typename Receiver = LoadStoreUnitReceiverSample>
class LoadStoreUnit final
{
    DEFAULT_DESTRUCT(LoadStoreUnit);
    DELETE_CM(LoadStoreUnit);
// public:
//     using Receiver = LoadStoreUnitReceiverSample;
private:
    SENSITIVITY_DECL(p_Reset_n, p_Clock);

    SIGNAL_ENTITIES();
public:
    LoadStoreUnit(Receiver* const parent, const u32 index = 0) noexcept
        : m_Parent(parent)
        , m_Index(index)
        , p_Reset_n(0)
        , p_Clock(0)
        , m_Misaligned(0)
        , m_Pending(0)
        , m_Pad0(0)
        , p_ControlBus()
        , p_Address(0)
        , p_WriteData(0)
        , m_MemoryAddressRequest(0)
        , p_MemoryResponse()
        , p_out_MemoryRequest()
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

    void SetPMPFault(const bool pmpFault) noexcept
    {
        p_PMPFault = BOOL_TO_BIT(pmpFault);
    }

    void SetAddress(const u32 address) noexcept
    {
        p_Address = address;
    }

    void SetWriteData(const u32 data) noexcept
    {
        p_WriteData = data;
    }

    void SetControlBus(const ControlBus& controlBus) noexcept
    {
        p_ControlBus = controlBus;

        UpdateError();
    }

    void SetMemoryResponse(const MemoryBusResponse& memoryResponse) noexcept
    {
        p_MemoryResponse = memoryResponse;

        m_Parent->ReceiveLoadStoreUnit_Wait(m_Index, !BIT_TO_BOOL(p_MemoryResponse.Acknowledge));
        UpdateError();
    }
private:
    void SetMemoryAddressRequest(const u32 memoryAddressRequest) noexcept
    {
        m_MemoryAddressRequest = memoryAddressRequest;

        m_Parent->ReceiveLoadStoreUnit_MemoryAddress(m_Index, memoryAddressRequest);

        UpdateMemoryRequest();
    }

    void UpdateMemoryRequest() noexcept
    {
        p_out_MemoryRequest.Address = m_MemoryAddressRequest;
        p_out_MemoryRequest.Strobe = BOOL_TO_BIT(MemoryRequest_Strobe());
        p_out_MemoryRequest.Source = 0; // Data Access
        p_out_MemoryRequest.Burst = 0; // Single Access
        p_out_MemoryRequest.Fence = p_ControlBus.LSU_Fence;

        m_Parent->ReceiveLoadStoreUnit_MemoryRequest(m_Index, p_out_MemoryRequest);
    }

    void UpdateError() const noexcept
    {
        const u32 error = (BOOL_TO_BIT(Error3()) << 3) | (BOOL_TO_BIT(Error2()) << 2) | (BOOL_TO_BIT(Error1()) << 1) | BOOL_TO_BIT(Error0());
        m_Parent->ReceiveLoadStoreUnit_Error(m_Index, error);
    }

    void SetPMPError(const bool pmpError) noexcept
    {
        m_PMPError = BOOL_TO_BIT(pmpError);
        UpdateError();
    }

    void SetMisaligned(const bool misaligned) noexcept
    {
        m_Misaligned = BOOL_TO_BIT(misaligned);
        UpdateError();
    }

    void SetPending(const bool pending) noexcept
    {
        m_Pending = BOOL_TO_BIT(pending);
        UpdateError();
    }

    // Wires

    /**
     * Misaligned Load
     */
    [[nodiscard]] bool Error0() const noexcept
    {
        return BIT_TO_BOOL(m_Pending) && !BIT_TO_BOOL(p_ControlBus.LSU_ReadWrite) && m_Misaligned;
    }

    /**
     * Load Bus Error
     */
    [[nodiscard]] bool Error1() const noexcept
    {
        return BIT_TO_BOOL(m_Pending) && !BIT_TO_BOOL(p_ControlBus.LSU_ReadWrite) && (BIT_TO_BOOL(p_MemoryResponse.Error) || BIT_TO_BOOL(m_PMPError));
    }

    /**
     * Misaligned Store
     */
    [[nodiscard]] bool Error2() const noexcept
    {
        return BIT_TO_BOOL(m_Pending) && BIT_TO_BOOL(p_ControlBus.LSU_ReadWrite) && m_Misaligned;
    }

    /**
     * Store Bus Error
     */
    [[nodiscard]] bool Error3() const noexcept
    {
        return BIT_TO_BOOL(m_Pending) && BIT_TO_BOOL(p_ControlBus.LSU_ReadWrite) && (BIT_TO_BOOL(p_MemoryResponse.Error) || BIT_TO_BOOL(m_PMPError));
    }

    [[nodiscard]] bool MemoryRequest_Strobe() const noexcept
    {
        return BIT_TO_BOOL(p_ControlBus.LSU_Request) && !BIT_TO_BOOL(m_Misaligned) && !BIT_TO_BOOL(p_PMPFault);
    }

    // Muxes

    [[nodiscard]] u32 AtomicOperation() const noexcept
    {
        switch((p_ControlBus.IR_Function12 >> 7) & 0x1F)
        {
            case 0b00010: return 0b1000; // Zalrsc.LR
            case 0b00011: return 0b1001; // Zalrsc.SC
            case 0b00000: return 0b0001; // Zaamo.ADD
            case 0b00100: return 0b0010; // Zaamo.XOR
            case 0b01100: return 0b0011; // Zaamo.AND
            case 0b01000: return 0b0100; // Zaamo.OR
            case 0b10000: return 0b1110; // Zaamo.MIN
            case 0b10100: return 0b1111; // Zaamo.MAX
            case 0b11000: return 0b0110; // Zaamo.MINU
            case 0b11100: return 0b0111; // Zaamo.MAXU
            default:      return 0b0000; // Zaamo.SWAP
        }
    }
private:
    PROCESSES_DECL()
    {
        PROCESS_ENTER(MemoryAddressRegister, p_Reset_n, p_Clock)
        PROCESS_ENTER(MemoryDataOutput, p_Reset_n, p_Clock)
        PROCESS_ENTER(BusLock, p_Reset_n, p_Clock)
        PROCESS_ENTER(MemoryDataInput, p_Reset_n, p_Clock)
        PROCESS_ENTER(AccessArbiter, p_Reset_n, p_Clock)
    }

    PROCESS_DECL(MemoryAddressRegister)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            SetMemoryAddressRequest(0);
            SetMisaligned(false);
        }
        else if(RISING_EDGE(p_Clock))
        {
            if(BIT_TO_BOOL(p_ControlBus.LSU_Enable))
            {
                SetMemoryAddressRequest(p_Address);
                switch(p_ControlBus.IR_Function3 & 0b11)
                {
                    case 0b00:
                        SetMisaligned(false);
                        break;
                    case 0b01:
                        SetMisaligned(BIT_TO_BOOL(p_Address & 0x1));
                        break;
                    default:
                        SetMisaligned(BIT_TO_BOOL(((p_Address >> 1) & 0x1) | (p_Address & 0x1)));
                        break;
                }
            }
        }
    }

    PROCESS_DECL(MemoryDataOutput)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            p_out_MemoryRequest.WriteData = 0;
            p_out_MemoryRequest.ByteEnable = 0;
            p_out_MemoryRequest.ReadWrite = 0;
            p_out_MemoryRequest.Atomic = 0;
            p_out_MemoryRequest.AtomicOperation = 0;
            p_out_MemoryRequest.Privileged = 0;
            p_out_MemoryRequest.Debug = 0;

            UpdateMemoryRequest();
        }
        else if(RISING_EDGE(p_Clock))
        {
            if(BIT_TO_BOOL(p_ControlBus.LSU_Enable))
            {
                p_out_MemoryRequest.ReadWrite = p_ControlBus.LSU_ReadWrite;
                p_out_MemoryRequest.Atomic = p_ControlBus.LSU_Atomic;
                p_out_MemoryRequest.AtomicOperation = AtomicOperation();
                p_out_MemoryRequest.Privileged = p_ControlBus.LSU_Privileged;
                p_out_MemoryRequest.Debug = p_ControlBus.CPU_Debug;

                switch(p_ControlBus.IR_Function3 & 0b11)
                {
                    case 0b00:
                    {
                        p_out_MemoryRequest.WriteData = ((p_WriteData & 0xFF) << 24) | ((p_WriteData & 0xFF) << 16) | ((p_WriteData & 0xFF) << 8) | (p_WriteData & 0xFF);
                        const bool byteEnable0 =  !BIT_TO_BOOL(p_Address & 0x1) && !BIT_TO_BOOL(p_Address & 0x2);
                        const bool byteEnable1 =  !BIT_TO_BOOL(p_Address & 0x1) && BIT_TO_BOOL(p_Address & 0x2);
                        const bool byteEnable2 =  BIT_TO_BOOL(p_Address & 0x1) && !BIT_TO_BOOL(p_Address & 0x2);
                        const bool byteEnable3 =  BIT_TO_BOOL(p_Address & 0x1) && BIT_TO_BOOL(p_Address & 0x2);
                        p_out_MemoryRequest.ByteEnable =
                            (BOOL_TO_BIT(byteEnable0) << 3) |
                            (BOOL_TO_BIT(byteEnable1) << 2) |
                            (BOOL_TO_BIT(byteEnable2) << 1) |
                            BOOL_TO_BIT(byteEnable3);
                        break;
                    }
                    case 0b01:
                        p_out_MemoryRequest.WriteData = ((p_WriteData & 0xFFFF) << 16) | (p_WriteData & 0xFFFF);
                        p_out_MemoryRequest.ByteEnable =
                            ((p_Address & 0x1) << 3) |
                            ((p_Address & 0x1) << 2) |
                            (((p_Address & 0x1) ^ 0x1) << 1) |
                            ((p_Address & 0x1) ^ 0x1);
                        break;
                    default:
                        p_out_MemoryRequest.WriteData = p_WriteData;
                        p_out_MemoryRequest.ByteEnable = 0b1111;
                        break;
                }

                UpdateMemoryRequest();
            }
        }
    }

    PROCESS_DECL(BusLock)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            p_out_MemoryRequest.Lock = 0;

            UpdateMemoryRequest();
        }
        else if(RISING_EDGE(p_Clock))
        {
            if(BIT_TO_BOOL(p_ControlBus.LSU_Enable) && BIT_TO_BOOL(p_ControlBus.LSU_Atomic) && !BIT_TO_BOOL((p_ControlBus.IR_Function12 >> 8) & 1))
            {
                p_out_MemoryRequest.Lock = 1; // Set if Zaamo instruction.

                UpdateMemoryRequest();
            }
            else if(BIT_TO_BOOL(p_ControlBus.IF_Acknowledge) || BIT_TO_BOOL(p_ControlBus.CPU_Trap))
            {
                p_out_MemoryRequest.Lock = 0; // Clear at the end of bus access.

                UpdateMemoryRequest();
            }
        }
    }

    PROCESS_DECL(MemoryDataInput)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            m_Parent->ReceiveLoadStoreUnit_ReadData(m_Index, 0);
        }
        else if(RISING_EDGE(p_Clock))
        {
            if(BIT_TO_BOOL(m_Pending))
            {
                u32 data;

                switch(p_ControlBus.IR_Function3 & 0b11)
                {
                    case 0b00: // Byte
                    {
                        switch(p_Address & 0b11)
                        {
                            case 0b00:
                                data = ReplicateToVector<24, u32>(!BIT_TO_BOOL((p_ControlBus.IR_Function3 >> 2) & 0x1) && BIT_TO_BOOL((p_MemoryResponse.Data >> 7) & 0x1)) << 24;
                                data |= p_MemoryResponse.Data & 0xFF;
                                break;
                            case 0b01:
                                data = ReplicateToVector<24, u32>(!BIT_TO_BOOL((p_ControlBus.IR_Function3 >> 2) & 0x1) && BIT_TO_BOOL((p_MemoryResponse.Data >> 15) & 0x1)) << 24;
                                data |= (p_MemoryResponse.Data >> 8) & 0xFF;
                                break;
                            case 0b10:
                                data = ReplicateToVector<24, u32>(!BIT_TO_BOOL((p_ControlBus.IR_Function3 >> 2) & 0x1) && BIT_TO_BOOL((p_MemoryResponse.Data >> 23) & 0x1)) << 24;
                                data |= (p_MemoryResponse.Data >> 16) & 0xFF;
                                break;
                            default:
                                data = ReplicateToVector<24, u32>(!BIT_TO_BOOL((p_ControlBus.IR_Function3 >> 2) & 0x1) && BIT_TO_BOOL((p_MemoryResponse.Data >> 31) & 0x1)) << 24;
                                data |= (p_MemoryResponse.Data >> 24) & 0xFF;
                                break;
                        }
                        break;
                    }
                    case 0b01: // Half Word
                        if((p_Address & 0x2) == 0) // Low
                        {
                            data = ReplicateToVector<16, u32>(!BIT_TO_BOOL((p_ControlBus.IR_Function3 >> 2) & 0x1) && BIT_TO_BOOL((p_MemoryResponse.Data >> 15) & 0x1)) << 16;
                            data |= p_MemoryResponse.Data & 0xFFFF;
                        }
                        else // High
                        {
                            data = ReplicateToVector<16, u32>(!BIT_TO_BOOL((p_ControlBus.IR_Function3 >> 2) & 0x1) && BIT_TO_BOOL((p_MemoryResponse.Data >> 31) & 0x1)) << 16;
                            data |= (p_MemoryResponse.Data >> 16) & 0xFFFF;
                        }
                        break;
                    default:
                        data = p_MemoryResponse.Data;
                        break;
                }
                m_Parent->ReceiveLoadStoreUnit_ReadData(m_Index, data);
            }
            else
            {
                m_Parent->ReceiveLoadStoreUnit_ReadData(m_Index, 0);
            }
        }
    }

    PROCESS_DECL(AccessArbiter)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            SetPMPError(false);
            SetPending(false);
        }
        else if(RISING_EDGE(p_Clock))
        {
            SetPMPError(BIT_TO_BOOL(p_PMPFault));
            if(!BIT_TO_BOOL(m_Pending))
            {
                SetPending(BIT_TO_BOOL(p_ControlBus.LSU_Request));
            }
            else if(BIT_TO_BOOL(p_ControlBus.IF_Acknowledge) || BIT_TO_BOOL(p_ControlBus.CPU_Trap)) // Bus response, or start of trap handling.
            {
                SetPending(false);
            }
        }
    }
private:
    Receiver* m_Parent;
    u32 m_Index;

    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    u32 p_PMPFault : 1;

    u32 m_Misaligned : 1;
    u32 m_Pending : 1;
    u32 m_PMPError : 1;
    [[maybe_unused]] u32 m_Pad0 : 26;

    ControlBus p_ControlBus;
    u32 p_Address;
    u32 p_WriteData;
    u32 m_MemoryAddressRequest;

    MemoryBusResponse p_MemoryResponse;
    MemoryBusRequest p_out_MemoryRequest;
};

}
