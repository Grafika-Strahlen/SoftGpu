#pragma once

#include <ConPrinter.hpp>
#include <Objects.hpp>
#include <riscv/CpuCore.hpp>
#include "StreamingMultiprocessor.hpp"
#include "PCIControlRegisters.hpp"
#include "Cache.hpp"
#include "DebugManager.hpp"
#include "PCIController.hpp"
#include "RomController.hpp"
#include "DisplayManager.hpp"
#include "DMAController.hpp"

class Processor final
{
    DEFAULT_DESTRUCT(Processor);
    DELETE_CM(Processor);
private:
    SENSITIVITY_DECL(p_Reset_n, p_Clock, m_TriggerReset_n);
    STD_LOGIC_DECL(m_TriggerReset_n);

    SIGNAL_ENTITIES();
public:
    Processor() noexcept
        : p_Reset_n(0)
        , p_Clock(0)
        , m_Pad0(0)
        , m_TriggerReset_n(StdLogic::U)
        , m_Pad1(0)
        , m_PciController(this)
        , m_RomController(this)
        , m_PciRegisters(this)
        , m_CacheController(this)
        , m_DmaController(this)
        , m_SMs { { this, 0 }, { this, 1 }, { this, 2 }, { this, 3 } }
        , m_DisplayManager(this)
        , m_ClockCycle(0)
        , m_RamBaseAddress(0)
        , m_RamSize(0)
    { }

    void SetResetN(const bool reset_n) noexcept
    {
        p_Reset_n = BOOL_TO_BIT(reset_n);

        TRIGGER_SENSITIVITY(p_Reset_n);
    }

    void SetClock(const bool clock) noexcept
    {
        p_Clock = BOOL_TO_BIT(clock);

        m_PciRegisters.SetClock(clock);
        // m_DmaController.SetClock(clock);
        m_DisplayManager.SetClock(clock);

        TRIGGER_SENSITIVITY(p_Clock);
    }

    void Reset()
    {
        m_PciRegisters.SetResetN(false);
        m_CacheController.Reset();
        m_DmaController.SetResetN(false);
        m_SMs[0].Reset();
        m_SMs[1].Reset();
        m_SMs[2].Reset();
        m_SMs[3].Reset();
        m_DisplayManager.SetResetN(false);
        m_ClockCycle = 0;

        m_PciRegisters.SetResetN(true);
        m_DmaController.SetResetN(true);
        m_DisplayManager.SetResetN(true);
    }

    void Clock() noexcept
    {
        ++m_ClockCycle;
        if(GlobalDebug.IsAttached())
        {
            GlobalDebug.WriteInfo(DebugCodeReportTiming, &m_ClockCycle, sizeof(m_ClockCycle));

            if(!GlobalDebug.Stepping() && !GlobalDebug.DisableStepping())
            {
                GlobalDebug.WriteStepping(DebugCodeCheckForPause);

                const u32 dataCode = GlobalDebug.ReadStepping<u32>();
                const u32 dataSize = GlobalDebug.ReadStepping<u32>();

                (void) dataSize;

                if(dataCode == DebugCodePause)
                {
                    GlobalDebug.Stepping() = true;
                }
            }

            if(GlobalDebug.Stepping())
            {
                ConPrinter::Print("Waiting for Step.\n");
                GlobalDebug.WriteStepping(DebugCodeReportStepReady);

                while(true)
                {
                    const u32 dataCode = GlobalDebug.ReadStepping<u32>();
                    const u32 dataSize = GlobalDebug.ReadStepping<u32>();

                    (void) dataSize;

                    if(dataCode == DebugCodeStep)
                    {
                        ConPrinter::Print("Step Received.\n");
                        break;
                    }
                    else if(dataCode == DebugCodeResume)
                    {
                        ConPrinter::Print("Resume Received.\n");
                        GlobalDebug.Stepping() = false;
                        break;
                    }
                }
            }
        }

        m_PciController.Clock(true);
        m_PciRegisters.SetClock(true);
        // m_DmaController.SetClock(true);
        m_DisplayManager.SetClock(true);

        m_SMs[0].Clock();
        m_SMs[1].Clock();
        m_SMs[2].Clock();
        m_SMs[3].Clock();

        m_PciController.Clock(false);
        m_PciRegisters.SetClock(false);
        // m_DmaController.SetClock(false);
        m_DisplayManager.SetClock(false);
    }

    void TestLoadProgram(const u32 sm, const u32 dispatchPort, const u8 replicationMask, const u64 program)
    {
        m_SMs[sm].TestLoadProgram(dispatchPort, replicationMask, program);
    }

    void TestLoadProgram(const u32 sm, const u32 dispatchPort, const u8 replicationMask, void* const program)
    {
        TestLoadProgram(sm, dispatchPort, replicationMask, reinterpret_cast<u64>(program));
    }

    void TestLoadRegister(const u32 sm, const u32 dispatchPort, const u32 replicationIndex, const u8 registerIndex, const u32 registerValue)
    {
        m_SMs[sm].TestLoadRegister(dispatchPort, replicationIndex, registerIndex, registerValue);
    }

    void TestSetRamBaseAddress(const u64 ramBaseAddress, const u64 size) noexcept
    {
        m_RamBaseAddress = ramBaseAddress;
        m_RamSize = size;
    }

    [[nodiscard]] u32 MemReadPhy(const u64 address, const bool external = false) noexcept
    {
        (void) external;

        u32 ret;
        // The memory granularity is 32 bits, thus we'll adjust to an 1 byte granularity for x86.
        const uintptr_t addressX86 = address << 2;
        (void) ::std::memcpy(&ret, reinterpret_cast<void*>(addressX86), sizeof(u32));
        return ret;
    }
    
    void MemWritePhy(const u64 address, const u32 value, const bool external = false) noexcept
    {
        (void) external;

        // The memory granularity is 32 bits, thus we'll adjust to an 1 byte granularity for x86.
        const uintptr_t addressX86 = address << 2;
        (void) ::std::memcpy(reinterpret_cast<void*>(addressX86), &value, sizeof(u32));
    }

    void PciBusRead(const u64 cpuPhysicalAddress, const u16 size, u32 transferBlock[1024]) noexcept
    {
        m_PciController.PciBusMasterRead(cpuPhysicalAddress, size, transferBlock);
    }

    void PciBusWrite(const u64 cpuPhysicalAddress, const u16 size, const u32 transferBlock[1024]) noexcept
    {
        m_PciController.PciBusMasterWrite(cpuPhysicalAddress, size, transferBlock);
    }

    [[nodiscard]] u32 PciConfigRead(const u16 address, const u8 size) noexcept
    {
        return m_PciController.ConfigRead(address, size);
    }

    void PciConfigWrite(const u16 address, const u8 size, const u32 value) noexcept
    {
        m_PciController.ConfigWrite(address, size, value);
    }

    void PciMemReadSet(const u64 address, const u16 size, u32* const data, u16* const readResponse) noexcept
    {
        m_PciController.PciMemReadSet(address, size, data, readResponse);
    }

    void PciMemWriteSet(const u64 address, const u16 size, const u32* const data) noexcept
    {
        m_PciController.PciMemWriteSet(address, size, data);
    }

    // u16 PciMemRead(const u64 address, const u16 size, u32* const data) noexcept
    // {
    //     if(!(m_PciController.CommandRegister() & PciController::COMMAND_REGISTER_MEMORY_SPACE_BIT))
    //     {
    //         ConPrinter::PrintLn("Attempted to Read over PCI while the Memory Space bit was not set.");
    //         return 0;
    //     }
    //
    //     const u8 bar = m_PciController.GetBARFromAddress(address);
    //
    //     if constexpr(false)
    //     {
    //         if(bar == PciController::EXPANSION_ROM_BAR_ID)
    //         {
    //             ConPrinter::PrintLn("Reading from Expansion ROM {} bytes at 0x{XP0}.", size, address);
    //         }
    //         else
    //         {
    //             ConPrinter::PrintLn("Reading from BAR{} {} bytes at 0x{XP0}.", bar, size, address);
    //         }
    //     }
    //
    //     if(bar == 0xFF)
    //     {
    //         return 0;
    //     }
    //
    //     const u64 addressOffset = m_PciController.GetBAROffset(address, bar);
    //
    //     if(bar == 0)
    //     {
    //         data[0] = m_PciRegisters.Read(static_cast<u32>(addressOffset));
    //         return 1;
    //     }
    //
    //     if(bar == 1)
    //     {
    //         // Shift right 2 to match the MMU granularity of 4 bytes.
    //         const u64 realAddress4 = (addressOffset + m_RamBaseAddress) >> 2;
    //
    //         const u16 sizeWords = size / 4;
    //
    //         for(u16 i = 0; i < sizeWords; ++i)
    //         {
    //             // ~Use the real address as that is mapped into the system virtual page.~
    //             data[i] = MemReadPhy(realAddress4 + i);
    //         }
    //
    //         return size;
    //     }
    //
    //     if(bar == PciController::EXPANSION_ROM_BAR_ID)
    //     {
    //         return m_RomController.PciReadExpansionRom(addressOffset, size, data);
    //     }
    //
    //     return 0;
    // }
    //
    // void PciMemWrite(const u64 address, const u16 size, const u32* const data) noexcept
    // {
    //     if(!(m_PciController.CommandRegister() & PciController::COMMAND_REGISTER_MEMORY_SPACE_BIT))
    //     {
    //         ConPrinter::PrintLn("Attempted to Write over PCI while the Memory Space bit was not set.");
    //         return;
    //     }
    //
    //     const u8 bar = m_PciController.GetBARFromAddress(address);
    //
    //     ConPrinter::PrintLn("Writing to BAR{} {} bytes at 0x{XP0}.", bar, size, address);
    //
    //     if(bar == 0xFF)
    //     {
    //         return;
    //     }
    //
    //     const u64 addressOffset = m_PciController.GetBAROffset(address, bar);
    //
    //     if(bar == 0)
    //     {
    //         m_PciRegisters.Write(static_cast<u32>(addressOffset), data[0]);
    //     }
    //     else if(bar == 1)
    //     {
    //         // Shift right 2 to match the MMU granularity of 4 bytes.
    //         const u64 realAddress4 = (addressOffset + m_RamBaseAddress) >> 2;
    //
    //         const u16 sizeWords = size / 4;
    //
    //         for(u16 i = 0; i < sizeWords; ++i)
    //         {
    //             // ~Use the real address as that is mapped into the system virtual page.~
    //             MemWritePhy(realAddress4 + i, data[i]);
    //         }
    //     }
    // }

    u16 PciReadExpansionRom(const u64 address, const u16 size, u32* const data) noexcept
    {
        return m_RomController.PciReadExpansionRom(address, size, data);
    }

    u16 CommandRegister() noexcept { return m_PciController.CommandRegister(); }
    bool ExpansionRomEnable() noexcept { return m_PciController.ExpansionRomEnable(); }

    void SetInterrupt(const u32 messageType) noexcept
    {
        m_PciController.SetInterrupt(messageType);
        m_PciRegisters.SetInterrupt(messageType);
    }

    [[nodiscard]] u32 Read(const u32 coreIndex, const u64 address, const bool cacheDisable = false, const bool external = false) noexcept
    {
        if(cacheDisable)
        {
            return MemReadPhy(address, external);
        }

        return m_CacheController.Read(coreIndex, address, external);
    }

    void Write(const u32 coreIndex, const u64 address, const u32 value, const bool writeThrough = false, const bool cacheDisable = false, const bool external = false) noexcept
    {
        if(cacheDisable)
        {
            MemWritePhy(address, value, external);
            return;
        }

        m_CacheController.Write(coreIndex, address, value, external, writeThrough);
    }

    void Prefetch(const u32 coreIndex, const u64 address, const bool external = false) noexcept
    {
        m_CacheController.Prefetch(coreIndex, address, external);
    }

    void FlushCache(const u32 coreIndex) noexcept
    {
        m_CacheController.Flush(coreIndex);
    }

    void LoadPageDirectoryPointer(const u64 coreIndex, const u64 pageDirectoryPhysicalAddress) noexcept
    {
        m_SMs[coreIndex].LoadPageDirectoryPointer(pageDirectoryPhysicalAddress);
    }

    void LoadPageDirectoryPointer(const u64 coreIndex, const void* const pageDirectoryPhysicalAddress) noexcept
    {
        LoadPageDirectoryPointer(coreIndex, reinterpret_cast<u64>(pageDirectoryPhysicalAddress) >> 16);
    }

    void FlushMmuCache(const u64 coreIndex) noexcept
    {
        m_SMs[coreIndex].FlushCache();
    }

    void ReceivePciControlRegisters_TriggerResetN(const StdLogic reset_n) noexcept
    {
        STD_LOGIC_SET(m_TriggerReset_n, reset_n);

        TRIGGER_SENSITIVITY(m_TriggerReset_n);
    }

    void ReceivePciControlRegisters_DisplayManagerRequestActive(const StdLogic active) noexcept
    {
        m_DisplayManager.SetRequestActive(active);
    }

    void ReceivePciControlRegisters_DisplayManagerRequestPacketType(const DisplayRequestPacketType packetType) noexcept
    {
        m_DisplayManager.SetRequestPacketType(packetType);
    }

    void ReceivePciControlRegisters_DisplayManagerRequestReadWrite(const EReadWrite readWrite) noexcept
    {
        m_DisplayManager.SetRequestReadWrite(readWrite);
    }

    void ReceivePciControlRegisters_DisplayManagerRequestDisplayIndex(const u32 displayIndex) noexcept
    {
        m_DisplayManager.SetRequestDisplayIndex(displayIndex);
    }

    void ReceivePciControlRegisters_DisplayManagerRequestRegister(const u32 registerIndex) noexcept
    {
        m_DisplayManager.SetRequestRegister(registerIndex);
    }

    void ReceivePciControlRegisters_DisplayManagerRequestData(const u32 data) noexcept
    {
        m_DisplayManager.SetRequestData(data);
    }

    void ReceiveDisplayManager_Acknowledge(const bool acknowledge) noexcept
    {
        m_PciRegisters.SetDisplayManagerAcknowledge(acknowledge);
    }

    void ReceiveDisplayManager_Data(const u32 data) noexcept
    {

    }

    void ReceiveDisplayManager_VSyncEvent(const bool event) noexcept
    {

    }

    [[nodiscard]] u64 RamBaseAddress() const noexcept { return m_RamBaseAddress; }
    [[nodiscard]] u64 RamSize() const noexcept { return m_RamSize; }
    [[nodiscard]] u16 PCITransferLimit() const noexcept { return 1024; }

    [[nodiscard]] PciControlRegistersBus& PciControlRegistersBus() noexcept { return m_PciRegisters.Bus(); }
    // [[nodiscard]] DMAChannelBus& DmaChannelBus(const u16 index) noexcept { return m_DmaController.Bus(index); }
    // void UnlockDma(const u16 index) noexcept { m_PciRegisters.UnlockDma(index); }

    [[nodiscard]] DMAController::BusState DmaGetBusState() const noexcept { return m_DmaController.GetBusState(); }
    void DmaSetCPUPhysicalAddress(const u64 cpuPhysicalAddress) noexcept { m_DmaController.SetCPUPhysicalAddress(cpuPhysicalAddress); }
    [[nodiscard]] u64 DmaGetCPUPhysicalAddress() const noexcept { return m_DmaController.GetCPUPhysicalAddress(); }
    void DmaSetGPUVirtualAddress(const u64 gpuVirtualAddress) noexcept { m_DmaController.SetGPUVirtualAddress(gpuVirtualAddress); }
    void DmaSetWordCount(const u64 wordCount) noexcept { m_DmaController.SetWordCount(wordCount); }
    void DmaSetReadWrite(const bool readWrite) noexcept { m_DmaController.SetReadWrite(readWrite); }
    void DmaSetAtomic(const bool atomic) noexcept { m_DmaController.SetAtomic(atomic); }
    void DmaSetActive(const bool active) noexcept { m_DmaController.SetActive(active); }
    void DmaSetReady(const u32 index, const bool ready) noexcept { m_DmaController.SetReady(index, ready); }
    [[nodiscard]] u32 DmaGetBusSelect() const noexcept { return m_DmaController.GetBusSelect(); }

    // Intended only for VBDevice.
    [[nodiscard]] PciController& GetPciController() noexcept { return m_PciController; }
    [[nodiscard]] PciControlRegisters& GetPciControlRegisters() noexcept { return m_PciRegisters; }
    [[nodiscard]] DisplayManager<Processor>& GetDisplayManager() noexcept { return m_DisplayManager; }
private:
    // Muxes

    //   This mux checks if we're in reset, either from an external
    // hardware pin, or from an internal trigger.
    //   We use AND logic since these are inverse bits, if either of them
    // are low, we want to signal the reset, which occurs when driven low.
    [[nodiscard]] bool MergedResetN() const noexcept
    {
        return BIT_TO_BOOL(p_Reset_n) && LOGIC_TO_BOOL(m_TriggerReset_n);
    }
private:
    PROCESSES_DECL()
    {
        STD_LOGIC_PROCESS_RESET_HANDLER(p_Clock);
        PROCESS_ENTER(HandleReset, p_Reset_n, m_TriggerReset_n)
        PROCESS_ENTER(HandleClock, p_Clock)
    }

    PROCESS_DECL(HandleReset)
    {
        // On reset, reset the elements of this entity.
        if(!MergedResetN())
        {
            m_RamBaseAddress = 0;
            m_RamSize = 0;
        }

        //   Since this is only triggered on sensitivity changes to p_Reset_n
        // and m_TriggerReset_n, this won't constantly trigger the reset
        // processes.
        //   We need to do this out here so that p_Reset_n of each entity
        // receives the current state, and doesn't simply drive them low during
        // a reset.
        const bool reset_n = MergedResetN();

        m_DmaController.SetResetN(reset_n);
        m_DisplayManager.SetResetN(reset_n);

        // This should be last since it is likely the cause of the reset.
        //   This shouldn't be necessary since we made the clearing of the
        // reset signal happen in the clock falling edge.
        m_PciRegisters.SetResetN(reset_n);
    }

    PROCESS_DECL(HandleClock)
    {
        if(!MergedResetN())
        {

        }
        else if(RISING_EDGE(p_Clock))
        {
            ++m_ClockCycle;

            if(GlobalDebug.IsAttached())
            {
                GlobalDebug.WriteInfo(DebugCodeReportTiming, &m_ClockCycle, sizeof(m_ClockCycle));

                if(!GlobalDebug.Stepping() && !GlobalDebug.DisableStepping())
                {
                    GlobalDebug.WriteStepping(DebugCodeCheckForPause);

                    const u32 dataCode = GlobalDebug.ReadStepping<u32>();
                    const u32 dataSize = GlobalDebug.ReadStepping<u32>();

                    (void) dataSize;

                    if(dataCode == DebugCodePause)
                    {
                        GlobalDebug.Stepping() = true;
                    }
                }

                if(GlobalDebug.Stepping())
                {
                    ConPrinter::Print("Waiting for Step.\n");
                    GlobalDebug.WriteStepping(DebugCodeReportStepReady);

                    while(true)
                    {
                        const u32 dataCode = GlobalDebug.ReadStepping<u32>();
                        const u32 dataSize = GlobalDebug.ReadStepping<u32>();

                        (void) dataSize;

                        if(dataCode == DebugCodeStep)
                        {
                            ConPrinter::Print("Step Received.\n");
                            break;
                        }
                        else if(dataCode == DebugCodeResume)
                        {
                            ConPrinter::Print("Resume Received.\n");
                            GlobalDebug.Stepping() = false;
                            break;
                        }
                    }
                }
            }
        }
    }
private:
    u32 p_Reset_n : 1;
    u32 p_Clock : 1;
    u32 m_Pad0 : 30;

    StdLogic m_TriggerReset_n : 3;
    u32 m_Pad1 : 29;

    PciController m_PciController;
    RomController m_RomController;
    PciControlRegisters m_PciRegisters;
    CacheController m_CacheController;
    DMAController m_DmaController;
    StreamingMultiprocessor m_SMs[4];
    DisplayManager<Processor> m_DisplayManager;
    u32 m_ClockCycle;
    u64 m_RamBaseAddress;
    u64 m_RamSize;
};
