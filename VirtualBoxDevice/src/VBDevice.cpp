// ReSharper disable CppEqualOperandsInBinaryExpression
#define LOG_GROUP LOG_GROUP_MISC

#define RT_STRICT
#include <VBox/com/assert.h>
#include <VBox/com/defs.h>
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/version.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <VBox/msi.h>

#include <VBox/com/string.h>
#include <VBox/com/VirtualBox.h>

#include <iprt/ctype.h>
#include <iprt/assert.h>

#include <Processor.hpp>
#include <DebugManager.hpp>
#include <Console.hpp>
#include <vd/VulkanManager.hpp>
#include "ConLogger.hpp"
#include "VBoxDeviceFunction.hpp"
#include "PciConfigHandler.hpp"
#include "vd/FramebufferRenderer.hpp"
#include "vd/VulkanCommandPools.hpp"
#include "vd/Window.hpp"
#include <chrono>

DebugManager GlobalDebug;

#undef PDMPCIDEV_ASSERT_VALID

//   The original version of this has a typo, making it dependent on a
// variable name, rather than a macro parameter name.
#define PDMPCIDEV_ASSERT_VALID(a_pDevIns, a_pPciDev) \
    do { \
        uintptr_t const offPciDevInTable = reinterpret_cast<uintptr_t>(a_pPciDev) - reinterpret_cast<uintptr_t>(a_pDevIns->apPciDevs[0]); \
        uint32_t  const cbPciDevTmp      = a_pDevIns->cbPciDev; \
        ASMCompilerBarrier(); \
        AssertMsg(   offPciDevInTable < static_cast<uintptr_t>(a_pDevIns->cPciDevs * cbPciDevTmp) \
                  && cbPciDevTmp >= RT_UOFFSETOF(PDMPCIDEV, abConfig) + 256 \
                  && offPciDevInTable % cbPciDevTmp == 0, \
                  ("pPciDev=%p apPciDevs[0]=%p offPciDevInTable=%p cPciDevs=%#x cbPciDev=%#x\n", \
                   (a_pPciDev), a_pDevIns->apPciDevs[0], offPciDevInTable, a_pDevIns->cPciDevs, cbPciDevTmp)); \
        AssertPtr((a_pPciDev)); \
        AssertMsg((a_pPciDev)->u32Magic == PDMPCIDEV_MAGIC, ("%#x\n", (a_pPciDev)->u32Magic)); \
    } while (false)

[[maybe_unused]] static DECLCALLBACK(void*) softGpuPortQueryInterface(PPDMIBASE pInterface, const char* const iid)
{
    (void) pInterface;
    (void) iid;

    return nullptr;
}

static void PciControlWriteCallback(const u32 address, const u32 value) noexcept
{
    if(address == PciControlRegisters::REGISTER_DEBUG_PRINT)
    {
        ConLogLn("SoftGpu/[0]: Writing Debug Register: {} [0x{XP0}]", value, value);
    }
    else if(address == PciControlRegisters::REGISTER_DEBUG_LOG_MULTI)
    {
        ConLog("%c", value);
    }
}

static DECLCALLBACK(VBOXSTRICTRC) softGpuMMIORead(PPDMDEVINS pDevIns, void* pvUser, RTGCPHYS off, void* pv, unsigned cb) noexcept
{
    SoftGpuDeviceFunction* pFun = static_cast<SoftGpuDeviceFunction*>(pvUser);
    NOREF(pDevIns);

#ifdef LOG_ENABLED
    // unsigned const cbLog = cb;
    // const RTGCPHYS       offLog = off;
#endif
    // uint8_t* pbDst = static_cast<uint8_t*>(pv);
    // while(cb-- > 0)
    // {
    //     *pbDst = pFun->abBacking[off % std::size(pFun->abBacking)];
    //     pbDst++;
    //     off++;
    // }
    

    // pFun->Processor.PciMemRead(off, static_cast<u16>(cb), reinterpret_cast<u32*>(pv));

    // Just fast-track BAR1 reads.
    // This won't work once we have virtual memory running...
    if constexpr(true)
    {
        if(pFun->Processor.GetPciController().GetBARFromAddress(off) == 1)
        {
            const u64 address = pFun->Processor.GetPciController().GetBAROffset(off, 1) + pFun->Processor.RamBaseAddress();
            (void) ::std::memcpy(pv, reinterpret_cast<void*>(address), cb);

            return VINF_SUCCESS;
        }
    }

    u16 response;
    pFun->Processor.PciMemReadSet(off, static_cast<u16>(cb), reinterpret_cast<u32*>(pv), &response);

    const DWORD waitResult = WaitForSingleObject(pFun->ProcessorSyncEvent, 250);

    // ConLogLn("SoftGpu/[{}]: READ off=0x{XP0} cb={}: 0x{XP0}", pFun->FunctionId, off, cb, *reinterpret_cast<u32*>(pv));

    if(waitResult == WAIT_TIMEOUT)
    {
        ConLogLn("Read wait timed out: 0x{XP0} 0x{XP0}", off, cb);
    }
    else if(waitResult == WAIT_FAILED)
    {
        ConLogLn("Wait failed: {}", GetLastError());
    }

    return VINF_SUCCESS;
}

static DECLCALLBACK(VBOXSTRICTRC) softGpuMMIOWrite(PPDMDEVINS pDevIns, void* pvUser, RTGCPHYS off, void const* pv, unsigned cb) noexcept
{
    SoftGpuDeviceFunction* pFun = static_cast<SoftGpuDeviceFunction*>(pvUser);
    NOREF(pDevIns);
    // ConLogLn("SoftGpu/[{}]: WRITE off=0x{XP0} cb={}: 0x{XP0}\n", pFun->FunctionId, off, cb, cb, pv);

    // uint8_t const* pbSrc = static_cast<uint8_t const*>(pv);
    // while(cb-- > 0)
    // {
    //     pFun->abBacking[off % std::size(pFun->abBacking)] = *pbSrc;
    //     pbSrc++;
    //     off++;
    // }

    // pFun->Processor.PciMemWrite(off, static_cast<u16>(cb), reinterpret_cast<const u32*>(pv));

    // Just fast-track BAR1 writes.
    // This won't work once we have virtual memory running...
    if constexpr(true)
    {
        if(pFun->Processor.GetPciController().GetBARFromAddress(off) == 1)
        {
            const u64 address = pFun->Processor.GetPciController().GetBAROffset(off, 1) + pFun->Processor.RamBaseAddress();
            (void) ::std::memcpy(reinterpret_cast<void*>(address), pv, cb);

            return VINF_SUCCESS;
        }
    }

    pFun->Processor.PciMemWriteSet(off, static_cast<u16>(cb), reinterpret_cast<const u32*>(pv));

    const DWORD waitResult = WaitForSingleObject(pFun->ProcessorSyncEvent, 250);

    if(waitResult == WAIT_TIMEOUT)
    {
        ConLogLn("Write wait timed out: 0x{XP0} 0x{XP0}", off, cb);
    }
    else if(waitResult == WAIT_FAILED)
    {
        ConLogLn("Write wait failed: {}", GetLastError());
    }

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) softGpuMMIOMapUnmap(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev, uint32_t iRegion, RTGCPHYS GCPhysAddress, RTGCPHYS cb, PCIADDRESSSPACE enmType) noexcept
{
    (void) pPciDev;

    const SoftGpuDevice* device = PDMDEVINS_2_DATA(pDevIns, SoftGpuDevice*);
    const SoftGpuDeviceFunction& pciFunction = device->pciFunction;

    if(GCPhysAddress != NIL_RTGCPHYS)
    {
        ConLogLn(u8"SoftGpu/[{}]: Mapping MMIO Region {}: Address = 0x{XP0}, Size = 0x{XP0}, BAR Flags: 0x{XP0}", pciFunction.FunctionId, iRegion, GCPhysAddress, cb, static_cast<u8>(enmType));
    }
    else
    {
        ConLogLn(u8"SoftGpu/[{}]: Unmapping MMIO Region {}: Address = 0x{XP0}, Size = 0x{XP0}, BAR Flags: 0x{XP0}", pciFunction.FunctionId, iRegion, GCPhysAddress, cb, static_cast<u8>(enmType));
    }

    return VINF_SUCCESS;
}

/**
 * Fills the framebuffer with a debug gradient.
 *
 *   This makes it easier to identify problems with framebuffer size,
 * and whether a portion of the screen has been written to.
 *
 * @param window The window we will be filling, used for sizing information.
 * @param framebuffer The framebuffer we will be filling.
 */
static void FillFramebufferGradient(const Ref<::tau::vd::Window>& window, u8* const framebuffer) noexcept
{
    for(uSys y = 0; y < window->FramebufferHeight(); ++y)
    {
        for(uSys x = 0; x < window->FramebufferWidth(); ++x)
        {
            const uSys index = (y * window->FramebufferWidth() + x) * 4;

            framebuffer[index + 0] = static_cast<u8>((x * 256) / window->FramebufferWidth());
            framebuffer[index + 1] = static_cast<u8>((y * 256) / window->FramebufferHeight());
            framebuffer[index + 2] = 0xFF - static_cast<u8>((x * y * 256) / static_cast<uSys>(window->FramebufferWidth() * window->FramebufferHeight()));
            framebuffer[index + 3] = 0xFF;
        }
    }
}

/**
 * @brief This function is used to initialize and manage the Vulkan environment for a specific GPU device function.
 *
 * It creates a window and a Vulkan manager for the given device function. It also sets up the Vulkan command pools,
 * transitions the swapchain, and fills the framebuffer with a gradient. Additionally, it sets up a framebuffer renderer
 * and a resize callback for the window. The function then enters a loop where it waits for the frame, records the command buffer,
 * submits the command buffer, and presents the frame until the window should close.
 *
 * @param pciFunction Pointer to the SoftGpuDeviceFunction structure which contains information about the GPU device function.
 */
void VulkanThreadFunc(SoftGpuDeviceFunction* pciFunction) noexcept
{
    ::new(&pciFunction->Window) Ref<tau::vd::Window>(tau::vd::Window::CreateWindow());

    EdidBlock& edid = pciFunction->Processor.GetDisplayManager().GetDisplayEdid(0);

    edid.Header[0] = 0x00;
    edid.Header[1] = 0xFF;
    edid.Header[2] = 0xFF;
    edid.Header[3] = 0xFF;
    edid.Header[4] = 0xFF;
    edid.Header[5] = 0xFF;
    edid.Header[6] = 0xFF;
    edid.Header[7] = 0x00;
    edid.ManufactureName = 0;
    edid.ProductCode = 0;
    edid.SerialNumber = 0;
    edid.WeekOfManufacture = 0;
    edid.YearOfManufacture = 24;
    edid.EdidVersion = 0;
    edid.EdidRevision = 0;
    edid.VideoInputDefinition = 0;
    edid.MaxHorizontalImageSize = 0;
    edid.MaxVerticalImageSize = 0;
    edid.DisplayTransferCharacteristic = 0;
    edid.FeatureSupport = 0;
    edid.RedGreenLowBits = 0;
    edid.BlueWhiteLowBits = 0;
    edid.RedX = 0;
    edid.RedY = 0;
    edid.GreenX = 0;
    edid.GreenY = 0;
    edid.BlueX = 0;
    edid.BlueY = 0;
    edid.WhiteX = 0;
    edid.WhiteY = 0;
    edid.EstablishedTimings[0] = 0b11111111;
    edid.EstablishedTimings[1] = 0b11111111;
    edid.EstablishedTimings[2] = 0b00000001;
    edid.StandardTimingIdentification[0x0] = 0;
    edid.StandardTimingIdentification[0x1] = 0;
    edid.StandardTimingIdentification[0x2] = 0;
    edid.StandardTimingIdentification[0x3] = 0;
    edid.StandardTimingIdentification[0x4] = 0;
    edid.StandardTimingIdentification[0x5] = 0;
    edid.StandardTimingIdentification[0x6] = 0;
    edid.StandardTimingIdentification[0x7] = 0;
    edid.StandardTimingIdentification[0x8] = 0;
    edid.StandardTimingIdentification[0x9] = 0;
    edid.StandardTimingIdentification[0xA] = 0;
    edid.StandardTimingIdentification[0xB] = 0;
    edid.StandardTimingIdentification[0xC] = 0;
    edid.StandardTimingIdentification[0xD] = 0;
    edid.StandardTimingIdentification[0xE] = 0;
    edid.StandardTimingIdentification[0xF] = 0;
    (void) ::std::memset(edid.DetailedTimingDescriptions, 0, sizeof(edid.DetailedTimingDescriptions));
    edid.ExtensionFlag = 0;

    {
        const u8* const rawEdid = reinterpret_cast<const u8*>(&edid);

        u8 checksum = 0;
            
        for(uSys i = 0; i < sizeof(edid) - 1; ++i)
        {
            checksum += rawEdid[i];
        }

        edid.Checksum = 0 - checksum;
    }

    ::std::atomic_bool displayActive = true;
    u32 refreshRateNumerator = 60;
    u32 refreshRateDenominator = 1;
    ::std::atomic_uint32_t refreshRateMeanMs = 1000 / (refreshRateNumerator / refreshRateDenominator);
    ::new(&pciFunction->FramebufferRenderer) Ref<tau::vd::FramebufferRenderer>(nullptr);

    pciFunction->Processor.GetDisplayManager().UpdateCallback() = [pciFunction, &displayActive, &refreshRateNumerator, &refreshRateDenominator, &refreshRateMeanMs](const u32 displayIndex, const DisplayData& displayData)
    {
        u16 rrDenominator = displayData.RefreshRateDenominator;
        if(rrDenominator == 0)
        {
            rrDenominator = 1;
        }

        ConLogLn("Updating display {} size: {}x{} Enabled: {}, Refresh Rate: {}, VSync: {}, Framebuffer: 0x{XP0}", displayIndex, displayData.Width, displayData.Height, displayData.Enable, static_cast<float>(displayData.RefreshRateNumerator) / static_cast<float>(rrDenominator), displayData.VSyncEnable, displayData.Framebuffer);
        
        if(displayIndex != 0)
        {
            return;
        }
    
        displayActive = displayData.Enable;
        
        if(pciFunction->Window->FramebufferWidth() != displayData.Width || pciFunction->Window->FramebufferHeight() != displayData.Height)
        {
            pciFunction->Window->SetSize(displayData.Width, displayData.Height);
        }

        if(refreshRateNumerator != displayData.RefreshRateNumerator || refreshRateDenominator != rrDenominator)
        {
            if(displayData.RefreshRateNumerator != 0 && displayData.RefreshRateDenominator != 0)
            {
                refreshRateNumerator = displayData.RefreshRateNumerator;
                refreshRateDenominator = rrDenominator;

                refreshRateMeanMs = 1000 / (refreshRateNumerator / refreshRateDenominator);
            }
        }

        if(pciFunction->FramebufferRenderer)
        {
            pciFunction->FramebufferRenderer->FramebufferOffset() = displayData.Framebuffer;
        }
    };

    // pciFunction->Processor.GetDisplayManager().GetDisplay(0).Width = pciFunction->Window->FramebufferWidth();
    // pciFunction->Processor.GetDisplayManager().GetDisplay(0).Height = pciFunction->Window->FramebufferHeight();
    // pciFunction->Processor.GetDisplayManager().GetDisplay(0).BitsPerPixel = 24;

    ::new(&pciFunction->VulkanManager) Ref<tau::vd::VulkanManager>(tau::vd::VulkanManager::CreateVulkanManager(pciFunction->Window));
    ConPrinter::PrintLn("Created vulkan manager.");
    ::new(&pciFunction->VulkanCommandPools) Ref<tau::vd::VulkanCommandPools>(::tau::vd::VulkanCommandPools::CreateCommandPools(
        pciFunction->VulkanManager->Device(),
        1,
        static_cast<u32>(pciFunction->VulkanManager->SwapchainImages().Count()),
        pciFunction->VulkanManager->Device()->GraphicsQueueFamilyIndex()
    ));

    if(!pciFunction->VulkanCommandPools)
    {
        return;
    }

    pciFunction->VulkanManager->TransitionSwapchain(pciFunction->VulkanCommandPools);

    FillFramebufferGradient(pciFunction->Window, reinterpret_cast<u8*>(pciFunction->Framebuffer));

    // ::new(&pciFunction->FramebufferRenderer) Ref<tau::vd::FramebufferRenderer>(tau::vd::FramebufferRenderer::CreateFramebufferRenderer(
    //     pciFunction->Window,
    //     pciFunction->VulkanManager->Device(),
    //     pciFunction->Framebuffer,
    //     0,
    //     pciFunction->VulkanCommandPools,
    //     pciFunction->VulkanManager->SwapchainImages(),
    //     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    // ));

    pciFunction->FramebufferRenderer = tau::vd::FramebufferRenderer::CreateFramebufferRenderer(
        pciFunction->Window,
        pciFunction->VulkanManager->Device(),
        pciFunction->Framebuffer,
        0,
        pciFunction->VulkanCommandPools,
        pciFunction->VulkanManager->SwapchainImages(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    );

    pciFunction->Window->ResizeCallback() = [pciFunction](const u32 width, const u32 height)
    {
        ConLogLn("Updating display size: {}x{}", width, height);

        if(pciFunction->Window->ShouldClose())
        {
            return;
        }

        pciFunction->VulkanManager->Device()->VkQueueWaitIdle(pciFunction->VulkanManager->Device()->GraphicsQueue());
        // FillFramebufferGradient(pciFunction->Window, reinterpret_cast<u8*>(pciFunction->Framebuffer));
        pciFunction->VulkanManager->RebuildSwapchain();
        pciFunction->VulkanManager->TransitionSwapchain(pciFunction->VulkanCommandPools);
        pciFunction->FramebufferRenderer->RebuildBuffers(pciFunction->VulkanManager->SwapchainImages(), pciFunction->Framebuffer, pciFunction->FramebufferRenderer->FramebufferOffset());
    };

    __try
    {
        using namespace std::chrono;

        while(!pciFunction->Window->ShouldClose())
        {
            const milliseconds frameBegin = duration_cast<milliseconds>(system_clock::now().time_since_epoch());

            pciFunction->Window->PollMessages();

            const u32 frameIndex = pciFunction->VulkanManager->WaitForFrame();
            VkCommandBuffer commandBuffer = pciFunction->FramebufferRenderer->Record(frameIndex, displayActive);
            pciFunction->VulkanManager->SubmitCommandBuffers(1, &commandBuffer);
            pciFunction->VulkanManager->Present(frameIndex);

            pciFunction->Processor.GetDisplayManager().SetDisplayVSyncEvent(0);

            const milliseconds frameTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch()) - frameBegin;

            const milliseconds waitTime = milliseconds(refreshRateMeanMs.load()) - frameTime;
            ::std::this_thread::sleep_for(waitTime);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        ConLogLn(u8"Encountered exception in Vulkan render loop.");
    }

    ConLogLn(u8"Vulkan Thread Exiting");
}

void ProcessorThreadFunc(SoftGpuDeviceFunction* pciFunction) noexcept
{
    // uSys i = 0;

    while(!pciFunction->ProcessorShouldExit)
    {
        // ConPrinter::PrintLn(u8"Clock {}", i++);

        pciFunction->Processor.Clock();
        ::std::this_thread::yield();
    }

    ConLogLn(u8"Processor Thread Exiting");
}

static DECLCALLBACK(int) softGpuConstruct(PPDMDEVINS deviceInstance, int instance, PCFGMNODE cfg) noexcept
{
    /*
     * Check that the device instance and device helper structures are compatible.
     * THIS IS ALWAYS THE FIRST STATEMENT IN A CONSTRUCTOR!
     */
    PDMDEV_CHECK_VERSIONS_RETURN(deviceInstance); /* This must come first. */
    Assert(instance == 0); RT_NOREF(instance);

    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: u32Version={XP}, iInstance={}", deviceInstance->u32Version, deviceInstance->iInstance);
    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Function Address: {}", reinterpret_cast<void*>(softGpuConstruct));

    /*
     * Initialize the instance data so that the destructor won't mess up.
     */
    SoftGpuDevice* device = PDMDEVINS_2_DATA(deviceInstance, SoftGpuDevice*);

    /*
     * Validate and read the configuration.
     */
    PDMDEV_VALIDATE_CONFIG_RETURN(deviceInstance, "ConfigBAR0MB|FrameBufferBAR1GB", "");

    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Validated config.");

    const PCPDMDEVHLPR3 pdmDeviceApi = deviceInstance->pHlpR3;

    AssertCompile(RT_ELEMENTS(deviceInstance->apPciDevs) >= 1);

    uint16_t configBAR0MB;
    int rc = pdmDeviceApi->pfnCFGMQueryU16Def(cfg, "ConfigBAR0MB", &configBAR0MB, 16);  /* Default to 16 MiB. */

    if(RT_FAILURE(rc))
    {
        return PDMDEV_SET_ERROR(deviceInstance, rc, N_("Configuration error: Failed to query integer value \"ConfigBAR0MB\""));
    }

    if(configBAR0MB < 16 && configBAR0MB > 0)
    {
        return PDMDEV_SET_ERROR(deviceInstance, rc, N_("Configuration error: Invalid \"ConfigBAR0MB\" value (must be at least 16)"));
    }

    if(configBAR0MB > 512)
    {
        return PDMDEV_SET_ERROR(deviceInstance, rc, N_("Configuration error: Invalid \"ConfigBAR0MB\" value (must be 512 or less)"));
    }

    RTGCPHYS firstBAR = 16ull * _1M;

    if(configBAR0MB)
    {
        firstBAR = static_cast<RTGCPHYS>(configBAR0MB) * _1M;
    }

    uint16_t frameBufferBAR1MiB;
    rc = pdmDeviceApi->pfnCFGMQueryU16Def(cfg, "FrameBufferBAR1MiB", &frameBufferBAR1MiB, 1024);  /* Default to 1 GiB. */

    if(RT_FAILURE(rc))
    {
        return PDMDEV_SET_ERROR(deviceInstance, rc, N_("Configuration error: Failed to query integer value \"FrameBufferBAR1MiB\""));
    }

    if(frameBufferBAR1MiB < 256 && frameBufferBAR1MiB > 0)
    {
        return PDMDEV_SET_ERROR(deviceInstance, rc, N_("Configuration error: Invalid \"FrameBufferBAR1MiB\" value (must be at least 256 MiB)"));
    }

    if(frameBufferBAR1MiB > 16 * 1024)
    {
        return PDMDEV_SET_ERROR(deviceInstance, rc, N_("Configuration error: Invalid \"FrameBufferBAR1MiB\" value (must be 16 GiB or less)"));
    }

    RTGCPHYS secondBAR = frameBufferBAR1MiB * _1M;
    // if(frameBufferBAR1GB)
    // {
    //     secondBAR = static_cast<RTGCPHYS>(frameBufferBAR1GB) * _1G64;
    // }

    ConLogLn(u8"VBoxSoftGpuEmulator::softGpuConstruct: BAR0 Size: 0x{XP0}", firstBAR);
    ConLogLn(u8"VBoxSoftGpuEmulator::softGpuConstruct: BAR1 Size: 0x{XP0}", secondBAR);

    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Read config.");

    /*
     * PCI device setup.
     */
    PPDMPCIDEV pciDevice = deviceInstance->apPciDevs[0];
    SoftGpuDeviceFunction* pciFunction = &device->pciFunction;
    RTStrPrintf(pciFunction->FunctionName, sizeof(pciFunction->FunctionName), "soft_gpu%u", 0);
    pciFunction->FunctionId = 0;
    ::new(&pciFunction->Processor) Processor;

    pciFunction->Processor.GetPciControlRegisters().RegisterDebugCallbacks(nullptr, PciControlWriteCallback);
    
    pciFunction->Framebuffer = VirtualAlloc(nullptr, static_cast<uSys>(secondBAR), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    pciFunction->Processor.TestSetRamBaseAddress(reinterpret_cast<uPtr>(pciFunction->Framebuffer), secondBAR);

    ::new(&pciFunction->ProcessorShouldExit) ::std::atomic_bool(false);
    pciFunction->ProcessorSyncEvent = CreateEventA(nullptr, FALSE, FALSE, "SoftGpuSync");
    pciFunction->Processor.GetPciController().SetSimulationSyncEvent(pciFunction->ProcessorSyncEvent);

    pciFunction->Processor.GetPciController().InterruptCallback() = [deviceInstance](const u16 messageData)
    {
        // We don't use this, VirtualBox manages it for us.
        (void) messageData;
        // Level is set to 1 (HIGH), IRQ is the offset into our MSI structure, not the actual IRQ.
        //   We only have 1 interrupt type, so our IRQ is 0. VirtualBox (specifically the ICH9 bridge, or regular PCI bridge)
        // will handle the sending the exact message data.
        PDMDevHlpPCISetIrqEx(deviceInstance, deviceInstance->apPciDevs[0], 0, 1);
    };
    pciFunction->Processor.GetPciController().BusMasterReadCallback() = [deviceInstance](const u64 address, const u16 size, void* const buffer)
    {
        PDMDevHlpPhysRead(deviceInstance, address, buffer, size);
    };
    pciFunction->Processor.GetPciController().BusMasterWriteCallback() = [deviceInstance](const u64 address, const u16 size, const void* const buffer)
    {
        PDMDevHlpPhysWrite(deviceInstance, address, buffer, size);
    };

    ::new(&pciFunction->ProcessorThread) ::std::thread(ProcessorThreadFunc, pciFunction);
    (void) SetThreadDescription(pciFunction->ProcessorThread.native_handle(), L"SoftGpuProcessorThread");

    ::new(&pciFunction->VulkanThread) ::std::thread(VulkanThreadFunc, pciFunction);
    (void) SetThreadDescription(pciFunction->VulkanThread.native_handle(), L"VulkanRenderThread");

    PDMPCIDEV_ASSERT_VALID(deviceInstance, pciDevice);
    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Asserted pciDevice as valid.");

    if constexpr(false)
    {
        ConLogLn("Setting PCI config info through VBox functions.");
        PDMPciDevSetVendorId(pciDevice, 0xFFFD);
        PDMPciDevSetDeviceId(pciDevice, 0x0001);
        PDMPciDevSetClassBase(pciDevice, 0x03);  /* display controller device */
        PDMPciDevSetClassSub(pciDevice, 0x00);  /* VGA compatible controller device */
        PDMPciDevSetHeaderType(pciDevice, 0x00);  /* normal, single function device */
    }
    else
    {
        // static_assert(sizeof(PciController) == sizeof(pciDevice->abConfig), "SoftGpu PCI config space does not match size of the VBox backing storage.");

        ConLogLn("Setting PCI config info by copying the PCIController.");

        (void) ::std::memcpy(pciDevice->abConfig, &pciFunction->Processor.GetPciController(), sizeof(pciDevice->abConfig));

        ConLogLn("VBox PCI Vendor ID: {X}", PDMPciDevGetVendorId(pciDevice));
        ConLogLn("SoftGpu PCI Vendor ID: {X}", pciFunction->Processor.PciConfigRead(0, 2));
    }

    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Set PCI info.");

    // pciFunction->IBase.pfnQueryInterface = nullptr;
    //
    // pciFunction->IDisplayPort.pfnUpdateDisplay = nullptr;
    // pciFunction->IDisplayPort.pfnUpdateDisplayAll = nullptr;
    // pciFunction->IDisplayPort.pfnQueryVideoMode = nullptr;
    // pciFunction->IDisplayPort.pfnSetRefreshRate = nullptr;
    // pciFunction->IDisplayPort.pfnTakeScreenshot = nullptr;
    // pciFunction->IDisplayPort.pfnFreeScreenshot = nullptr;
    // pciFunction->IDisplayPort.pfnDisplayBlt = nullptr;
    // pciFunction->IDisplayPort.pfnUpdateDisplayRect = nullptr;
    // pciFunction->IDisplayPort.pfnCopyRect = nullptr;
    // pciFunction->IDisplayPort.pfnSetRenderVRAM = nullptr;
    //
    // pciFunction->IDisplayPort.pfnSetViewport = nullptr;
    // pciFunction->IDisplayPort.pfnReportMonitorPositions = nullptr;
    //
    // pciFunction->IDisplayPort.pfnSendModeHint = nullptr;
    // pciFunction->IDisplayPort.pfnReportHostCursorCapabilities = nullptr;
    // pciFunction->IDisplayPort.pfnReportHostCursorPosition = nullptr;

    rc = PDMDevHlpPCIRegisterEx(deviceInstance, pciDevice, 0 /*fFlags*/, PDMPCIDEVREG_DEV_NO_FIRST_UNUSED, 0, device->pciFunction.FunctionName);
    AssertLogRelRCReturn(rc, rc);

    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Registered PCI device.");

    /* MSI Capability Header register. */
    PDMMSIREG MsiReg;
    RT_ZERO(MsiReg);
    MsiReg.cMsiVectors = 1;
    MsiReg.iMsiCapOffset = PciConfigOffsets::MessageSignalledInterruptsCapabilityOffsetBegin;
    MsiReg.iMsiNextOffset = 0;
    MsiReg.fMsi64bit = true;
    MsiReg.fMsiNoMasking = true;

    rc = PDMDevHlpPCIRegisterMsiEx(deviceInstance, pciDevice, &MsiReg);
    AssertLogRelRCReturn(rc, rc);

    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Registering config interceptor.");
    // Register a interceptor for PCI config reads and writes.
    rc = PDMDevHlpPCIInterceptConfigAccesses(deviceInstance, pciDevice, SoftGpuConfigRead, SoftGpuConfigWrite);
    AssertLogRelRCReturn(rc, rc);
    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Registered config interceptor.");

    /* BAR0 region. */
    RTStrPrintf(pciFunction->MmioBar0Name, sizeof(pciFunction->MmioBar0Name), "SG-F%d-BAR0", 0);

    rc = PDMDevHlpMmioCreate(deviceInstance, firstBAR, pciDevice, 0 /*iPciRegion*/,
        softGpuMMIOWrite, softGpuMMIORead, pciFunction,
        IOMMMIO_FLAGS_READ_DWORD | IOMMMIO_FLAGS_WRITE_DWORD_ZEROED | IOMMMIO_FLAGS_ABS, pciFunction->MmioBar0Name, &pciFunction->hMmioBar0);
    AssertLogRelRCReturn(rc, rc);
    
    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Created BAR0 region.");
    
    rc = PDMDevHlpPCIIORegionRegisterMmioEx(deviceInstance, pciDevice, 0, firstBAR,
        static_cast<PCIADDRESSSPACE>(PCI_ADDRESS_SPACE_MEM | PCI_ADDRESS_SPACE_BAR32),
        pciFunction->hMmioBar0, softGpuMMIOMapUnmap);
    AssertLogRelRCReturn(rc, rc);
    
    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Registered MMIO function for BAR0 region.");

    /* BAR1 region. */
    RTStrPrintf(pciFunction->MmioBar1Name, sizeof(pciFunction->MmioBar1Name), "SG-F%d-BAR1", 0);

    rc = PDMDevHlpMmioCreate(deviceInstance, secondBAR, pciDevice, 1 << 16 /*iPciRegion*/,
        softGpuMMIOWrite, softGpuMMIORead, pciFunction,
        IOMMMIO_FLAGS_READ_DWORD | IOMMMIO_FLAGS_WRITE_DWORD_ZEROED | IOMMMIO_FLAGS_ABS, pciFunction->MmioBar1Name, &pciFunction->hMmioBar1);
    AssertLogRelRCReturn(rc, rc);
    
    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Created BAR1 region.");

    rc = PDMDevHlpPCIIORegionRegisterMmioEx(deviceInstance, pciDevice, 1, secondBAR,
        static_cast<PCIADDRESSSPACE>(PCI_ADDRESS_SPACE_MEM | PCI_ADDRESS_SPACE_BAR64 | PCI_ADDRESS_SPACE_MEM_PREFETCH),
        pciFunction->hMmioBar1, softGpuMMIOMapUnmap);
    AssertLogRelRCReturn(rc, rc);
    
    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Registered MMIO function BAR1 region.");

    /* Expansion ROM region. */
    RTStrPrintf(pciFunction->MmioExpansionRomName, sizeof(pciFunction->MmioExpansionRomName), "SG-F%d-ROM", 0);

    rc = PDMDevHlpMmioCreate(deviceInstance, 32ull * 1024, pciDevice, 6 << 16 /*iPciRegion*/,
        softGpuMMIOWrite, softGpuMMIORead, pciFunction,
        IOMMMIO_FLAGS_WRITE_PASSTHRU | IOMMMIO_FLAGS_READ_PASSTHRU | IOMMMIO_FLAGS_ABS, pciFunction->MmioExpansionRomName, &pciFunction->hMmioExpansionRom);
    AssertLogRelRCReturn(rc, rc);
    
    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Created Expansion ROM region.");
    
    rc = PDMDevHlpPCIIORegionRegisterMmioEx(deviceInstance, pciDevice, 6, 32ull * 1024,
        static_cast<PCIADDRESSSPACE>(PCI_ADDRESS_SPACE_MEM | PCI_ADDRESS_SPACE_MEM_PREFETCH),
        pciFunction->hMmioExpansionRom, softGpuMMIOMapUnmap);
    AssertLogRelRCReturn(rc, rc);
    
    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Registered MMIO function Expansion ROM region.");

    /*
     * Save state handling.
     */
    rc = PDMDevHlpSSMRegister(deviceInstance, SOFT_GPU_SSM_VERSION, sizeof(*device), nullptr, nullptr);
    AssertLogRelRCReturn(rc, rc);

    ConLogLn("VBoxSoftGpuEmulator::softGpuConstruct: Registered saves state handler.");
    
    // rc = PDMDevHlpDriverAttach(deviceInstance, 0, &pciFunction->IBase, &pciFunction->pDrvBase, "Display Port");

    if(RT_FAILURE(rc))
    {
        return rc;
    }

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) softGpuDestruct(PPDMDEVINS deviceInstance) noexcept
{
    ConLogLn(u8"VBoxSoftGpuEmulator::softGpuDestruct: deviceInstance->u32Version={X} deviceInstance->iInstance={}", deviceInstance->u32Version, deviceInstance->iInstance);

    /*
     * Check the versions here as well since the destructor is *always* called.
     * THIS IS ALWAYS THE FIRST STATEMENT IN A DESTRUCTOR!
     */
    PDMDEV_CHECK_VERSIONS_RETURN_QUIET(deviceInstance);

    SoftGpuDevice* device = PDMDEVINS_2_DATA(deviceInstance, SoftGpuDevice*);
    SoftGpuDeviceFunction& pciFunction = device->pciFunction;

    ConLogLn(u8"VBoxSoftGpuEmulator::softGpuDestruct: Telling processor thread to exit.");

    pciFunction.ProcessorShouldExit = true;

    ConLogLn(u8"VBoxSoftGpuEmulator::softGpuDestruct: Processor thread told to exit.");

    pciFunction.Window->Close();

    ConLogLn(u8"VBoxSoftGpuEmulator::softGpuDestruct: Window Closed.");

    pciFunction.VulkanThread.join();
    pciFunction.VulkanThread.~thread();

    ConLogLn(u8"VBoxSoftGpuEmulator::softGpuDestruct: Vulkan Thread Exited.");

    if(pciFunction.VulkanManager->Device()->VkDeviceWaitIdle)
    {
        pciFunction.VulkanManager->Device()->VkDeviceWaitIdle(pciFunction.VulkanManager->Device()->Device());
    }

    pciFunction.FramebufferRenderer.~ReferenceCountingPointer();
    pciFunction.VulkanCommandPools.~ReferenceCountingPointer();
    pciFunction.VulkanManager.~ReferenceCountingPointer();
    pciFunction.Window.~ReferenceCountingPointer();

    pciFunction.ProcessorThread.join();
    pciFunction.ProcessorThread.~thread();

    ConLogLn(u8"VBoxSoftGpuEmulator::softGpuDestruct: Processor Thread Exited.");

    pciFunction.Processor.~Processor();

    pciFunction.ProcessorShouldExit.~atomic();

    CloseHandle(pciFunction.ProcessorSyncEvent);

    (void) VirtualFree(pciFunction.Framebuffer, static_cast<uSys>(256 * 1024 * 1024), MEM_RELEASE);

    return VINF_SUCCESS;
}

/**
 * The device registration structure.
 */
static constexpr PDMDEVREG g_DeviceSoftGpu =
{
    /* .u32Version = */             PDM_DEVREG_VERSION,
    /* .uReserved0 = */             0,
    /* .szName = */                 "softgpu",
    /* .fFlags = */                 PDM_DEVREG_FLAGS_DEFAULT_BITS | PDM_DEVREG_FLAGS_NEW_STYLE,
    /* .fClass = */                 PDM_DEVREG_CLASS_GRAPHICS,
    /* .cMaxInstances = */          1,
    /* .uSharedVersion = */         1,
    /* .cbInstanceShared = */       sizeof(SoftGpuDevice),
    /* .cbInstanceCC = */           0,
    /* .cbInstanceRC = */           0,
    /* .cMaxPciDevices = */         1,
    /* .cMaxMsixVectors = */        0,
    /* .pszDescription = */         "SoftGpu Device.",
#if defined(IN_RING3)
    /* .pszRCMod = */               "",
    /* .pszR0Mod = */               "",
    /* .pfnConstruct = */           softGpuConstruct,
    /* .pfnDestruct = */            softGpuDestruct,
    /* .pfnRelocate = */            nullptr,
    /* .pfnMemSetup = */            nullptr,
    /* .pfnPowerOn = */             nullptr,
    /* .pfnReset = */               nullptr,
    /* .pfnSuspend = */             nullptr,
    /* .pfnResume = */              nullptr,
    /* .pfnAttach = */              nullptr,
    /* .pfnDetach = */              nullptr,
    /* .pfnQueryInterface = */      nullptr,
    /* .pfnInitComplete = */        nullptr,
    /* .pfnPowerOff = */            nullptr,
    /* .pfnSoftReset = */           nullptr,
    /* .pfnReserved0 = */           nullptr,
    /* .pfnReserved1 = */           nullptr,
    /* .pfnReserved2 = */           nullptr,
    /* .pfnReserved3 = */           nullptr,
    /* .pfnReserved4 = */           nullptr,
    /* .pfnReserved5 = */           nullptr,
    /* .pfnReserved6 = */           nullptr,
    /* .pfnReserved7 = */           nullptr,
#elif defined(IN_RING0)
    /* .pfnEarlyConstruct = */      NULL,
    /* .pfnConstruct = */           NULL,
    /* .pfnDestruct = */            NULL,
    /* .pfnFinalDestruct = */       NULL,
    /* .pfnRequest = */             NULL,
    /* .pfnReserved0 = */           NULL,
    /* .pfnReserved1 = */           NULL,
    /* .pfnReserved2 = */           NULL,
    /* .pfnReserved3 = */           NULL,
    /* .pfnReserved4 = */           NULL,
    /* .pfnReserved5 = */           NULL,
    /* .pfnReserved6 = */           NULL,
    /* .pfnReserved7 = */           NULL,
#elif defined(IN_RC)
    /* .pfnConstruct = */           NULL,
    /* .pfnReserved0 = */           NULL,
    /* .pfnReserved1 = */           NULL,
    /* .pfnReserved2 = */           NULL,
    /* .pfnReserved3 = */           NULL,
    /* .pfnReserved4 = */           NULL,
    /* .pfnReserved5 = */           NULL,
    /* .pfnReserved6 = */           NULL,
    /* .pfnReserved7 = */           NULL,
#else
# error "Not in IN_RING3, IN_RING0 or IN_RC!"
#endif
    /* .u32VersionEnd = */          PDM_DEVREG_VERSION
};

static bool pdmR3IsValidName(const char* pszName)
{
    char ch;
    while((ch = *pszName) != '\0'
        && (RT_C_IS_ALNUM(ch)
            || ch == '-'
            || ch == '_'))
        pszName++;
    return ch == '\0';
}

/** The maximum device instance (total) size, ring-0/raw-mode capable devices. */
#define PDM_MAX_DEVICE_INSTANCE_SIZE    _4M
/** The maximum device instance (total) size, ring-3 only devices. */
#define PDM_MAX_DEVICE_INSTANCE_SIZE_R3 _8M

[[maybe_unused]] static DECLCALLBACK(int) preRegisterTest(PPDMDEVREGCB pCallbacks, PCPDMDEVREG pReg)
{
    (void) pCallbacks;

    /*
     * Validate the registration structure.
     */
    Assert(pReg);
    AssertLogRelMsgReturn(pReg->u32Version == PDM_DEVREG_VERSION,
        ("Unknown struct version %#x!\n", pReg->u32Version),
        VERR_PDM_UNKNOWN_DEVREG_VERSION);

    AssertLogRelMsgReturn(pReg->szName[0]
        && strlen(pReg->szName) < sizeof(pReg->szName)
        && pdmR3IsValidName(pReg->szName),
        ("Invalid name '%.*s'\n", sizeof(pReg->szName), pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertLogRelMsgReturn(!(pReg->fFlags & PDM_DEVREG_FLAGS_RC)
        || (pReg->pszRCMod[0]
            && strlen(pReg->pszRCMod) < RT_SIZEOFMEMB(PDMDEVICECREATEREQ, szModName)),
        ("Invalid GC module name '%s' - (Device %s)\n", pReg->pszRCMod, pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertLogRelMsgReturn(!(pReg->fFlags & PDM_DEVREG_FLAGS_R0)
        || (pReg->pszR0Mod[0]
            && strlen(pReg->pszR0Mod) < RT_SIZEOFMEMB(PDMDEVICECREATEREQ, szModName)),
        ("Invalid R0 module name '%s' - (Device %s)\n", pReg->pszR0Mod, pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertLogRelMsgReturn((pReg->fFlags & PDM_DEVREG_FLAGS_HOST_BITS_MASK) == PDM_DEVREG_FLAGS_HOST_BITS_DEFAULT,
        ("Invalid host bits flags! fFlags=%#x (Device %s)\n", pReg->fFlags, pReg->szName),
        VERR_PDM_INVALID_DEVICE_HOST_BITS);
    AssertLogRelMsgReturn((pReg->fFlags & PDM_DEVREG_FLAGS_GUEST_BITS_MASK),
        ("Invalid guest bits flags! fFlags=%#x (Device %s)\n", pReg->fFlags, pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertLogRelMsgReturn(pReg->fClass,
        ("No class! (Device %s)\n", pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertLogRelMsgReturn(pReg->cMaxInstances > 0,
        ("Max instances %u! (Device %s)\n", pReg->cMaxInstances, pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);
    uint32_t const cbMaxInstance = pReg->fFlags & (PDM_DEVREG_FLAGS_RC | PDM_DEVREG_FLAGS_R0)
        ? PDM_MAX_DEVICE_INSTANCE_SIZE : PDM_MAX_DEVICE_INSTANCE_SIZE_R3;
    AssertLogRelMsgReturn(pReg->cbInstanceShared <= cbMaxInstance,
        ("Instance size %u bytes! (Max %u; Device %s)\n", pReg->cbInstanceShared, cbMaxInstance, pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertLogRelMsgReturn(pReg->cbInstanceCC <= cbMaxInstance,
        ("Instance size %d bytes! (Max %u; Device %s)\n", pReg->cbInstanceCC, cbMaxInstance, pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertLogRelMsgReturn(pReg->pfnConstruct,
        ("No constructor! (Device %s)\n", pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertLogRelMsgReturn((pReg->fFlags & PDM_DEVREG_FLAGS_GUEST_BITS_MASK) == PDM_DEVREG_FLAGS_GUEST_BITS_DEFAULT,
        ("PDM: Rejected device '%s' because it didn't match the guest bits.\n", pReg->szName),
        VERR_PDM_INVALID_DEVICE_GUEST_BITS);
    AssertLogRelMsg(pReg->u32VersionEnd == PDM_DEVREG_VERSION,
        ("u32VersionEnd=%#x, expected %#x. (szName=%s)\n",
            pReg->u32VersionEnd, PDM_DEVREG_VERSION, pReg->szName));
    AssertLogRelMsgReturn(pReg->cMaxPciDevices <= 8, ("%#x (szName=%s)\n", pReg->cMaxPciDevices, pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertLogRelMsgReturn(pReg->cMaxMsixVectors <= VBOX_MSIX_MAX_ENTRIES,
        ("%#x (szName=%s)\n", pReg->cMaxMsixVectors, pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);
    AssertLogRelMsgReturn(pReg->fFlags & PDM_DEVREG_FLAGS_NEW_STYLE /* the flag is required now */,
        ("PDM_DEVREG_FLAGS_NEW_STYLE not set for szName=%s!\n", pReg->szName),
        VERR_PDM_INVALID_DEVICE_REGISTRATION);

    return 0;
}

extern "C" DECLEXPORT(int) VBoxDevicesRegister(PPDMDEVREGCB pCallbacks, const uint32_t u32Version)
{
    LogRelFlow(("VBoxSoftGpuEmulator::VBoxDevicesRegister: u32Version=%#x pCallbacks->u32Version=%#x\n", u32Version, pCallbacks->u32Version));

    Console::Create();
    Console::Init();
    
    ConPrinter::PrintLn(u8"VBoxSoftGpuEmulator::VBoxDevicesRegister: u32Version=0x{X} pCallbacks->u32Version=0x{X}", u32Version, pCallbacks->u32Version);

    AssertLogRelMsgReturn(u32Version >= VBOX_VERSION, ("VirtualBox version %#x, expected %#x or higher\n", u32Version, VBOX_VERSION), VERR_VERSION_MISMATCH);
    AssertLogRelMsgReturn(pCallbacks->u32Version == PDM_DEVREG_CB_VERSION, ("Callbacks version %#x, expected %#x or higher\n", pCallbacks->u32Version, PDM_DEVREG_CB_VERSION), VERR_VERSION_MISMATCH);

    // preRegisterTest(pCallbacks, &g_DeviceSoftGpu);

    const int registerRC = pCallbacks->pfnRegister(pCallbacks, &g_DeviceSoftGpu);
    
    ConLogLn(u8"VBoxSoftGpuEmulator::VBoxDevicesRegister: pfnRegister: [{}] {}, {}", registerRC, ConLogRCDefine<c8>(registerRC), ConLogRCMsgFull<c8>(registerRC));

    return registerRC;
}

