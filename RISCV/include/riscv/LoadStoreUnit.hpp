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
    void ReceiveLoadStoreUnit_ReadData(const u32 index, const u32 readData) noexcept { }
    void ReceiveLoadStoreUnit_MemoryAddress(const u32 index, const u32 memoryAddress) noexcept { }
    void ReceiveLoadStoreUnit_Wait(const u32 index, const bool wait) noexcept { }
    void ReceiveLoadStoreUnit_Error(const u32 index, const u32 error) noexcept { }
    void ReceiveLoadStoreUnit_MemoryRequest(const u32 index, const MemoryBusRequest& memoryRequest) noexcept { }
};

// template<typename Receiver = LoadStoreUnitReceiverSample>
class LoadStoreUnit final
{
    DEFAULT_DESTRUCT(LoadStoreUnit);
    DELETE_CM(LoadStoreUnit);
public:
    using Receiver = LoadStoreUnitReceiverSample;
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
        , m_AtomicOperation(0)
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

    void SetControlBus(const ControlBus& controlBus) noexcept
    {
        p_ControlBus = controlBus;
    }

    void SetMemoryResponse(const MemoryBusResponse& memoryResponse) noexcept
    {
        p_MemoryResponse = memoryResponse;

        m_Parent->ReceiveLoadStoreUnit_Wait(m_Index, !BIT_TO_BOOL(p_MemoryResponse.Acknowledge));
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
        p_out_MemoryRequest.Fence = p_ControlBus.LSU_Fence;

        m_Parent->ReceiveLoadStoreUnit_MemoryRequest(m_Index, p_out_MemoryRequest);
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
        return BIT_TO_BOOL(m_Pending) && !BIT_TO_BOOL(p_ControlBus.LSU_ReadWrite) && p_MemoryResponse.Error;
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
        return BIT_TO_BOOL(m_Pending) && BIT_TO_BOOL(p_ControlBus.LSU_ReadWrite) && p_MemoryResponse.Error;
    }

    [[nodiscard]] bool MemoryRequest_Strobe() const noexcept
    {
        return BIT_TO_BOOL(p_ControlBus.LSU_Request) && !BIT_TO_BOOL(m_Misaligned);
    }
private:
    PROCESSES_DECL()
    {
        PROCESS_ENTER(MemoryAddressRegister, p_Reset_n, p_Clock)
        PROCESS_ENTER(MemoryDataOutput, p_Reset_n, p_Clock)
    }

    PROCESS_DECL(MemoryAddressRegister)
    {
        if(!BIT_TO_BOOL(p_Reset_n))
        {
            SetMemoryAddressRequest(0);
            m_Misaligned = 0;
        }
        else if(RISING_EDGE(p_Clock))
        {
            if(BIT_TO_BOOL(p_ControlBus.LSU_Enable))
            {
                SetMemoryAddressRequest(p_Address);
                switch(p_ControlBus.IR_Function3 & 0b11)
                {
                    case 0b00:
                        m_Misaligned = 0;
                        break;
                    case 0b01:
                        m_Misaligned = p_Address & 0x1;
                        break;
                    default:
                        m_Misaligned = ((p_Address >> 1) & 0x1) | (p_Address & 0x1);
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

            UpdateMemoryRequest();
        }
        else if(RISING_EDGE(p_Clock))
        {
            p_out_MemoryRequest.ReadWrite = p_ControlBus.LSU_ReadWrite;
            p_out_MemoryRequest.Atomic = p_ControlBus.LSU_Atomic;
            p_out_MemoryRequest.AtomicOperation = m_AtomicOperation;

            switch(p_ControlBus.IR_Function3 & 0b11)
            {
                case 0b00:
                    break;
                case 0b01:
                    break;
                default:
                    p_out_MemoryRequest.WriteData = p_WriteData;
                    p_out_MemoryRequest.ByteEnable = 0b1111;
                    break;
            }

            UpdateMemoryRequest();
        }
    }
private:
    Receiver* m_Parent;
    u32 m_Index;

    u32 p_Reset_n : 1;
    u32 p_Clock : 1;

    u32 m_Misaligned : 1;
    u32 m_Pending : 1;
    u32 m_AtomicOperation : 4;
    [[maybe_unused]] u32 m_Pad0 : 24;

    ControlBus p_ControlBus;
    u32 p_Address;
    u32 p_WriteData;
    u32 m_MemoryAddressRequest;

    MemoryBusResponse p_MemoryResponse;
    MemoryBusRequest p_out_MemoryRequest;
};

}
