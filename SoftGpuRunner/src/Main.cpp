/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#include <bit>

#include <riscv/DualClockFIFO/Memory.TestBench.hpp>
#include <riscv/DualClockFIFO/Synchronizer.TestBench.hpp>
#include <riscv/DualClockFIFO/ReadPointer.TestBench.hpp>
#include <riscv/DualClockFIFO/WritePointer.TestBench.hpp>
#include <riscv/DualClockFIFO/DualClockFIFO.TestBench.hpp>
#include <riscv/CoProcessor/Shifter.TestBench.hpp>
#include <riscv/ClockGate.TestBench.hpp>
#include <riscv/ArithmeticLogicUnit.TestBench.hpp>
#include "Processor.hpp"
#include <ConPrinter.hpp>
#include "DebugManager.hpp"
#include "InputAssembler.hpp"
#include "PCIControlRegisters.hpp"
#include "MMU.hpp"
#include <allocator/PageAllocator.hpp>
#include <vd/Window.hpp>
#include <vd/VulkanManager.hpp>
#include <vd/VulkanCommandPools.hpp>
#include <vd/FramebufferRenderer.hpp>
#include <vd/SdlManager.hpp>

#include <numeric>
#include <Safeties.hpp>
#include <TauUnit.hpp>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif defined(__linux__)
#include <pthread.h>
#endif

static Processor processor;
DebugManager GlobalDebug;

static u32 BAR0 = 0;
static u64 BAR1 = 0;

static void InitEnvironment() noexcept;
static int InitCommandRegister() noexcept;
static int InitBAR() noexcept;
static void ReleaseBARs() noexcept;

namespace tau::test::register_allocator {
extern void RunTests() noexcept;
}

[[maybe_unused]] static void FillFramebufferBlackMagenta(const Ref<::tau::vd::Window>& window, u8* const framebuffer) noexcept
{
    for(uSys y = 0; y < window->FramebufferHeight(); ++y)
    {
        for(uSys x = 0; x < window->FramebufferWidth(); ++x)
        {
            const uSys index = (y * window->FramebufferWidth() + x) * 4;

            if(x < window->FramebufferWidth() / 2)
            {
                if(y < window->FramebufferHeight() / 2)
                {
                    framebuffer[index + 0] = 0xFF;
                    framebuffer[index + 1] = 0x00;
                    framebuffer[index + 2] = 0xFF;
                    framebuffer[index + 3] = 0xFF;
                }
                else
                {
                    framebuffer[index + 0] = 0x00;
                    framebuffer[index + 1] = 0x00;
                    framebuffer[index + 2] = 0x00;
                    framebuffer[index + 3] = 0xFF;
                }
            }
            else
            {
                if(y < window->FramebufferHeight() / 2)
                {
                    framebuffer[index + 0] = 0x00;
                    framebuffer[index + 1] = 0x00;
                    framebuffer[index + 2] = 0x00;
                    framebuffer[index + 3] = 0xFF;
                }
                else
                {
                    framebuffer[index + 0] = 0xFF;
                    framebuffer[index + 1] = 0x00;
                    framebuffer[index + 2] = 0xFF;
                    framebuffer[index + 3] = 0xFF;

                }
            }
        }
    }
}

static void FillFramebufferGradient(const Ref<::tau::vd::Window>& window, u8* const framebuffer) noexcept
{
    for(uSys y = 0; y < window->FramebufferHeight(); ++y)
    {
        for(uSys x = 0; x < window->FramebufferWidth(); ++x)
        {
            const uSys index = (y * window->FramebufferWidth() + x) * 4;

            framebuffer[index + 0] = static_cast<u8>((x * 256) / window->FramebufferWidth());
            framebuffer[index + 1] = static_cast<u8>((y * 256) / window->FramebufferHeight());
            framebuffer[index + 2] = 0xFF - static_cast<u8>((x * y * 256) / (static_cast<uSys>(window->FramebufferWidth()) * window->FramebufferHeight()));
            framebuffer[index + 3] = 0xFF;
        }
    }
}

static u8* BuildFramebuffer(const Ref<::tau::vd::Window>& window) noexcept
{
    u8* const framebuffer = new(::std::nothrow) u8[static_cast<uSys>(window->FramebufferWidth()) * static_cast<uSys>(window->FramebufferHeight()) * 4];
    FillFramebufferGradient(window, framebuffer);
    return framebuffer;
}

static ::std::atomic_bool s_ShouldExit = false;

int main(int argCount, char* args[])
{
    UNUSED2(argCount, args);
    InitEnvironment();

    if constexpr(false)
    {
        // riscv::coprocessor::test::ShifterSerialCheckResetResult(true);
        // riscv::coprocessor::test::ShifterBarrelCheckResetResult(true);
        // riscv::coprocessor::test::ShifterSerialCheck1S0Right(true);
        // riscv::coprocessor::test::ShifterBarrelCheck1S0Right(true);
        // riscv::coprocessor::test::ShifterSerialCheck2S1Right(true);
        // riscv::coprocessor::test::ShifterBarrelCheck2S1Right(true);
        // riscv::coprocessor::test::ShifterSerialCheck4S2Right(true);
        // riscv::coprocessor::test::ShifterBarrelCheck4S2Right(true);
        // riscv::coprocessor::test::ShifterSerialCheck31S3Right(true);
        // riscv::coprocessor::test::ShifterBarrelCheck31S3Right(true);
        // riscv::coprocessor::test::ShifterSerialCheckMaxS31Right(false);
        // riscv::coprocessor::test::ShifterBarrelCheckMaxS31Right(true);
        // riscv::coprocessor::test::ShifterSerialCheckMaxS32Right(false);
        // riscv::coprocessor::test::ShifterBarrelCheckMaxS32Right(true);

        // riscv::coprocessor::test::ShifterBarrelCheckFuzzRight(false);
        // riscv::coprocessor::test::ShifterSerialCheckFuzzRight(false);

        riscv::coprocessor::test::ShifterSerialCheck31S3Left(true);
        riscv::coprocessor::test::ShifterBarrelCheck31S3Left(true);
        riscv::coprocessor::test::ShifterSerialCheckMaxS31Left(false);
        riscv::coprocessor::test::ShifterBarrelCheckMaxS31Left(true);
        riscv::coprocessor::test::ShifterSerialCheckMaxS32Left(false);
        riscv::coprocessor::test::ShifterBarrelCheckMaxS32Left(true);

        // riscv::test::ClockGateTestBench();
        // riscv::fifo::test::SynchronizerResetTest();
        // riscv::fifo::test::SynchronizerSingleTest();
        // riscv::fifo::test::SynchronizerIncrementSingleBitTest();
        // riscv::fifo::test::SynchronizerIncrementDualBitTest();
        // riscv::fifo::test::SynchronizerIncrement6BitTest();
        // riscv::fifo::test::MemorySimpleSet();
        // riscv::fifo::test::MemoryFullSet();
        // riscv::fifo::test::MemoryWrapSet();
        // riscv::fifo::test::MemoryLargeSet();
        // riscv::fifo::test::ReadPointerResetTest();
        // riscv::fifo::test::ReadPointer1BitEmptyTest();
        // riscv::fifo::test::ReadPointer1BitTest(false);
        // riscv::fifo::test::ReadPointer2BitEmptyTest();
        // riscv::fifo::test::ReadPointer2BitTest(false);
        // riscv::fifo::test::WritePointerResetTest();
        // riscv::fifo::test::WritePointer1BitFullTest();
        // riscv::fifo::test::WritePointer1BitTest();
        // riscv::fifo::test::WritePointer2BitFullTest();
        // riscv::fifo::test::WritePointer2BitTest();
        // riscv::fifo::test::FifoTestFastRead1Bit();
        // riscv::fifo::test::FifoTestFastWrite1Bit();
        // riscv::fifo::test::FifoTestFastRead2Bit();
        // riscv::fifo::test::FifoTestFastWrite2Bit();
        // riscv::fifo::test::FifoTestFastRead3Bit();
        // riscv::fifo::test::FifoTestFastWrite3Bit();
        // riscv::fifo::test::FifoTestFastRead4Bit();
        // riscv::fifo::test::FifoTestFastWrite4Bit();
        // riscv::fifo::test::FifoTestFastRead5Bit();
        // riscv::fifo::test::FifoTestFastWrite5Bit();
        tau::TestContainer::Instance().PrintTotals();
        return 0;
    }

    if constexpr(true)
    {
        riscv::test::ALUTestShifter();

        tau::TestContainer::Instance().PrintTotals();
        return 0;
    }

#if 0
    ::tau::test::register_allocator::RunTests();
#endif

    tau::vd::InitSdl();
    Ref<tau::vd::Window> window = tau::vd::Window::CreateWindow();
    Ref<tau::vd::VulkanManager> vulkanManager = tau::vd::VulkanManager::CreateVulkanManager(window);
    ConPrinter::PrintLn("Created vulkan manager.");
    Ref<tau::vd::VulkanCommandPools> vulkanCommandPools = ::tau::vd::VulkanCommandPools::CreateCommandPools(
        vulkanManager->Device(), 
        1, 
        static_cast<u32>(vulkanManager->SwapchainImages().Count()),
        vulkanManager->Device()->GraphicsQueueFamilyIndex()
    );

    if(!vulkanCommandPools)
    {
        return -1;
    }

    vulkanManager->TransitionSwapchain(vulkanCommandPools);

    u8* framebuffer = BuildFramebuffer(window);

    Ref<tau::vd::FramebufferRenderer> frameBufferRenderer = tau::vd::FramebufferRenderer::CreateFramebufferRenderer(
        window,
        vulkanManager->Device(),
        framebuffer,
        0,
        vulkanCommandPools,
        vulkanManager->SwapchainImages(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    window->ResizeCallback() = [&framebuffer, &window, &frameBufferRenderer, &vulkanManager, &vulkanCommandPools](const u32, const u32)
    {
        vulkanManager->Device()->VkQueueWaitIdle(vulkanManager->Device()->GraphicsQueue());
        framebuffer = BuildFramebuffer(window);
        vulkanManager->RebuildSwapchain();
        vulkanManager->TransitionSwapchain(vulkanCommandPools);
        frameBufferRenderer->RebuildBuffers(vulkanManager->SwapchainImages(), framebuffer, 0);
    };

    processor.SetResetN(true);
    processor.GetPciController().VirtualBoxPciPhy().SetVirtualBoxReadResetN(true);

    ::std::thread processorThread([]()
    {
        while(!s_ShouldExit)
        {
            processor.SetClock(true);
            processor.SetClock(false);
            ::std::this_thread::yield();
        }

        ConPrinter::PrintLn(u8"Processor Thread Exiting");
    });

#if defined(_WIN32)
    (void) SetThreadDescription(processorThread.native_handle(), L"SoftGpuProcessorThread");
#elif defined(__linux__)
    // This is limited to 16 characters (including the null terminator) on linux.
    (void) pthread_setname_np(processorThread.native_handle(), "SG-ProcThread");
#endif

    {
        const int commandInit = InitCommandRegister();
        if(commandInit)
        {
            return commandInit;
        }
    }

    {
        const int barInit = InitBAR();
        if(barInit)
        {
            return barInit;
        }
    }

    u32 resetRead;
    // processor.PciMemRead(BAR0 + PciControlRegisters::REGISTER_RESET, 4, &resetRead);

    while(!window->ShouldClose() && !tau::vd::ShouldClose())
    {
//        window->PollMessages();
        tau::vd::PollEvents();

        const u32 frameIndex = vulkanManager->WaitForFrame();
        VkCommandBuffer commandBuffer = frameBufferRenderer->Record(frameIndex, true);
        vulkanManager->SubmitCommandBuffers(1, &commandBuffer, frameIndex);
        vulkanManager->Present(frameIndex);
    }

    s_ShouldExit = true;

    if(vulkanManager->Device()->VkDeviceWaitIdle)
    {
        vulkanManager->Device()->VkDeviceWaitIdle(vulkanManager->Device()->Device());
    }

    processorThread.join();

    ReleaseBARs();

    return 0;
}

static void InitEnvironment() noexcept
{
    Console::Init();

#ifdef _WIN32
    if(SUCCEEDED(DebugManager::Create(&GlobalDebug, L"\\\\.\\pipe\\gpu-pipe-step", L"\\\\.\\pipe\\gpu-pipe-info", true)))
    {
        ConPrinter::Print("Debug enabled.\n");
    }
#endif
}

static void* RawGpuMemory = nullptr;
static u8* GpuMemory = nullptr;

static int InitCommandRegister() noexcept
{
    if constexpr(false)
    {
        u16 commandRegister = static_cast<u16>(processor.PciConfigRead(0x04, 2));
        commandRegister |= 0x6;
        processor.PciConfigWrite(0x04, 2, commandRegister);

        commandRegister = static_cast<u16>(processor.PciConfigRead(0x04, 2));
        if((commandRegister & 0x06) != 0x06)
        {
            return -501;
        }
    }
    else
    {
        u16 commandRegister = processor.GetPciController().VirtualBoxPciPhy().VirtualBoxConfigRead(
            0x004,
            0x3
        );

        commandRegister |= 0x6;
        processor.GetPciController().VirtualBoxPciPhy().VirtualBoxConfigWrite(
            0x004,
            0x3,
            commandRegister
        );

        commandRegister = processor.GetPciController().VirtualBoxPciPhy().VirtualBoxConfigRead(
            0x004,
            0x3
        );
        if((commandRegister & 0x06) != 0x06)
        {
            return -501;
        }
    }

    return 0;
}

static int InitBAR() noexcept
{
    {
        const u32 bar0Request = processor.PciConfigRead(0x10, 4);
        const u32 bar1Request = processor.PciConfigRead(0x14, 4);

        if((bar0Request & 0x1) != 0)
        {
            ConPrinter::PrintLn("BAR 0 requesting IO space is not allowed.");
            return -101;
        }

        if(((bar0Request >> 1) & 0x3) != 0)
        {
            ConPrinter::PrintLn("Expected BAR 0 to request 32 bit memory space.");
            return -102;
        }

        if(((bar0Request >> 3) & 0x1) != 0)
        {
            ConPrinter::PrintLn("Expected BAR 0 to request non-prefetchable memory.");
            return -103;
        }

        if((bar1Request & 0x1) != 0)
        {
            ConPrinter::PrintLn("BAR 1 requesting IO space is not allowed.");
            return -104;
        }

        if(((bar1Request >> 1) & 0x3) != 2)
        {
            ConPrinter::PrintLn("Expected BAR 1 to request 64 bit memory space.");
            return -105;
        }

        if(((bar1Request >> 3) & 0x1) != 1)
        {
            ConPrinter::PrintLn("Expected BAR 1 to request prefetchable memory.");
            return -106;
        }
    }

    processor.PciConfigWrite(0x10, 4, 0xFFFFFFFF);
    processor.PciConfigWrite(0x14, 4, 0xFFFFFFFF);
    processor.PciConfigWrite(0x18, 4, 0xFFFFFFFF);

    const u32 bar0ActiveResponse = processor.PciConfigRead(0x10, 4) & 0xFFFFFFF0;
    const u32 bar1ActiveResponse = processor.PciConfigRead(0x14, 4) & 0xFFFFFFF0;
    const u32 bar2ActiveResponse = processor.PciConfigRead(0x18, 4);
    
    const u32 bar0InactiveBits = 32 - ::std::popcount(bar0ActiveResponse);
    const u64 bar1InactiveBits = 64 - (::std::popcount(bar1ActiveResponse) + ::std::popcount(bar2ActiveResponse));

    const u32 bar0Size = 1 << bar0InactiveBits;
    const u64 bar1Size = 1ull << bar1InactiveBits;

    // Random offsets for testing.
    BAR0 = bar0Size * 0x03;

    {
        // Enough pages to ensure that the data is aligned.
        const u64 gpuPageCount = (bar1Size * 2) / PageAllocator::PageSize() - 1;

        RawGpuMemory = PageAllocator::Reserve(gpuPageCount);
        
        if(!RawGpuMemory)
        {
            ConPrinter::PrintLn("Failed to reserve GPU memory pages.");
            return  -107;
        }

        const u64 virtualAllocAddress = reinterpret_cast<u64>(RawGpuMemory);
        const u64 alignedVirtualAddress = virtualAllocAddress + (-virtualAllocAddress & (bar1Size - 1));

        GpuMemory = reinterpret_cast<u8*>(alignedVirtualAddress);

        BAR1 = alignedVirtualAddress;
    }

    processor.PciConfigWrite(0x10, 4, BAR0);
    processor.PciConfigWrite(0x14, 4, static_cast<u32>(BAR1));
    processor.PciConfigWrite(0x18, 4, static_cast<u32>(BAR1 >> 32));
    processor.TestSetRamBaseAddress(BAR1, bar1Size);

    ConPrinter::PrintLn("Loaded BAR0 at 0x{XP0}.", BAR0);
    ConPrinter::PrintLn("Loaded BAR1 at 0x{XP0}.", BAR1);

    return 0;
}

static void ReleaseBARs() noexcept
{
    GpuMemory = nullptr;

    PageAllocator::Free(RawGpuMemory);

    RawGpuMemory = nullptr;
}
