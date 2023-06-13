#include "vd/VulkanManager.hpp"
#include "vd/VulkanDebug.hpp"
#include "vd/MemoryRecovery.hpp"
#include "vd/VulkanFunctions.hpp"
#include "vd/VulkanInstanceLayers.hpp"
#include "vd/VulkanInstanceExtensions.hpp"
#include "vd/VulkanDeviceExtensions.hpp"

#include <DynArray.hpp>
#include <String.hpp>
#include <ConPrinter.hpp>
#include <TUMaths.hpp>

#include "vd/Window.hpp"

tau::vd::VulkanManager::VulkanManager(
    VkInstance vulkan, 
    const u32 vulkanVersion, 
    VkDebugUtilsMessengerEXT debugMessenger,
    const ReferenceCountingPointer<Window>& window,
    VkSurfaceKHR surface,
    VkPhysicalDevice physicalDevice, 
    ReferenceCountingPointer<VulkanDevice>&& device,
    VkSwapchainKHR swapchain,
    const DynArray<VkImage>& swapchainImages,
    const DynArray<VkImageView>& swapchainImageViews,
    VkExtent2D swapchainSize,
    VkFormat swapchainImageFormat
) noexcept
    : m_Vulkan(vulkan)
    , m_VulkanVersion(vulkanVersion)
    , m_DebugMessenger(debugMessenger)
    , m_Window(window)
    , m_Surface(surface)
    , m_PhysicalDevice(physicalDevice)
    , m_Device(::std::move(device))
    , m_Swapchain(swapchain)
    , m_SwapchainImages(swapchainImages)
    , m_SwapchainImageViews(swapchainImageViews)
    , m_SwapchainSize(swapchainSize)
    , m_SwapchainImageFormat(swapchainImageFormat)
{ }

tau::vd::VulkanManager::~VulkanManager() noexcept
{
    for(VkImageView imageView : m_SwapchainImageViews)
    {
        m_Device->DestroyImageView(imageView);
    }

    m_Device->DestroySwapchainKHR(m_Swapchain);

    // Force the device to destroy before we destroy the instance.
    m_Device = nullptr;

    vkDestroySurfaceKHR(m_Vulkan, m_Surface, nullptr);

    if(InstanceFunctions::VkDestroyDebugUtilsMessengerEXT && m_DebugMessenger)
    {
        InstanceFunctions::VkDestroyDebugUtilsMessengerEXT(m_Vulkan, m_DebugMessenger, nullptr);
    }

    vkDestroyInstance(m_Vulkan, nullptr);
}

static DynArray<VkPhysicalDevice> GetPhysicalDevices(VkInstance vulkanInstance) noexcept
{
    u32 physicalDeviceCount;
    VkResult result = vkEnumeratePhysicalDevices(vulkanInstance, &physicalDeviceCount, nullptr);

    if(result == VK_ERROR_OUT_OF_HOST_MEMORY)
    {
        tau::vd::RecoverSacrificialMemory();
        ConPrinter::PrintLn("Ran out of system memory while querying number of physical devices.");
        return { };
    }

    if(result == VK_ERROR_OUT_OF_DEVICE_MEMORY)
    {
        ConPrinter::PrintLn("Ran out of device memory while querying number of physical devices.");
        return { };
    }

    if(result != VK_SUCCESS && result != VK_INCOMPLETE)
    {
        ConPrinter::PrintLn("Encountered undefined error while querying number of physical devices: {}.", result);
        return { };
    }

    do
    {
        DynArray<VkPhysicalDevice> devices(physicalDeviceCount);

        result = vkEnumeratePhysicalDevices(vulkanInstance, &physicalDeviceCount, devices);

        if(result == VK_ERROR_OUT_OF_HOST_MEMORY)
        {
            tau::vd::RecoverSacrificialMemory();
            ConPrinter::PrintLn("Ran system memory while querying physical devices.");
            return { };
        }

        if(result == VK_ERROR_OUT_OF_DEVICE_MEMORY)
        {
            ConPrinter::PrintLn("Ran device memory while querying physical devices.");
            return { };
        }

        if(result == VK_SUCCESS)
        {
            return devices;
        }
    } while(result == VK_INCOMPLETE);

    ConPrinter::PrintLn("Encountered undefined error while querying physical devices: {}.", result);
    return { };
}

static u32 GetVulkanVersion() noexcept
{
    // Load the vkEnumerateInstanceVersion function. If this is null, we have to use Vulkan 1.0
    ::tau::vd::LoadNonInstanceFunctions();

    // Default to Vulkan 1.0
    u32 vulkanVersion = VK_API_VERSION_1_0;

    // Attempt to get the latest Vulkan version.
    if(VkEnumerateInstanceVersion)
    {
        const VkResult result = vkEnumerateInstanceVersion(&vulkanVersion);

        // If the system is out of memory it's probably not worth trying to allocate anything.
        if(result == VK_ERROR_OUT_OF_HOST_MEMORY)
        {
            tau::vd::RecoverSacrificialMemory();
            return VK_API_VERSION_1_0;
        }
        // If we get some other error, make sure vulkanVersion is well defined to Vulkan 1.0
        else if(result != VK_SUCCESS)
        {
            return VK_API_VERSION_1_0;
        }
    }

#if defined(VD_FORCE_MAX_VULKAN_VERSION)
  #define MAX_VK_VERSION VD_FORCE_MAX_VULKAN_VERSION
#else
  #define MAX_VK_VERSION VK_API_VERSION_1_3
#endif

    //   If the Vulkan loader supports a version newer than what we're
    // compiling against (currently Vulkan 1.3), set the max version to
    // what we can guarantee.
    if(vulkanVersion > MAX_VK_VERSION)
    {
        return MAX_VK_VERSION;
    }

#undef MAX_VK_VERSION

    return vulkanVersion;
}

struct RankedDevice final
{
    DEFAULT_DESTRUCT(RankedDevice);
    DEFAULT_CM_PU(RankedDevice);
public:
    i32 Rank;
    VkPhysicalDevice Device;
    u32 DeviceVulkanVersion;
    iSys GraphicsQueueIndex;
    iSys PresentQueueIndex;
    DynArray<const char*> EnabledExtensions;
    VkSurfaceCapabilitiesKHR SurfaceCapabilities;
    DynArray<VkSurfaceFormatKHR> SurfaceFormats;
    DynArray<VkPresentModeKHR> PresentModes;

    RankedDevice() noexcept
        : Rank(IntMaxMin<i32>::Min)
        , Device(VK_NULL_HANDLE)
        , DeviceVulkanVersion(0)
        , GraphicsQueueIndex(-1)
        , PresentQueueIndex(-1)
        , SurfaceCapabilities{ }
    { }

    [[nodiscard]] bool HasRequiredExtensions() const noexcept
    {
        if constexpr(::std::size(tau::vd::RequiredDeviceExtensions) > 0)
        {
            if(EnabledExtensions.Count() == 0)
            {
                return false;
            }
        }

        return true;
    }

    [[nodiscard]] bool IsValid() const noexcept { return GraphicsQueueIndex >= 0 && PresentQueueIndex >= 0 && HasRequiredExtensions(); }
    [[nodiscard]] bool IsGraphicsAndPresentSame() const noexcept { return GraphicsQueueIndex == PresentQueueIndex; }
};

static i32 ComputeScoreOfDeviceProps(const VkPhysicalDeviceProperties& deviceProperties, u32* const deviceVulkanVersion) noexcept
{
    *deviceVulkanVersion = deviceProperties.apiVersion;

    i32 propsRank = 0;

    if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        propsRank += 1024;
    }
    else if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    {
        propsRank += 512;
    }

    if(deviceProperties.limits.maxFramebufferHeight >= 2160 && deviceProperties.limits.maxFramebufferWidth >= 3840)
    {
        propsRank += 768;
    }

    return propsRank;
}

template<typename DeviceProperties2, VkStructureType Type, typename Func>
[[nodiscard]] static DeviceProperties2 GetPhysicalDeviceProperties2(VkPhysicalDevice physicalDevice, Func func) noexcept
{
    DeviceProperties2 deviceProperties { };
    deviceProperties.sType = Type;
    deviceProperties.pNext = nullptr;

    func(physicalDevice, &deviceProperties);

    return deviceProperties;
}

template<typename QueueFamilyProperties2, VkStructureType Type, typename Func>
[[nodiscard]] static DynArray<QueueFamilyProperties2> GetPhysicalDeviceQueueFamilyProperties2(VkPhysicalDevice physicalDevice, Func func) noexcept
{
    u32 physicalDeviceCount;
    func(physicalDevice, &physicalDeviceCount, nullptr);
    
    DynArray<QueueFamilyProperties2> queueFamilyProperties(physicalDeviceCount);
    queueFamilyProperties.Zero();

    if constexpr(!::std::is_same_v<QueueFamilyProperties2, VkQueueFamilyProperties>)
    {
        for(QueueFamilyProperties2& properties : queueFamilyProperties)
        {
            properties.sType = Type;
            properties.pNext = nullptr;
        }
    }

    func(physicalDevice, &physicalDeviceCount, queueFamilyProperties);
    
    return queueFamilyProperties;
}

template<typename QueueFamilyProperties2>
static void FindQueues(const DynArray<QueueFamilyProperties2>& queueFamilyProperties, RankedDevice* const device, VkSurfaceKHR surface) noexcept
{
    for(iSys i = 0; i < static_cast<iSys>(queueFamilyProperties.Length()); ++i)
    {
        const VkQueueFamilyProperties* properties;
        if constexpr(::std::is_same_v<QueueFamilyProperties2, VkQueueFamilyProperties>)
        {
            properties = &queueFamilyProperties[i];
        }
        else
        {
            properties = &queueFamilyProperties[i].queueFamilyProperties;
        }
        
        if(properties->queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            device->GraphicsQueueIndex = i;
        }

        {
            VkBool32 presentSupport = VK_FALSE;
            const VkResult result = ::tau::vd::InstanceFunctions::VkGetPhysicalDeviceSurfaceSupportKHR(device->Device, static_cast<u32>(i), surface, &presentSupport);

            if(result != VK_SUCCESS)
            {
                switch(result)
                {
                    case VK_ERROR_OUT_OF_HOST_MEMORY:
                        tau::vd::RecoverSacrificialMemory();
                        ConPrinter::PrintLn("Ran out of system memory while checking present support.");
                        return;
                    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                        ConPrinter::PrintLn("Ran out of device memory while checking present support.");
                        return;
                    case VK_ERROR_SURFACE_LOST_KHR:
                        ConPrinter::PrintLn("Lost surface while checking present support.");
                        return;
                    default:
                        ConPrinter::PrintLn("Encountered unknown error while checking present support: {}.", result);
                        return;
                }
            }

            if(presentSupport)
            {
                device->PresentQueueIndex = i;
            }
        }

        if(device->GraphicsQueueIndex >= 0 && device->PresentQueueIndex >= 0)
        {
            break;
        }
    }
}

template<typename SurfaceFormat2, VkStructureType Type, typename Func>
[[nodiscard]] static DynArray<SurfaceFormat2> GetPhysicalDeviceSurfaceFormats2(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, Func func) noexcept
{
    VkResult result;
    u32 formatCount;

    VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo { };
    surfaceInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
    surfaceInfo.pNext = nullptr;
    surfaceInfo.surface = surface;

    do
    {
        if constexpr(::std::is_same_v<SurfaceFormat2, VkSurfaceFormat2KHR>)
        {
            result = func(physicalDevice, &surfaceInfo, &formatCount, nullptr);
        }
        else
        {
            result = func(physicalDevice, surface, &formatCount, nullptr);
        }

        if(result != VK_SUCCESS && result != VK_INCOMPLETE)
        {
            switch(result)
            {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    tau::vd::RecoverSacrificialMemory();
                    ConPrinter::PrintLn("Ran out of system memory while querying surface formats.");
                    return DynArray<SurfaceFormat2>();
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    ConPrinter::PrintLn("Ran out of device memory while querying surface formats.");
                    return DynArray<SurfaceFormat2>();
                case VK_ERROR_SURFACE_LOST_KHR:
                    ConPrinter::PrintLn("Lost surface while querying surface formats.");
                    return DynArray<SurfaceFormat2>();
                default:
                    ConPrinter::PrintLn("Encountered unknown error while querying surface formats: {}.", result);
                    return DynArray<SurfaceFormat2>();
            }
        }

        DynArray<SurfaceFormat2> formats(formatCount);
        formats.Zero();

        if constexpr(!::std::is_same_v<SurfaceFormat2, VkSurfaceFormatKHR>)
        {
            SurfaceFormat2 baseProperties { };
            baseProperties.sType = Type;
            baseProperties.pNext = nullptr;

            formats.MemCpyAll(baseProperties);
        }

        if constexpr(::std::is_same_v<SurfaceFormat2, VkSurfaceFormat2KHR>)
        {
            result = func(physicalDevice, &surfaceInfo, &formatCount, formats);
        }
        else
        {
            result = func(physicalDevice, surface, &formatCount, formats);
        }

        if(result == VK_SUCCESS)
        {
            return formats;
        }
        else if(result != VK_INCOMPLETE)
        {
            switch(result)
            {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    tau::vd::RecoverSacrificialMemory();
                    ConPrinter::PrintLn("Ran out of system memory while querying surface formats.");
                    return DynArray<SurfaceFormat2>();
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    ConPrinter::PrintLn("Ran out of device memory while querying surface formats.");
                    return DynArray<SurfaceFormat2>();
                case VK_ERROR_SURFACE_LOST_KHR:
                    ConPrinter::PrintLn("Lost surface while querying surface formats.");
                    return DynArray<SurfaceFormat2>();
                default:
                    ConPrinter::PrintLn("Encountered unknown error while querying surface formats: {}.", result);
                    return DynArray<SurfaceFormat2>();
            }
        }
    } while(result == VK_INCOMPLETE);

    return DynArray<SurfaceFormat2>();
}

[[nodiscard]] static DynArray<VkPresentModeKHR> GetPhysicalDeviceSurfacePresentModes(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) noexcept
{
    VkResult result;
    u32 presentModeCount;
    
    do
    {
        result = ::tau::vd::InstanceFunctions::VkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

        if(result != VK_SUCCESS && result != VK_INCOMPLETE)
        {
            switch(result)
            {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    tau::vd::RecoverSacrificialMemory();
                    ConPrinter::PrintLn("Ran out of system memory while querying surface present modes.");
                    return DynArray<VkPresentModeKHR>();
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    ConPrinter::PrintLn("Ran out of device memory while querying surface present modes.");
                    return DynArray<VkPresentModeKHR>();
                case VK_ERROR_SURFACE_LOST_KHR:
                    ConPrinter::PrintLn("Lost surface while querying surface present modes.");
                    return DynArray<VkPresentModeKHR>();
                default:
                    ConPrinter::PrintLn("Encountered unknown error while querying surface present modes: {}.", result);
                    return DynArray<VkPresentModeKHR>();
            }
        }

        DynArray<VkPresentModeKHR> presentModes(presentModeCount);
        presentModes.Zero();
        
        result = ::tau::vd::InstanceFunctions::VkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes);

        if(result == VK_SUCCESS)
        {
            return presentModes;
        }
        else if(result != VK_INCOMPLETE)
        {
            switch(result)
            {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    tau::vd::RecoverSacrificialMemory();
                    ConPrinter::PrintLn("Ran out of system memory while querying surface present modes.");
                    return DynArray<VkPresentModeKHR>();
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    ConPrinter::PrintLn("Ran out of device memory while querying surface present modes.");
                    return DynArray<VkPresentModeKHR>();
                case VK_ERROR_SURFACE_LOST_KHR:
                    ConPrinter::PrintLn("Lost surface while querying surface present modes.");
                    return DynArray<VkPresentModeKHR>();
                default:
                    ConPrinter::PrintLn("Encountered unknown error while querying surface present modes: {}.", result);
                    return DynArray<VkPresentModeKHR>();
            }
        }
    } while(result == VK_INCOMPLETE);

    return DynArray<VkPresentModeKHR>();
}

static void RankDevice(RankedDevice* const device, const i32 index, const u32 vulkanVersion, VkSurfaceKHR surface) noexcept
{
    (void) vulkanVersion;

    i32 rank = -index;

    i32 propsRank;

    if(tau::vd::InstanceFunctions::VkGetPhysicalDeviceProperties2)
    {
        VkPhysicalDeviceProperties2 deviceProperties = GetPhysicalDeviceProperties2<VkPhysicalDeviceProperties2, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2>(device->Device, tau::vd::InstanceFunctions::VkGetPhysicalDeviceProperties2);
        propsRank = ComputeScoreOfDeviceProps(deviceProperties.properties, &device->DeviceVulkanVersion);
        
    }
    else if(tau::vd::InstanceFunctions::VkGetPhysicalDeviceProperties2KHR)
    {
        VkPhysicalDeviceProperties2KHR deviceProperties = GetPhysicalDeviceProperties2<VkPhysicalDeviceProperties2KHR, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR>(device->Device, tau::vd::InstanceFunctions::VkGetPhysicalDeviceProperties2KHR);
        propsRank = ComputeScoreOfDeviceProps(deviceProperties.properties, &device->DeviceVulkanVersion);
    }
    else
    {
        VkPhysicalDeviceProperties deviceProperties { };

        vkGetPhysicalDeviceProperties(device->Device, &deviceProperties);
        propsRank = ComputeScoreOfDeviceProps(deviceProperties, &device->DeviceVulkanVersion);
    }

    if(propsRank == IntMaxMin<decltype(propsRank)>::Min)
    {
        return;
    }

    rank += propsRank;

    device->GraphicsQueueIndex = -1;
    device->PresentQueueIndex = -1;

    if(tau::vd::InstanceFunctions::VkGetPhysicalDeviceQueueFamilyProperties2)
    {
        DynArray<VkQueueFamilyProperties2> queueFamilyProperties = GetPhysicalDeviceQueueFamilyProperties2<VkQueueFamilyProperties2, VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2>(device->Device, tau::vd::InstanceFunctions::VkGetPhysicalDeviceQueueFamilyProperties2);
        FindQueues(queueFamilyProperties, device, surface);
    }
    else if(tau::vd::InstanceFunctions::VkGetPhysicalDeviceQueueFamilyProperties2KHR)
    {
        DynArray<VkQueueFamilyProperties2KHR> queueFamilyProperties = GetPhysicalDeviceQueueFamilyProperties2<VkQueueFamilyProperties2KHR, VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2_KHR>(device->Device, tau::vd::InstanceFunctions::VkGetPhysicalDeviceQueueFamilyProperties2KHR);
        FindQueues(queueFamilyProperties, device, surface);
    }
    else
    {
        DynArray<VkQueueFamilyProperties> queueFamilyProperties = GetPhysicalDeviceQueueFamilyProperties2<VkQueueFamilyProperties, VK_STRUCTURE_TYPE_MAX_ENUM>(device->Device, vkGetPhysicalDeviceQueueFamilyProperties);
        FindQueues(queueFamilyProperties, device, surface);
    }

    if(!device->IsValid())
    {
        return;
    }

    if(device->IsGraphicsAndPresentSame())
    {
        rank += 256;
    }

    {
        u32 extensionCount;
        DynArray<const char*> enabledExtensions = tau::vd::GetRequestedDeviceExtensions(device->Device, device->DeviceVulkanVersion, &extensionCount);

        if constexpr(::std::size(tau::vd::RequiredDeviceExtensions) > 0)
        {
            if(extensionCount == 0)
            {
                return;
            }
        }

        device->EnabledExtensions = DynArray<const char*>(extensionCount);

        // Perform an optimized memcpy of the pointers. We don't need to worry about copies or moves.
        ::std::memcpy(device->EnabledExtensions.Array(), enabledExtensions.Array(), extensionCount * sizeof(const char*));
    }

    {
        VkResult result;

        if(::tau::vd::InstanceFunctions::VkGetPhysicalDeviceSurfaceCapabilities2KHR)
        {
            VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo { };
            surfaceInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
            surfaceInfo.pNext = nullptr;
            surfaceInfo.surface = surface;

            VkSurfaceCapabilities2KHR surfaceCapabilities { };
            surfaceCapabilities.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR;
            surfaceCapabilities.pNext = nullptr;

            result = ::tau::vd::InstanceFunctions::VkGetPhysicalDeviceSurfaceCapabilities2KHR(device->Device, &surfaceInfo, &surfaceCapabilities);

            (void) ::std::memcpy(&device->SurfaceCapabilities, &surfaceCapabilities.surfaceCapabilities, sizeof(device->SurfaceCapabilities));
        }
        else if(::tau::vd::InstanceFunctions::VkGetPhysicalDeviceSurfaceCapabilitiesKHR)
        {
            result = ::tau::vd::InstanceFunctions::VkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->Device, surface, &device->SurfaceCapabilities);
        }
        else
        {
            // vkGetPhysicalDeviceSurfaceCapabilitiesKHR should always exist...
            ConPrinter::PrintLn("Missing vkGetPhysicalDeviceSurfaceCapabilitiesKHR, this shouldn't be possible");
            return;
        }

        if(result != VK_SUCCESS)
        {
            switch(result)
            {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    tau::vd::RecoverSacrificialMemory();
                    ConPrinter::PrintLn("Ran out of system memory while querying surface capabilities.");
                    return;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    ConPrinter::PrintLn("Ran out of device memory while querying surface capabilities.");
                    return;
                case VK_ERROR_SURFACE_LOST_KHR:
                    ConPrinter::PrintLn("Lost surface while querying surface capabilities.");
                    return;
                default:
                    ConPrinter::PrintLn("Encountered unknown error while querying surface capabilities: {}.", result);
                    return;
            }
        }
    }

    {
        if(::tau::vd::InstanceFunctions::VkGetPhysicalDeviceSurfaceFormats2KHR)
        {
            DynArray<VkSurfaceFormat2KHR> formats = GetPhysicalDeviceSurfaceFormats2<VkSurfaceFormat2KHR, VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR>(device->Device, surface, ::tau::vd::InstanceFunctions::VkGetPhysicalDeviceSurfaceFormats2KHR);

            device->SurfaceFormats = DynArray<VkSurfaceFormatKHR>(formats.Length());

            for(uSys i = 0; i < formats.Length(); ++i)
            {
                (void) ::std::memcpy(&device->SurfaceFormats[i], &formats[i].surfaceFormat, sizeof(device->SurfaceFormats[0]));
            }
            
        }
        else if(::tau::vd::InstanceFunctions::VkGetPhysicalDeviceSurfaceFormatsKHR)
        {
            device->SurfaceFormats = GetPhysicalDeviceSurfaceFormats2<VkSurfaceFormatKHR, VK_STRUCTURE_TYPE_MAX_ENUM>(device->Device, surface, ::tau::vd::InstanceFunctions::VkGetPhysicalDeviceSurfaceFormatsKHR);
        }
        else
        {
            // vkGetPhysicalDeviceSurfaceFormatsKHR should always exist...
            ConPrinter::PrintLn("Missing vkGetPhysicalDeviceSurfaceFormatsKHR, this shouldn't be possible");
            return;
        }

        if(device->SurfaceFormats.Length() == 0)
        {
            return;
        }
    }

    {
        device->PresentModes = GetPhysicalDeviceSurfacePresentModes(device->Device, surface);

        if(device->SurfaceFormats.Length() == 0)
        {
            return;
        }
    }

    device->Rank = rank;
}

static void PickDevice(DynArray<VkPhysicalDevice> physicalDevices, const u32 vulkanVersion, VkSurfaceKHR surface, RankedDevice* const physicalDevice) noexcept
{
    RankedDevice maxRankedDevice;
    
    for(uSys i = 0; i < physicalDevices.Count(); ++i)
    {
        RankedDevice currentDevice;
        currentDevice.Device = physicalDevices[i];
        
        RankDevice(&currentDevice, static_cast<i32>(i), vulkanVersion, surface);
        if(currentDevice.Rank == IntMaxMin<i32>::Min)
        {
            continue;
        }

        if(currentDevice.Rank > maxRankedDevice.Rank)
        {
            maxRankedDevice = currentDevice;
        }
    }

    *physicalDevice = maxRankedDevice;
}

/**
 * \brief Ideally choose either an RGBA or BGRA sRGB format.
 *
 *   This will prioritize an sRGB format of either BGRA or RGBA. If it
 * does not find one of those 2 formats, it will pick the first sRGB
 * format it encounters, failing that it just defaults to the first
 * format.
 *
 * \param surfaceFormats The list of surface formats available to us.
 * \return The chosen format.
 */
[[nodiscard]] static VkSurfaceFormatKHR PickSwapChainFormat(const DynArray<VkSurfaceFormatKHR>& surfaceFormats) noexcept
{
    // A non-ideal sRGB format that we'll use if we can't find our ideal.
    const VkSurfaceFormatKHR* fallbackSrgbFormat = nullptr;

    for(uSys i = 0; i < surfaceFormats.Length(); ++i)
    {
        if(surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            switch(surfaceFormats[i].format)
            {
                case VK_FORMAT_B8G8R8A8_SRGB:
                case VK_FORMAT_R8G8B8A8_SRGB:
                    return surfaceFormats[i];
                default: break;
            }

            if(!fallbackSrgbFormat)
            {
                fallbackSrgbFormat = &surfaceFormats[i];
            }
        }
    }

    if(fallbackSrgbFormat)
    {
        return *fallbackSrgbFormat;
    }

    return surfaceFormats[0];
}

[[nodiscard]] static VkPresentModeKHR PickPresentMode(const DynArray<VkPresentModeKHR>& presentModes) noexcept
{
    // for(const VkPresentModeKHR presentMode : presentModes)
    // {
    //     if(presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
    //     {
    //         return presentMode;
    //     }
    // }

    return VK_PRESENT_MODE_FIFO_KHR;
}

[[nodiscard]] VkExtent2D PickSwapChainSize(const ReferenceCountingPointer<tau::vd::Window>& window, const VkSurfaceCapabilitiesKHR& capabilities) noexcept
{
    if(capabilities.currentExtent.width != IntMaxMin<decltype(capabilities.currentExtent.width)>::Max)
    {
        return capabilities.currentExtent;
    }

    VkExtent2D ret;
    ret.width = ClampT(window->FramebufferWidth(), capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    ret.height = ClampT(window->FramebufferHeight(), capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    return ret;
}

ReferenceCountingPointer<tau::vd::VulkanManager> tau::vd::VulkanManager::CreateVulkanManager(const ReferenceCountingPointer<Window>& window) noexcept
{
    const u32 vulkanVersion = GetVulkanVersion();

    VkInstance vkInstance;

    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo { };

    {
        u32 layerCount;
        DynArray<const char*> enabledInstanceLayers = GetRequestedInstanceLayers(&layerCount);
        u32 extensionCount;
        bool hasDebugExt;
        DynArray<const char*> enabledInstanceExtensions = GetRequestedInstanceExtensions(vulkanVersion, &extensionCount, &hasDebugExt);

        if constexpr(::std::size(RequiredInstanceExtensions) > 0)
        {
            if(extensionCount == 0)
            {
                return nullptr;
            }
        }

        VkApplicationInfo applicationInfo { };
        applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        applicationInfo.pNext = nullptr;
        applicationInfo.pApplicationName = "SoftGpu Display";
        applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        applicationInfo.pEngineName = "VD Display Engine";
        applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        applicationInfo.apiVersion = vulkanVersion;

        VkInstanceCreateInfo createInfo { };
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.pApplicationInfo = &applicationInfo;
        createInfo.enabledLayerCount = layerCount;
        createInfo.ppEnabledLayerNames = enabledInstanceLayers;
        createInfo.enabledExtensionCount = extensionCount;
        createInfo.ppEnabledExtensionNames = enabledInstanceExtensions;

        if(hasDebugExt)
        {
            debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugMessengerCreateInfo.messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugMessengerCreateInfo.messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugMessengerCreateInfo.pfnUserCallback = DebugCallback;
            debugMessengerCreateInfo.pNext = nullptr;

            createInfo.pNext = &debugMessengerCreateInfo;
        }

        const VkResult result = vkCreateInstance(&createInfo, nullptr, &vkInstance);

        if(result != VK_SUCCESS)
        {
            switch(result)
            {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    RecoverSacrificialMemory();
                    ConPrinter::PrintLn("Ran out of system memory while creating VkInstance.");
                    return nullptr;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    ConPrinter::PrintLn("Ran out of device memory while creating VkInstance.");
                    return nullptr;
                default:
                    ConPrinter::PrintLn("Encountered unknown error while creating VkInstance: {}.", result);
                    return nullptr;
            }
        }
    }

    InstanceFunctions::LoadInstanceFunctions(vkInstance, vulkanVersion);

    {
        if(!InstanceFunctions::VkGetPhysicalDeviceSurfaceSupportKHR)
        {
            return nullptr;
        }
        if(!InstanceFunctions::VkGetPhysicalDeviceSurfacePresentModesKHR)
        {
            return nullptr;
        }
    }

    VkDebugUtilsMessengerEXT vkDebugMessenger;

    {
        if(InstanceFunctions::VkCreateDebugUtilsMessengerEXT && InstanceFunctions::VkDestroyDebugUtilsMessengerEXT)
        {
            const VkResult result = InstanceFunctions::VkCreateDebugUtilsMessengerEXT(vkInstance, &debugMessengerCreateInfo, nullptr, &vkDebugMessenger);

            if(result != VK_SUCCESS)
            {
                switch(result)
                {
                    case VK_ERROR_OUT_OF_HOST_MEMORY:
                        RecoverSacrificialMemory();
                        ConPrinter::PrintLn("Ran out of system memory while creating VkDebugUtilsMessengerEXT.");
                        return nullptr;
                    default:
                        ConPrinter::PrintLn("Encountered unknown error while creating VkDebugUtilsMessengerEXT: {}.", result);
                        return nullptr;
                }
            }
        }
        else
        {
            vkDebugMessenger = VK_NULL_HANDLE;
        }
    }

    VkSurfaceKHR surface;

    {
        VkWin32SurfaceCreateInfoKHR createInfo { };
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.hinstance = window->ModuleInstance();
        createInfo.hwnd = window->WindowHandle();

        const VkResult result = vkCreateWin32SurfaceKHR(vkInstance, &createInfo, nullptr, &surface);

        if(result != VK_SUCCESS)
        {
            switch(result)
            {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    RecoverSacrificialMemory();
                    ConPrinter::PrintLn("Ran out of system memory while creating surface.");
                    return nullptr;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    ConPrinter::PrintLn("Ran out of device memory while creating surface.");
                    return nullptr;
                default:
                    ConPrinter::PrintLn("Encountered unknown error while creating surface: {}.", result);
                    return nullptr;
            }
        }
    }

    RankedDevice physicalDevice;

    {
        const DynArray<VkPhysicalDevice> physicalDevices = GetPhysicalDevices(vkInstance);
        PickDevice(physicalDevices, vulkanVersion, surface, &physicalDevice);

        if(physicalDevice.Device == VK_NULL_HANDLE || physicalDevice.GraphicsQueueIndex < 0 || physicalDevice.PresentQueueIndex < 0)
        {
            return nullptr;
        }
    }

    VkDevice device;

    {
        u32 extensionCount;
        DynArray<const char*> enabledExtensions = GetRequestedDeviceExtensions(physicalDevice.Device, physicalDevice.DeviceVulkanVersion, &extensionCount);

        if constexpr(::std::size(RequiredDeviceExtensions) > 0)
        {
            if(extensionCount == 0)
            {
                return nullptr;
            }
        }

        constexpr f32 queuePriorities[1] = { 1.0f };

        VkDeviceQueueCreateInfo queueCreateInfos[2] = { };

        queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[0].pNext = nullptr;
        queueCreateInfos[0].flags = 0;
        queueCreateInfos[0].queueFamilyIndex = static_cast<u32>(physicalDevice.GraphicsQueueIndex);
        queueCreateInfos[0].queueCount = 1;
        queueCreateInfos[0].pQueuePriorities = queuePriorities;

        queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[1].pNext = nullptr;
        queueCreateInfos[1].flags = 0;
        queueCreateInfos[1].queueFamilyIndex = static_cast<u32>(physicalDevice.PresentQueueIndex);
        queueCreateInfos[1].queueCount = 1;
        queueCreateInfos[1].pQueuePriorities = queuePriorities;

        // We don't really need any special features.
        VkPhysicalDeviceFeatures enabledFeatures { };

        // Only create one queue if that queue can do both graphics & present
        const u32 queueCount = physicalDevice.IsGraphicsAndPresentSame() ? 1 : ::std::size(queueCreateInfos);

        VkDeviceCreateInfo deviceCreateInfo { };
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pNext = nullptr;
        deviceCreateInfo.flags = 0;
        deviceCreateInfo.queueCreateInfoCount = queueCount;
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
        deviceCreateInfo.enabledLayerCount = 0;
        deviceCreateInfo.ppEnabledLayerNames = nullptr;
        deviceCreateInfo.enabledExtensionCount = extensionCount;
        deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions;
        deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

        const VkResult result = vkCreateDevice(physicalDevice.Device, &deviceCreateInfo, nullptr, &device);

        if(result != VK_SUCCESS)
        {
            switch(result)
            {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    RecoverSacrificialMemory();
                    ConPrinter::PrintLn("Ran out of system memory while creating device.");
                    return nullptr;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    ConPrinter::PrintLn("Ran out of device memory while creating device.");
                    return nullptr;
                case VK_ERROR_INITIALIZATION_FAILED:
                    ConPrinter::PrintLn("Failed to initialize device.");
                    return nullptr;
                case VK_ERROR_EXTENSION_NOT_PRESENT:
                    ConPrinter::PrintLn("An extension was not present during device creation, this should be preconditioned.");
                    return nullptr;
                case VK_ERROR_FEATURE_NOT_PRESENT:
                    ConPrinter::PrintLn("A feature was not present during device creation, this should be preconditioned.");
                    return nullptr;
                case VK_ERROR_TOO_MANY_OBJECTS:
                    ConPrinter::PrintLn("There were too many objects when creating a device.");
                    return nullptr;
                case VK_ERROR_DEVICE_LOST:
                    ConPrinter::PrintLn("The device was lost during device creation.");
                    return nullptr;
                default:
                    ConPrinter::PrintLn("Encountered unknown error while creating device: {}.", result);
                    return nullptr;
            }
        }
    }

    ReferenceCountingPointer<VulkanDevice> vulkanDevice = VulkanDevice::LoadDeviceFunctions(device, physicalDevice.DeviceVulkanVersion);

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    {
        vkGetDeviceQueue(device, static_cast<u32>(physicalDevice.GraphicsQueueIndex), 0, &graphicsQueue);
        vulkanDevice->GraphicsQueue() = graphicsQueue;

        if(!physicalDevice.IsGraphicsAndPresentSame())
        {
            vkGetDeviceQueue(device, static_cast<u32>(physicalDevice.PresentQueueIndex), 0, &presentQueue);
        }
        else
        {
            presentQueue = graphicsQueue;
        }

        vulkanDevice->PresentQueue() = presentQueue;
    }

    VkSwapchainKHR swapchain;
    const VkExtent2D swapchainSize = PickSwapChainSize(window, physicalDevice.SurfaceCapabilities);
    VkFormat swapchainImageFormat;

    {
        u32 frameCount = physicalDevice.SurfaceCapabilities.minImageCount + 1;
        if(physicalDevice.SurfaceCapabilities.maxImageCount > 0)
        {
            frameCount = minT(frameCount, physicalDevice.SurfaceCapabilities.maxImageCount);
        }

        const VkSurfaceFormatKHR surfaceFormat = PickSwapChainFormat(physicalDevice.SurfaceFormats);
        swapchainImageFormat = surfaceFormat.format;
        const VkPresentModeKHR presentMode = PickPresentMode(physicalDevice.PresentModes);

        VkSwapchainCreateInfoKHR createInfo { };
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.surface = surface;
        createInfo.minImageCount = frameCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = swapchainSize;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

        u32 queueFamilyIndices[2];

        if(physicalDevice.GraphicsQueueIndex != physicalDevice.PresentQueueIndex)
        {
            queueFamilyIndices[0] = static_cast<u32>(physicalDevice.GraphicsQueueIndex);
            queueFamilyIndices[1] = static_cast<u32>(physicalDevice.PresentQueueIndex);

            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = physicalDevice.SurfaceCapabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        const VkResult result = vulkanDevice->CreateSwapchainKHR(&createInfo, &swapchain);

        if(result != VK_SUCCESS)
        {
            switch(result)
            {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    RecoverSacrificialMemory();
                    ConPrinter::PrintLn("Ran out of system memory while creating swapchain.");
                    return nullptr;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    ConPrinter::PrintLn("Ran out of device memory while creating swapchain.");
                    return nullptr;
                case VK_ERROR_DEVICE_LOST:
                    ConPrinter::PrintLn("The device was lost during swapchain creation.");
                    return nullptr;
                case VK_ERROR_SURFACE_LOST_KHR:
                    ConPrinter::PrintLn("The surface was lost during swapchain creation.");
                    return nullptr;
                case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
                    ConPrinter::PrintLn("The window was already in use during swapchain creation.");
                    return nullptr;
                case VK_ERROR_INITIALIZATION_FAILED:
                    ConPrinter::PrintLn("Failed to initialize swapchain.");
                    return nullptr;
                default:
                    ConPrinter::PrintLn("Encountered unknown error while creating swapchain: {}.", result);
                    return nullptr;
            }
        }
    }

    DynArray<VkImage> swapchainImages;

    {
        VkResult result;
        do
        {
            u32 imageCount;
            result = vulkanDevice->GetSwapchainImagesKHR(swapchain, &imageCount, nullptr);

            if(result != VK_SUCCESS && result != VK_INCOMPLETE)
            {
                switch(result)
                {
                    case VK_ERROR_OUT_OF_HOST_MEMORY:
                        RecoverSacrificialMemory();
                        ConPrinter::PrintLn("Ran out of system memory while getting swapchain image count.");
                        return nullptr;
                    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                        ConPrinter::PrintLn("Ran out of device memory while getting swapchain image count.");
                        return nullptr;
                    default:
                        ConPrinter::PrintLn("Encountered unknown error while getting swapchain image count: {}.", result);
                        return nullptr;
                }
            }

            swapchainImages = DynArray<VkImage>(imageCount);

            result = vulkanDevice->GetSwapchainImagesKHR(swapchain, &imageCount, swapchainImages);

            if(result != VK_SUCCESS && result != VK_INCOMPLETE)
            {
                switch(result)
                {
                    case VK_ERROR_OUT_OF_HOST_MEMORY:
                        RecoverSacrificialMemory();
                        ConPrinter::PrintLn("Ran out of system memory while getting swapchain images.");
                        return nullptr;
                    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                        ConPrinter::PrintLn("Ran out of device memory while getting swapchain images.");
                        return nullptr;
                    default:
                        ConPrinter::PrintLn("Encountered unknown error while getting swapchain images: {}.", result);
                        return nullptr;
                }
            }
        } while(result == VK_INCOMPLETE);
    }

    DynArray<VkImageView> swapchainImageViews(swapchainImages.Length());

    for(uSys i = 0; i < swapchainImages.Length(); ++i)
    {
        VkImageViewCreateInfo createInfo { };
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.image = swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        const VkResult result = vulkanDevice->CreateImageView(&createInfo, &swapchainImageViews[i]);

        if(result != VK_SUCCESS)
        {
            switch(result)
            {
                case VK_ERROR_OUT_OF_HOST_MEMORY:
                    RecoverSacrificialMemory();
                    ConPrinter::PrintLn("Ran out of system memory while creating swapchain image view {}.", i);
                    return nullptr;
                case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                    ConPrinter::PrintLn("Ran out of device memory while creating swapchain image view {}.", i);
                    return nullptr;
                case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR:
                    ConPrinter::PrintLn("Invalid opaque capture address while creating swapchain image view {}.", i);
                    return nullptr;
                default:
                    ConPrinter::PrintLn("Encountered unknown error while creating swapchain image view {}: {}.", i, result);
                    return nullptr;
            }
        }
    }

    return ReferenceCountingPointer<VulkanManager>(vkInstance, vulkanVersion, vkDebugMessenger, window, surface, physicalDevice.Device, ::std::move(vulkanDevice), swapchain, swapchainImages, swapchainSize, swapchainImageFormat);
}
