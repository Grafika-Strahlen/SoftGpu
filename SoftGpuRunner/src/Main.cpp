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

static Processor processor;
DebugManager GlobalDebug;

static u32 BAR0 = 0;
static u64 BAR1 = 0;

static void InitEnvironment() noexcept;
static int InitCommandRegister() noexcept;
static int InitBAR() noexcept;
static int InitMmu() noexcept;
static void ReleaseMmu() noexcept;

[[maybe_unused]] static void TestMove() noexcept;
[[maybe_unused]] static void TestAdd1F() noexcept;
[[maybe_unused]] static void TestAdd2F() noexcept;
[[maybe_unused]] static void TestAdd2FReplicated() noexcept;
[[maybe_unused]] static void TestMul2FReplicated() noexcept;
[[maybe_unused]] static void TestMul2FReplicatedDualDispatch() noexcept;

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

    {
        const int mmuInit = InitMmu();
        if(mmuInit)
        {
            return mmuInit;
        }
    }

    u32 resetRead;
    // processor.PciMemRead(BAR0 + PciControlRegisters::REGISTER_RESET, 4, &resetRead);

    while(!window->ShouldClose() && !tau::vd::ShouldClose())
    {
//        window->PollMessages();
        tau::vd::PollEvents();

        const u32 frameIndex = vulkanManager->WaitForFrame();
        VkCommandBuffer commandBuffer = frameBufferRenderer->Record(frameIndex);
        vulkanManager->SubmitCommandBuffers(1, &commandBuffer);
        vulkanManager->Present(frameIndex);
    }

    if(vulkanManager->Device()->VkDeviceWaitIdle)
    {
        vulkanManager->Device()->VkDeviceWaitIdle(vulkanManager->Device()->Device());
    }

    // TestMove();
    // processor.PciMemRead(BAR0 + REGISTER_RESET, 4, &resetRead);
    // TestAdd1F();
    // processor.PciMemRead(BAR0 + REGISTER_RESET, 4, &resetRead);
    // TestAdd2F();
    // processor.PciMemRead(BAR0 + REGISTER_RESET, 4, &resetRead);
    // TestAdd2FReplicated();
    // processor.PciMemRead(BAR0 + REGISTER_RESET, 4, &resetRead);
    // TestMul2FReplicated();
    // processor.PciMemRead(BAR0 + REGISTER_RESET, 4, &resetRead);
    TestMul2FReplicatedDualDispatch();
    // processor.PciMemRead(BAR0 + PciControlRegisters::REGISTER_RESET, 4, &resetRead);

    ReleaseMmu();

    return 0;
}

static PageEntry* PageDirectory = nullptr;
static PageEntry* PageTable0 = nullptr;
static void* RawBuffer = nullptr;
static void* ExecutableBuffer = nullptr;

static inline constexpr u64 ExecutableStart = 0x10000;

static void TestMove() noexcept
{
    processor.LoadPageDirectoryPointer(0, PageDirectory);

    constexpr u32 initialValue = 42;

    // processor.PciMemWrite(BAR1 + GpuPageSize * 2 + 0, 4, &initialValue);

    u32* const dataBuffer = reinterpret_cast<u32*>(RawBuffer);
    
    u32* const pStorageLocation = &dataBuffer[1];
    
    *pStorageLocation = 0xCDCDCDCD;
    
    constexpr u8 program[] =
    {
        static_cast<u8>(EInstruction::LoadImmediate), 0, 0, 0, 0, 0,
        static_cast<u8>(EInstruction::LoadImmediate), 1, 0, 0, 0, 0,
        static_cast<u8>(EInstruction::LoadImmediate), 2, 1, 0, 0, 0,
        static_cast<u8>(EInstruction::LoadImmediate), 3, 0, 0, 0, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b00111000, 0, 4, 0, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b01111000, 2, 4, 0, 0,
        static_cast<u8>(EInstruction::FlushCache),
        static_cast<u8>(EInstruction::Hlt)
    };

    (void) ::std::memcpy(ExecutableBuffer, program, sizeof(program));
    
    ConPrinter::PrintLn("Value to copy: {}", initialValue);
    ConPrinter::PrintLn("Value at storage location: 0x{X}", *pStorageLocation);

    processor.TestLoadProgram(0, 0, 0x1, ExecutableStart);

    for(int i = 0; i < 20; ++i)
    {
        processor.Clock();
    }
    
    ConPrinter::PrintLn("Value to copy: {}", initialValue);
    ConPrinter::PrintLn("Value at storage location: {}", *pStorageLocation);
}

static void TestAdd1F() noexcept
{
    processor.LoadPageDirectoryPointer(0, PageDirectory);
    
    f32* const dataBuffer = reinterpret_cast<f32*>(RawBuffer);

    f32* const pValueA = &dataBuffer[0];
    f32* const pStorageValue = &dataBuffer[1];

    *pValueA = 1.5f;
    *pStorageValue = ::std::numeric_limits<f32>::quiet_NaN();
    
    constexpr f32 valueB = 2.5f;

    u8 valueBBytes[8];
    (void) ::std::memcpy(valueBBytes, &valueB, sizeof(valueB));

    const u8 program[] =
    {
        static_cast<u8>(EInstruction::LoadImmediate), 0, 0, 0, 0, 0,
        static_cast<u8>(EInstruction::LoadImmediate), 1, 0, 0, 0, 0,
        static_cast<u8>(EInstruction::LoadImmediate), 2, 1, 0, 0, 0,
        static_cast<u8>(EInstruction::LoadImmediate), 3, 0, 0, 0, 0,
        static_cast<u8>(EInstruction::LoadImmediate), 4, valueBBytes[0], valueBBytes[1], valueBBytes[2], valueBBytes[3],
        static_cast<u8>(EInstruction::LoadStore), 0b00111000, 0, 5, 0, 0,
        static_cast<u8>(EInstruction::AddF), 5, 4, 6,
        static_cast<u8>(EInstruction::LoadStore), 0b01111000, 2, 6, 0, 0,
        static_cast<u8>(EInstruction::FlushCache),
        static_cast<u8>(EInstruction::Hlt)
    };

    (void) ::std::memcpy(ExecutableBuffer, program, sizeof(program));

    ConPrinter::PrintLn("{} + {} = {}",*pValueA, valueB, *pStorageValue);

    processor.TestLoadProgram(0, 0, 0x1, ExecutableStart);

    for(int i = 0; i < 40; ++i)
    {
        processor.Clock();
    }

    // Should print 4.0
    ConPrinter::PrintLn("{} + {} = {}", *pValueA, valueB, *pStorageValue);
}

static void TestAdd2F() noexcept
{
    processor.LoadPageDirectoryPointer(0, PageDirectory);

    f32* const dataBuffer = reinterpret_cast<f32*>(RawBuffer);

    f32* const pValueA = &dataBuffer[0];
    f32* const pValueB = &dataBuffer[2];
    f32* const pStorageValue = &dataBuffer[4];

    pValueA[0] = 1.5f;
    pValueA[1] = 1.0f;
    pValueB[0] = 2.5f;
    pValueB[1] = -2.5f;
    pStorageValue[0] = ::std::numeric_limits<f32>::quiet_NaN();
    pStorageValue[1] = ::std::numeric_limits<f32>::quiet_NaN();
    
    constexpr u8 program[] =
    {
        static_cast<u8>(EInstruction::LoadImmediate), 0, 0, 0, 0, 0,
        static_cast<u8>(EInstruction::LoadImmediate), 1, 0, 0, 0, 0,
        static_cast<u8>(EInstruction::LoadImmediate), 2, 2, 0, 0, 0,
        static_cast<u8>(EInstruction::LoadImmediate), 3, 0, 0, 0, 0,
        static_cast<u8>(EInstruction::LoadImmediate), 4, 4, 0, 0, 0,
        static_cast<u8>(EInstruction::LoadImmediate), 5, 0, 0, 0, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b00111001, 0, 6, 0, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b00111001, 2, 8, 0, 0,
        static_cast<u8>(EInstruction::AddVec2F), 6, 8, 10,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 4, 10, 0, 0,
        static_cast<u8>(EInstruction::FlushCache),
        static_cast<u8>(EInstruction::Hlt)
    };

    (void) ::std::memcpy(ExecutableBuffer, program, sizeof(program));

    ConPrinter::PrintLn("[{}, {}] + [{}, {}] = [{}, {}]", pValueA[0], pValueA[1], pValueB[0], pValueB[1], pStorageValue[0], pStorageValue[1]);

    processor.TestLoadProgram(0, 0, 0x1, ExecutableStart);

    for(int i = 0; i < 20; ++i)
    {
        processor.Clock();
    }

    // Should print [4.0, -1.5]
    ConPrinter::PrintLn("[{}, {}] + [{}, {}] = [{}, {}]", pValueA[0], pValueA[1], pValueB[0], pValueB[1], pStorageValue[0], pStorageValue[1]);
}

static void TestAdd2FReplicated() noexcept
{
    processor.LoadPageDirectoryPointer(0, PageDirectory);

    struct InputData
    {
        f32 ValueA[2];
        f32 ValueB[2];
    };

    struct OutputData
    {
        f32 ValueResult[2];
    };

    InputData* const inData = reinterpret_cast<InputData*>(RawBuffer);
    OutputData* const outData = reinterpret_cast<OutputData*>(inData + 2);

    inData[0].ValueA[0] = 1.5f;
    inData[0].ValueA[1] = 1.0f;
    inData[0].ValueB[0] = 2.5f;
    inData[0].ValueB[1] = -2.5f;

    inData[1].ValueA[0] = 3.0f;
    inData[1].ValueA[1] = -1.0f;
    inData[1].ValueB[0] = 2.5f;
    inData[1].ValueB[1] = -2.5f;

    outData[0].ValueResult[0] = ::std::numeric_limits<f32>::quiet_NaN();
    outData[0].ValueResult[1] = ::std::numeric_limits<f32>::quiet_NaN();
    outData[1].ValueResult[0] = ::std::numeric_limits<f32>::quiet_NaN();
    outData[1].ValueResult[1] = ::std::numeric_limits<f32>::quiet_NaN();
    
    constexpr u8 program[] =
    {
        static_cast<u8>(EInstruction::LoadStore), 0b00111011, 0, 6, 0, 0, // Load inData.ValueA and inData.ValueB
        static_cast<u8>(EInstruction::AddVec2F), 6, 8, 10,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 10, 0, 0, // Store inData.ValueA + inData.ValueB in outData.ValueResult
        static_cast<u8>(EInstruction::FlushCache),
        static_cast<u8>(EInstruction::Hlt)
    };

    (void) ::std::memcpy(ExecutableBuffer, program, sizeof(program));

    ConPrinter::PrintLn("[{}, {}] + [{}, {}] = [{}, {}]", inData[0].ValueA[0], inData[0].ValueA[1], inData[0].ValueB[0], inData[0].ValueB[1], outData[0].ValueResult[0], outData[0].ValueResult[1]);
    ConPrinter::PrintLn("[{}, {}] + [{}, {}] = [{}, {}]", inData[1].ValueA[0], inData[1].ValueA[1], inData[1].ValueB[0], inData[1].ValueB[1], outData[1].ValueResult[0], outData[1].ValueResult[1]);

    processor.TestLoadProgram(0, 0, 0x3, ExecutableStart);

    processor.TestLoadRegister(0, 0, 0, 0, 0);
    processor.TestLoadRegister(0, 0, 0, 1, 0);
    processor.TestLoadRegister(0, 0, 1, 0, 4);
    processor.TestLoadRegister(0, 0, 1, 1, 0);
    processor.TestLoadRegister(0, 0, 0, 2, 8);
    processor.TestLoadRegister(0, 0, 0, 3, 0);
    processor.TestLoadRegister(0, 0, 1, 2, 10);
    processor.TestLoadRegister(0, 0, 1, 3, 0);

    for(int i = 0; i < 100; ++i)
    {
        processor.Clock();
    }

    // Should print
    //   [4.0, -1.5]
    //   [5.5, -3.5]
    ConPrinter::PrintLn("[{}, {}] + [{}, {}] = [{}, {}]", inData[0].ValueA[0], inData[0].ValueA[1], inData[0].ValueB[0], inData[0].ValueB[1], outData[0].ValueResult[0], outData[0].ValueResult[1]);
    ConPrinter::PrintLn("[{}, {}] + [{}, {}] = [{}, {}]", inData[1].ValueA[0], inData[1].ValueA[1], inData[1].ValueB[0], inData[1].ValueB[1], outData[1].ValueResult[0], outData[1].ValueResult[1]);
}

static void TestMul2FReplicated() noexcept
{
    processor.LoadPageDirectoryPointer(0, PageDirectory);

    struct InputData
    {
        f32 ValueA[2];
        f32 ValueB[2];
    };

    struct OutputData
    {
        f32 ValueResult[2];
        u64 FpSaturation;
        u64 IntFpSaturation;
        u64 LdStSaturation;
        u64 FpTotals;
        u64 IntFpTotals;
        u64 LdStTotals;
    };

    InputData* const inData = reinterpret_cast<InputData*>(RawBuffer);
    OutputData* const outData = reinterpret_cast<OutputData*>(inData + 4);
    
    inData[0].ValueA[0] = 1.5f;
    inData[0].ValueA[1] = 1.0f;
    inData[0].ValueB[0] = 2.5f;
    inData[0].ValueB[1] = -2.5f;

    inData[1].ValueA[0] = 3.0f;
    inData[1].ValueA[1] = -1.0f;
    inData[1].ValueB[0] = 2.5f;
    inData[1].ValueB[1] = -2.5f;

    inData[2].ValueA[0] = 0.5f;
    inData[2].ValueA[1] = 2.0f;
    inData[2].ValueB[0] = 2.5f;
    inData[2].ValueB[1] = -2.5f;

    inData[3].ValueA[0] = 5.3f;
    inData[3].ValueA[1] = 0.0f;
    inData[3].ValueB[0] = 0.1f;
    inData[3].ValueB[1] = -2.5f;

    for(u32 i = 0; i < 4; ++i)
    {
        outData[i].ValueResult[0] = ::std::numeric_limits<f32>::quiet_NaN();
        outData[i].ValueResult[1] = ::std::numeric_limits<f32>::quiet_NaN();
        outData[i].FpSaturation = 0;
        outData[i].IntFpSaturation = 0;
        outData[i].LdStSaturation = 0;
        outData[i].FpTotals = 0;
        outData[i].IntFpTotals = 0;
        outData[i].LdStTotals = 0;
    }

    // const uintptr_t inData0Ptr = reinterpret_cast<uintptr_t>(&inData[0]) >> 2;
    // u32 inData0Words[2];
    // (void) ::std::memcpy(inData0Words, &inData0Ptr, sizeof(inData0Ptr));
    //
    // const uintptr_t inData1Ptr = reinterpret_cast<uintptr_t>(&inData[1]) >> 2;
    // u32 inData1Words[2];
    // (void) ::std::memcpy(inData1Words, &inData1Ptr, sizeof(inData1Ptr));
    //
    // const uintptr_t inData2Ptr = reinterpret_cast<uintptr_t>(&inData[2]) >> 2;
    // u32 inData2Words[2];
    // (void) ::std::memcpy(inData2Words, &inData2Ptr, sizeof(inData2Ptr));
    //
    // const uintptr_t inData3Ptr = reinterpret_cast<uintptr_t>(&inData[3]) >> 2;
    // u32 inData3Words[2];
    // (void) ::std::memcpy(inData3Words, &inData3Ptr, sizeof(inData3Ptr));
    //
    // const uintptr_t outData0Ptr = reinterpret_cast<uintptr_t>(&outData[0]) >> 2;
    // u32 outData0Words[2];
    // (void) ::std::memcpy(outData0Words, &outData0Ptr, sizeof(outData0Ptr));
    //
    // const uintptr_t outData1Ptr = reinterpret_cast<uintptr_t>(&outData[1]) >> 2;
    // u32 outData1Words[2];
    // (void) ::std::memcpy(outData1Words, &outData1Ptr, sizeof(outData1Ptr));
    //
    // const uintptr_t outData2Ptr = reinterpret_cast<uintptr_t>(&outData[2]) >> 2;
    // u32 outData2Words[2];
    // (void) ::std::memcpy(outData2Words, &outData2Ptr, sizeof(outData2Ptr));
    //
    // const uintptr_t outData3Ptr = reinterpret_cast<uintptr_t>(&outData[3]) >> 2;
    // u32 outData3Words[2];
    // (void) ::std::memcpy(outData3Words, &outData3Ptr, sizeof(outData3Ptr));

    constexpr u8 program[] =
    {
        static_cast<u8>(EInstruction::LoadStore), 0b00111011, 0, 6, 0, 0, // Load inData.ValueA and inData.ValueB
        static_cast<u8>(EInstruction::MulVec2F), 6, 8, 10,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 10, 0, 0, // Store inData.ValueA + inData.ValueB in outData.ValueResult
        static_cast<u8>(EInstruction::WriteStatistics), 0, 12, 14,
        static_cast<u8>(EInstruction::WriteStatistics), 1, 16, 18,
        static_cast<u8>(EInstruction::WriteStatistics), 2, 20, 22,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 12, 2, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 16, 4, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 20, 6, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 14, 8, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 18, 10, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 22, 12, 0,
        static_cast<u8>(EInstruction::FlushCache),
        static_cast<u8>(EInstruction::Hlt)
    };

    (void) ::std::memcpy(ExecutableBuffer, program, sizeof(program));

    for(u32 i = 0; i < 4; ++i)
    {
        ConPrinter::Print("[{}, {}] * [{}, {}] = [{}, {}]\n", inData[i].ValueA[0], inData[i].ValueA[1], inData[i].ValueB[0], inData[i].ValueB[1], outData[i].ValueResult[0], outData[i].ValueResult[1]);
    }

    processor.TestLoadProgram(0, 0, 0xF, ExecutableStart);

    processor.TestLoadRegister(0, 0, 0, 0, 0);
    processor.TestLoadRegister(0, 0, 0, 1, 0);
    processor.TestLoadRegister(0, 0, 1, 0, 4);
    processor.TestLoadRegister(0, 0, 1, 1, 0);
    processor.TestLoadRegister(0, 0, 2, 0, 8);
    processor.TestLoadRegister(0, 0, 2, 1, 0);
    processor.TestLoadRegister(0, 0, 3, 0, 12);
    processor.TestLoadRegister(0, 0, 3, 1, 0);
    processor.TestLoadRegister(0, 0, 0, 2, 16);
    processor.TestLoadRegister(0, 0, 0, 3, 0);
    processor.TestLoadRegister(0, 0, 1, 2, 30);
    processor.TestLoadRegister(0, 0, 1, 3, 0);
    processor.TestLoadRegister(0, 0, 2, 2, 44);
    processor.TestLoadRegister(0, 0, 2, 3, 0);
    processor.TestLoadRegister(0, 0, 3, 2, 58);
    processor.TestLoadRegister(0, 0, 3, 3, 0);

    for(int i = 0; i < 80; ++i)
    {
        processor.Clock();
    }

    for(u32 i = 0; i < 4; ++i)
    {
        ConPrinter::Print("[{}, {}] * [{}, {}] = [{}, {}]\n", inData[i].ValueA[0], inData[i].ValueA[1], inData[i].ValueB[0], inData[i].ValueB[1], outData[i].ValueResult[0], outData[i].ValueResult[1]);
    }

    const f64 fpSaturation = static_cast<f64>(outData[0].FpSaturation) / static_cast<f64>(outData[0].FpTotals);
    const f64 intFpSaturation = static_cast<f64>(outData[0].IntFpSaturation) / static_cast<f64>(outData[0].IntFpTotals);
    const f64 ldStSaturation = static_cast<f64>(outData[0].LdStSaturation) / static_cast<f64>(outData[0].LdStTotals);

    ConPrinter::Print("FP Saturation: {}:{} ({})\n", outData[0].FpSaturation, outData[0].FpTotals, fpSaturation);
    ConPrinter::Print("Int/FP Saturation: {}:{} ({})\n", outData[0].IntFpSaturation, outData[0].IntFpTotals, intFpSaturation);
    ConPrinter::Print("LD/ST Saturation: {}:{} ({})\n", outData[0].LdStSaturation, outData[0].LdStTotals, ldStSaturation);
}

static void TestMul2FReplicatedDualDispatch() noexcept
{
    processor.LoadPageDirectoryPointer(0, PageDirectory);

    struct InputData
    {
        f32 ValueA[2];
        f32 ValueB[2];
    };

    struct OutputData
    {
        f32 ValueResult[2];
        u64 FpSaturation;
        u64 IntFpSaturation;
        u64 LdStSaturation;
        u64 FpTotals;
        u64 IntFpTotals;
        u64 LdStTotals;
    };

    InputData* const inData = reinterpret_cast<InputData*>(RawBuffer);
    OutputData* const outData = reinterpret_cast<OutputData*>(inData + 4);

    inData[0].ValueA[0] = 1.5f;
    inData[0].ValueA[1] = 1.0f;
    inData[0].ValueB[0] = 2.5f;
    inData[0].ValueB[1] = -2.5f;

    inData[1].ValueA[0] = 3.0f;
    inData[1].ValueA[1] = -1.0f;
    inData[1].ValueB[0] = 2.5f;
    inData[1].ValueB[1] = -2.5f;

    inData[2].ValueA[0] = 0.5f;
    inData[2].ValueA[1] = 2.0f;
    inData[2].ValueB[0] = 2.5f;
    inData[2].ValueB[1] = -2.5f;

    inData[3].ValueA[0] = 5.3f;
    inData[3].ValueA[1] = 0.0f;
    inData[3].ValueB[0] = 0.1f;
    inData[3].ValueB[1] = -2.5f;

    for(u32 i = 0; i < 4; ++i)
    {
        outData[i].ValueResult[0] = ::std::numeric_limits<f32>::quiet_NaN();
        outData[i].ValueResult[1] = ::std::numeric_limits<f32>::quiet_NaN();
        outData[i].FpSaturation = 0;
        outData[i].IntFpSaturation = 0;
        outData[i].LdStSaturation = 0;
        outData[i].FpTotals = 0;
        outData[i].IntFpTotals = 0;
        outData[i].LdStTotals = 0;
    }
    
    constexpr u8 program[] =
    {
        static_cast<u8>(EInstruction::LoadStore), 0b00111011, 0, 6, 0, 0, // Load inData.ValueA and inData.ValueB
        static_cast<u8>(EInstruction::MulVec2F), 6, 8, 10,
        static_cast<u8>(EInstruction::WriteStatistics), 0, 12, 14,
        static_cast<u8>(EInstruction::WriteStatistics), 1, 16, 18,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 10, 0, 0, // Store inData.ValueA + inData.ValueB in outData.ValueResult
        static_cast<u8>(EInstruction::WriteStatistics), 2, 20, 22,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 12, 2, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 16, 4, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 20, 6, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 14, 8, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 18, 10, 0,
        static_cast<u8>(EInstruction::LoadStore), 0b01111001, 2, 22, 12, 0,
        static_cast<u8>(EInstruction::FlushCache),
        static_cast<u8>(EInstruction::Hlt)
    };

    (void) ::std::memcpy(ExecutableBuffer, program, sizeof(program));

    for(u32 i = 0; i < 4; ++i)
    {
        ConPrinter::PrintLn("[{}, {}] * [{}, {}] = [{}, {}]", inData[i].ValueA[0], inData[i].ValueA[1], inData[i].ValueB[0], inData[i].ValueB[1], outData[i].ValueResult[0], outData[i].ValueResult[1]);
    }

    for(u32 i = 0; i < 2; ++i)
    {
        processor.TestLoadProgram(0, i, 0xF, ExecutableStart);

        processor.TestLoadRegister(0, i, 0, 0, 0);
        processor.TestLoadRegister(0, i, 0, 1, 0);
        processor.TestLoadRegister(0, i, 1, 0, 4);
        processor.TestLoadRegister(0, i, 1, 1, 0);
        processor.TestLoadRegister(0, i, 2, 0, 8);
        processor.TestLoadRegister(0, i, 2, 1, 0);
        processor.TestLoadRegister(0, i, 3, 0, 12);
        processor.TestLoadRegister(0, i, 3, 1, 0);
        processor.TestLoadRegister(0, i, 0, 2, 16);
        processor.TestLoadRegister(0, i, 0, 3, 0);
        processor.TestLoadRegister(0, i, 1, 2, 30);
        processor.TestLoadRegister(0, i, 1, 3, 0);
        processor.TestLoadRegister(0, i, 2, 2, 44);
        processor.TestLoadRegister(0, i, 2, 3, 0);
        processor.TestLoadRegister(0, i, 3, 2, 58);
        processor.TestLoadRegister(0, i, 3, 3, 0);
    }

    for(int i = 0; i < 500; ++i)
    {
        processor.Clock();
    }

    for(u32 i = 0; i < 4; ++i)
    {
        ConPrinter::PrintLn("[{}, {}] * [{}, {}] = [{}, {}]", inData[i].ValueA[0], inData[i].ValueA[1], inData[i].ValueB[0], inData[i].ValueB[1], outData[i].ValueResult[0], outData[i].ValueResult[1]);
    }

    const u64 totalCoreSaturation = outData[0].FpSaturation + outData[0].IntFpSaturation;
    const f64 totalCoreTotals = static_cast<f64>(outData[0].FpTotals + outData[0].IntFpTotals) / 2.0;

    const f64 fpPercent = static_cast<f64>(outData[0].FpSaturation) / static_cast<f64>(outData[0].FpTotals);
    const f64 intFpPercent = static_cast<f64>(outData[0].IntFpSaturation) / static_cast<f64>(outData[0].IntFpTotals);
    const f64 totalCorePercent = static_cast<f64>(totalCoreSaturation) / totalCoreTotals;
    const f64 ldStPercent = static_cast<f64>(outData[0].LdStSaturation) / static_cast<f64>(outData[0].LdStTotals);

    ConPrinter::PrintLn("FP Saturation: {}:{} ({})", outData[0].FpSaturation, outData[0].FpTotals, fpPercent);
    ConPrinter::PrintLn("Int/FP Saturation: {}:{} ({})", outData[0].IntFpSaturation, outData[0].IntFpTotals, intFpPercent);
    ConPrinter::PrintLn("Total Core Saturation: {}:{} ({})", totalCoreSaturation, static_cast<u64>(totalCoreTotals), totalCorePercent);
    ConPrinter::PrintLn("LD/ST Saturation: {}:{} ({})", outData[0].LdStSaturation, outData[0].LdStTotals, ldStPercent);
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

static int InitMmu() noexcept
{
    const u64 gpuMemoryAddress = reinterpret_cast<u64>(GpuMemory);

    // Four blocks, 1 block for the Page Directory, 1 block for the Page Table, 1 block of usable memory, 1 block of executable memory.
    if(!PageAllocator::CommitPages(GpuMemory, GpuPageSize * 4))
    {
        ConPrinter::PrintLn("Failed to commit virtual pages.");
        return -202;
    }

    PageDirectory = reinterpret_cast<PageEntry*>(GpuMemory);
    PageTable0 = reinterpret_cast<PageEntry*>(GpuMemory + GpuPageSize);
    RawBuffer = reinterpret_cast<PageEntry*>(GpuMemory + GpuPageSize * 2);
    ExecutableBuffer = reinterpret_cast<PageEntry*>(GpuMemory + GpuPageSize * 3);

    (void) ::std::memset(GpuMemory, 0, sizeof(GpuPageSize) * 4);

    PageTable0[0].Present = true;
    PageTable0[0].ReadWrite = true;
    PageTable0[0].Execute = false;
    PageTable0[0].WriteThrough = false;
    PageTable0[0].CacheDisable = false;
    PageTable0[0].Accessed = false;
    PageTable0[0].Dirty = false;
    PageTable0[0].External = false;
    PageTable0[0].PhysicalAddress = (gpuMemoryAddress >> 16) + 2;
    PageTable0[0].Reserved1 = 0;

    PageTable0[1].Present = true;
    PageTable0[1].ReadWrite = false;
    PageTable0[1].Execute = true;
    PageTable0[1].WriteThrough = false;
    PageTable0[1].CacheDisable = false;
    PageTable0[1].Accessed = false;
    PageTable0[1].Dirty = false;
    PageTable0[1].External = false;
    PageTable0[1].PhysicalAddress = (gpuMemoryAddress >> 16) + 3;
    PageTable0[1].Reserved1 = 0;

    PageDirectory[0].Present = true;
    PageDirectory[0].ReadWrite = true;
    PageDirectory[0].Execute = false;
    PageDirectory[0].WriteThrough = false;
    PageDirectory[0].CacheDisable = false;
    PageDirectory[0].Accessed = false;
    PageDirectory[0].Dirty = false;
    PageDirectory[0].External = false;
    PageDirectory[0].PhysicalAddress = (gpuMemoryAddress >> 16) + 1;
    PageDirectory[0].Reserved1 = 0;

    return 0;
}

static void ReleaseMmu() noexcept
{
    PageDirectory = nullptr;
    PageTable0 = nullptr;
    RawBuffer = nullptr;
    ExecutableBuffer = nullptr;
    GpuMemory = nullptr;

    PageAllocator::Free(RawGpuMemory);

    RawGpuMemory = nullptr;
}
