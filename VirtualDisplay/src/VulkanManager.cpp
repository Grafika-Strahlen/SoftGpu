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

using namespace tau::vd;

VulkanManager::VulkanManager(
    StrongRef<VulkanInstance>&& vulkan,
    VkDebugUtilsMessengerEXT debugMessenger,
    const Ref<Window>& window,
    VkSurfaceKHR surface,
    VkPhysicalDevice physicalDevice, 
    StrongRef<VulkanDevice>&& device,
    VkSwapchainKHR swapchain,
    const DynArray<VkImage>& swapchainImages,
    const DynArray<VkImageView>& swapchainImageViews,
    VkExtent2D swapchainSize,
    VkFormat swapchainImageFormat
) noexcept
    : m_Vulkan(::std::move(vulkan))
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

VulkanManager::~VulkanManager() noexcept
{
    for(VkImageView imageView : m_SwapchainImageViews)
    {
        m_Device->DestroyImageView(imageView);
    }

    m_Device->DestroySwapchainKHR(m_Swapchain);

    // Force the device to destroy before we destroy the instance.
    m_Device = nullptr;

    m_Vulkan->DestroySurfaceKHR(m_Surface);

    if(m_Vulkan->VkDestroyDebugUtilsMessengerEXT && m_DebugMessenger)
    {
        m_Vulkan->DestroyDebugUtilsMessengerEXT(m_DebugMessenger);
    }
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

static DynArray<VkPhysicalDevice> GetPhysicalDevices(const StrongRef<VulkanInstance>& vulkan) noexcept
{
    u32 physicalDeviceCount;
    VK_CALL(vulkan->EnumeratePhysicalDevices(&physicalDeviceCount, nullptr), "querying number of physical devices.");

    VkResult result;

    do
    {
        DynArray<VkPhysicalDevice> devices(physicalDeviceCount);

        result = vulkan->EnumeratePhysicalDevices(&physicalDeviceCount, devices);

        VK_CALL(result, "querying physical devices.");

        if(result == VK_SUCCESS)
        {
            return devices;
        }
    } while(result == VK_INCOMPLETE);
    
    VK_ERROR_HANDLER_BRACE();
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

    [[nodiscard]] bool AreQueuesValid() const noexcept { return GraphicsQueueIndex >= 0 && PresentQueueIndex >= 0; }
    [[nodiscard]] bool IsValid() const noexcept { return AreQueuesValid() && HasRequiredExtensions(); }
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
static void FindQueues(const StrongRef<VulkanInstance>& vulkan, const DynArray<QueueFamilyProperties2>& queueFamilyProperties, RankedDevice* const device, VkSurfaceKHR surface) noexcept
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
            VK_CALL(vulkan->VkGetPhysicalDeviceSurfaceSupportKHR(device->Device, static_cast<u32>(i), surface, &presentSupport), "checking present support.");

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

    VK_ERROR_HANDLER_VOID();
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

    if constexpr(::std::is_same_v<SurfaceFormat2, VkSurfaceFormat2KHR>)
    {
        VK_CALL(func(physicalDevice, &surfaceInfo, &formatCount, nullptr), "querying surface format count.");
    }
    else
    {
        VK_CALL(func(physicalDevice, surface, &formatCount, nullptr), "querying surface format count.");
    }

    do
    {
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

        VK_CALL(result, "querying surface formats.")

        if(result == VK_SUCCESS)
        {
            return formats;
        }
    } while(result == VK_INCOMPLETE);

    return DynArray<SurfaceFormat2>();

    VK_ERROR_HANDLER_BRACE();
}

[[nodiscard]] static DynArray<VkPresentModeKHR> GetPhysicalDeviceSurfacePresentModes(const StrongRef<VulkanInstance>& vulkan, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) noexcept
{
    VkResult result;
    u32 presentModeCount;

    VK_CALL(vulkan->VkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr), "surface present mode count.");
    
    do
    {
        DynArray<VkPresentModeKHR> presentModes(presentModeCount);
        presentModes.Zero();
        
        result = vulkan->VkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes);

        VK_CALL(result, "querying surface present modes.")

        if(result == VK_SUCCESS)
        {
            return presentModes;
        }
    } while(result == VK_INCOMPLETE);

    return DynArray<VkPresentModeKHR>();

    VK_ERROR_HANDLER_BRACE();
}

static void RankDevice(const StrongRef<VulkanInstance>& vulkan, RankedDevice* const device, const i32 index, const u32 vulkanVersion, VkSurfaceKHR surface) noexcept
{
    (void) vulkanVersion;

    i32 rank = -index;

    i32 propsRank;

    if(vulkan->VkGetPhysicalDeviceProperties2)
    {
        VkPhysicalDeviceProperties2 deviceProperties = GetPhysicalDeviceProperties2<VkPhysicalDeviceProperties2, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2>(device->Device, vulkan->VkGetPhysicalDeviceProperties2);
        propsRank = ComputeScoreOfDeviceProps(deviceProperties.properties, &device->DeviceVulkanVersion);
    }
    else if(vulkan->VkGetPhysicalDeviceProperties2KHR)
    {
        VkPhysicalDeviceProperties2KHR deviceProperties = GetPhysicalDeviceProperties2<VkPhysicalDeviceProperties2KHR, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR>(device->Device, vulkan->VkGetPhysicalDeviceProperties2KHR);
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

    if(vulkan->VkGetPhysicalDeviceQueueFamilyProperties2)
    {
        DynArray<VkQueueFamilyProperties2> queueFamilyProperties = GetPhysicalDeviceQueueFamilyProperties2<VkQueueFamilyProperties2, VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2>(device->Device, vulkan->VkGetPhysicalDeviceQueueFamilyProperties2);
        FindQueues(vulkan, queueFamilyProperties, device, surface);
    }
    else if(vulkan->VkGetPhysicalDeviceQueueFamilyProperties2KHR)
    {
        DynArray<VkQueueFamilyProperties2KHR> queueFamilyProperties = GetPhysicalDeviceQueueFamilyProperties2<VkQueueFamilyProperties2KHR, VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2_KHR>(device->Device, vulkan->VkGetPhysicalDeviceQueueFamilyProperties2KHR);
        FindQueues(vulkan, queueFamilyProperties, device, surface);
    }
    else
    {
        DynArray<VkQueueFamilyProperties> queueFamilyProperties = GetPhysicalDeviceQueueFamilyProperties2<VkQueueFamilyProperties, VK_STRUCTURE_TYPE_MAX_ENUM>(device->Device, vkGetPhysicalDeviceQueueFamilyProperties);
        FindQueues(vulkan, queueFamilyProperties, device, surface);
    }

    if(!device->AreQueuesValid())
    {
        return;
    }

    if(device->IsGraphicsAndPresentSame())
    {
        rank += 256;
    }

    {
        u32 extensionCount;
        DynArray<const char*> enabledExtensions = GetRequestedDeviceExtensions(device->Device, device->DeviceVulkanVersion, &extensionCount);

        if constexpr(::std::size(RequiredDeviceExtensions) > 0)
        {
            if(extensionCount == 0)
            {
                return;
            }
        }

        device->EnabledExtensions = DynArray<const char*>(extensionCount);
        // Perform an optimized memcpy of the pointers. We don't need to worry about object copies or moves.
        device->EnabledExtensions.MemCpyCountFrom(enabledExtensions.Array(), extensionCount);
    }

    {
        if(vulkan->VkGetPhysicalDeviceSurfaceCapabilities2KHR)
        {
            VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo { };
            surfaceInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
            surfaceInfo.pNext = nullptr;
            surfaceInfo.surface = surface;

            VkSurfaceCapabilities2KHR surfaceCapabilities { };
            surfaceCapabilities.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR;
            surfaceCapabilities.pNext = nullptr;

            VK_CALL(vulkan->VkGetPhysicalDeviceSurfaceCapabilities2KHR(device->Device, &surfaceInfo, &surfaceCapabilities), "querying surface capabilities.");

            (void) ::std::memcpy(&device->SurfaceCapabilities, &surfaceCapabilities.surfaceCapabilities, sizeof(device->SurfaceCapabilities));
        }
        else if(vulkan->VkGetPhysicalDeviceSurfaceCapabilitiesKHR)
        {
            VK_CALL(vulkan->VkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->Device, surface, &device->SurfaceCapabilities), "querying surface capabilities.");
        }
        else
        {
            // vkGetPhysicalDeviceSurfaceCapabilitiesKHR should always exist...
            ConPrinter::PrintLn("Missing vkGetPhysicalDeviceSurfaceCapabilitiesKHR, this shouldn't be possible");
            return;
        }
    }

    {
        if(vulkan->VkGetPhysicalDeviceSurfaceFormats2KHR)
        {
            DynArray<VkSurfaceFormat2KHR> formats = GetPhysicalDeviceSurfaceFormats2<VkSurfaceFormat2KHR, VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR>(device->Device, surface, vulkan->VkGetPhysicalDeviceSurfaceFormats2KHR);

            device->SurfaceFormats = DynArray<VkSurfaceFormatKHR>(formats.Length());

            for(uSys i = 0; i < formats.Length(); ++i)
            {
                (void) ::std::memcpy(&device->SurfaceFormats[i], &formats[i].surfaceFormat, sizeof(device->SurfaceFormats[0]));
            }
            
        }
        else if(vulkan->VkGetPhysicalDeviceSurfaceFormatsKHR)
        {
            device->SurfaceFormats = GetPhysicalDeviceSurfaceFormats2<VkSurfaceFormatKHR, VK_STRUCTURE_TYPE_MAX_ENUM>(device->Device, surface, vulkan->VkGetPhysicalDeviceSurfaceFormatsKHR);
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
        device->PresentModes = GetPhysicalDeviceSurfacePresentModes(vulkan, device->Device, surface);

        if(device->SurfaceFormats.Length() == 0)
        {
            return;
        }
    }

    device->Rank = rank;

    VK_ERROR_HANDLER_VOID();
}

static void PickDevice(const StrongRef<VulkanInstance>& vulkan, const DynArray<VkPhysicalDevice>& physicalDevices, const u32 vulkanVersion, VkSurfaceKHR surface, RankedDevice* const physicalDevice) noexcept
{
    RankedDevice maxRankedDevice;
    
    for(uSys i = 0; i < physicalDevices.Count(); ++i)
    {
        RankedDevice currentDevice;
        currentDevice.Device = physicalDevices[i];
        
        RankDevice(vulkan, &currentDevice, static_cast<i32>(i), vulkanVersion, surface);
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
    (void) presentModes;

    // for(const VkPresentModeKHR presentMode : presentModes)
    // {
    //     if(presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
    //     {
    //         return presentMode;
    //     }
    // }

    return VK_PRESENT_MODE_FIFO_KHR;
}

[[nodiscard]] VkExtent2D PickSwapChainSize(const Ref<Window>& window, const VkSurfaceCapabilitiesKHR& capabilities) noexcept
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

[[nodiscard]] DynArray<VkImage> GetSwapchainImages(const StrongRef<VulkanDevice>& vulkanDevice, VkSwapchainKHR swapchain) noexcept
{
    u32 imageCount;
    VK_CALL(vulkanDevice->GetSwapchainImagesKHR(swapchain, &imageCount, nullptr), "quering swapchain image count.");
    
    VkResult result;
    do
    {
        DynArray<VkImage> swapchainImages(imageCount);

        result = vulkanDevice->GetSwapchainImagesKHR(swapchain, &imageCount, swapchainImages);

        VK_CALL(result, "getting swapchain images.");

        if(result == VK_SUCCESS)
        {
            return swapchainImages;
        }
    } while(result == VK_INCOMPLETE);

    VK_ERROR_HANDLER_BRACE();
}

[[nodiscard]] StrongRef<VulkanInstance> CreateVulkanInstance(const u32 vulkanVersion, VkDebugUtilsMessengerCreateInfoEXT& debugMessengerCreateInfo) noexcept
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

    VkInstance vkInstance;

    VK_CALL(vkCreateInstance(&createInfo, nullptr, &vkInstance), "creating VkInstance.");

    return VulkanInstance::LoadInstanceFunctions(vkInstance, nullptr, vulkanVersion);

    VK_ERROR_HANDLER_NULL();
}

[[nodiscard]] VkDebugUtilsMessengerEXT CreateDebugMessenger(const StrongRef<VulkanInstance>& vulkan, const VkDebugUtilsMessengerCreateInfoEXT& debugMessengerCreateInfo) noexcept
{
    VkDebugUtilsMessengerEXT vkDebugMessenger;

    if(vulkan->VkCreateDebugUtilsMessengerEXT && vulkan->VkDestroyDebugUtilsMessengerEXT)
    {
        VK_CALL(vulkan->CreateDebugUtilsMessengerEXT(&debugMessengerCreateInfo, &vkDebugMessenger), "creating Debug Messenger.");
        return vkDebugMessenger;
    }
    
    VK_ERROR_HANDLER_VK_NULL();
}

[[nodiscard]] VkSurfaceKHR CreateSurface(const Ref<Window>& window, const StrongRef<VulkanInstance>& vulkan) noexcept
{
    VkWin32SurfaceCreateInfoKHR createInfo { };
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.hinstance = window->ModuleInstance();
    createInfo.hwnd = window->WindowHandle();

    VkSurfaceKHR surface;

    VK_CALL(vulkan->CreateWin32SurfaceKHR(&createInfo, &surface), "creating surface.");

    return surface;

    VK_ERROR_HANDLER_VK_NULL();
}

[[nodiscard]] VkDevice CreateDevice(const StrongRef<VulkanInstance>& vulkan, const RankedDevice& physicalDevice) noexcept
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
    constexpr VkPhysicalDeviceFeatures enabledFeatures { };

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

    VkDevice device;

    VK_CALL(vulkan->VkCreateDevice(physicalDevice.Device, &deviceCreateInfo, nullptr, &device), "creating device.");

    return device;

    VK_ERROR_HANDLER_VK_NULL();
}

[[nodiscard]] VkSwapchainKHR CreateSwapchain(const StrongRef<VulkanDevice>& vulkanDevice, const RankedDevice& physicalDevice, VkSurfaceKHR surface, const VkSurfaceFormatKHR& surfaceFormat, const VkExtent2D swapchainSize) noexcept
{
    u32 frameCount = physicalDevice.SurfaceCapabilities.minImageCount + 1;
    if(physicalDevice.SurfaceCapabilities.maxImageCount > 0)
    {
        frameCount = minT(frameCount, physicalDevice.SurfaceCapabilities.maxImageCount);
    }
    
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
    // TODO: See if we need to have the color attachment bit, and if we need to have image views.
    createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

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

    VkSwapchainKHR swapchain;

    VK_CALL(vulkanDevice->CreateSwapchainKHR(&createInfo, &swapchain), "creating swapchain.");

    return swapchain;

    VK_ERROR_HANDLER_VK_NULL();
}

[[nodiscard]] DynArray<VkImageView> CreateSwapchainImageViews(const StrongRef<VulkanDevice>& vulkanDevice, const DynArray<VkImage>& swapchainImages, const VkFormat swapchainImageFormat) noexcept
{
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

        VK_CALL_FMT(vulkanDevice->CreateImageView(&createInfo, &swapchainImageViews[i]), "creating swapchain image view {}.", i);
    }

    return swapchainImageViews;

    VK_ERROR_HANDLER_BRACE();
}

ReferenceCountingPointer<VulkanManager> VulkanManager::CreateVulkanManager(const ReferenceCountingPointer<Window>& window) noexcept
{
    const u32 vulkanVersion = GetVulkanVersion();

    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo { };

    StrongRef<VulkanInstance> vulkan = CreateVulkanInstance(vulkanVersion, debugMessengerCreateInfo);

    {
        if(!vulkan)
        {
            return nullptr;
        }
        if(!vulkan->VkGetPhysicalDeviceSurfaceSupportKHR)
        {
            return nullptr;
        }
        if(!vulkan->VkGetPhysicalDeviceSurfacePresentModesKHR)
        {
            return nullptr;
        }
    }

    VkDebugUtilsMessengerEXT vkDebugMessenger = CreateDebugMessenger(vulkan, debugMessengerCreateInfo);

    VkSurfaceKHR surface = CreateSurface(window, vulkan);

    if(!surface)
    {
        return nullptr;
    }

    RankedDevice physicalDevice;

    {
        const DynArray<VkPhysicalDevice> physicalDevices = GetPhysicalDevices(vulkan);
        PickDevice(vulkan, physicalDevices, vulkanVersion, surface, &physicalDevice);

        if(physicalDevice.Device == VK_NULL_HANDLE || physicalDevice.GraphicsQueueIndex < 0 || physicalDevice.PresentQueueIndex < 0)
        {
            return nullptr;
        }
    }

    VkDevice device = CreateDevice(vulkan, physicalDevice);

    if(!device)
    {
        return nullptr;
    }

    StrongRef<VulkanDevice> vulkanDevice = VulkanDevice::LoadDeviceFunctions(vulkan, device, physicalDevice.DeviceVulkanVersion, static_cast<u32>(physicalDevice.GraphicsQueueIndex), static_cast<u32>(physicalDevice.PresentQueueIndex));

    if(!vulkanDevice)
    {
        return nullptr;
    }

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    {
        vulkanDevice->GetDeviceQueue(static_cast<u32>(physicalDevice.GraphicsQueueIndex), 0, &graphicsQueue);
        vulkanDevice->GraphicsQueue() = graphicsQueue;

        if(!physicalDevice.IsGraphicsAndPresentSame())
        {
            vulkanDevice->GetDeviceQueue(static_cast<u32>(physicalDevice.PresentQueueIndex), 0, &presentQueue);
        }
        else
        {
            presentQueue = graphicsQueue;
        }

        vulkanDevice->PresentQueue() = presentQueue;
    }

    const VkExtent2D swapchainSize = PickSwapChainSize(window, physicalDevice.SurfaceCapabilities);
    const VkSurfaceFormatKHR surfaceFormat = PickSwapChainFormat(physicalDevice.SurfaceFormats);
    const VkFormat swapchainImageFormat = surfaceFormat.format;

    VkSwapchainKHR swapchain = CreateSwapchain(vulkanDevice, physicalDevice, surface, surfaceFormat, swapchainSize);

    if(!swapchain)
    {
        return nullptr;
    }

    const DynArray<VkImage> swapchainImages = GetSwapchainImages(vulkanDevice, swapchain);

    if(swapchainImages.Length() == 0)
    {
        return nullptr;
    }
    
    const DynArray<VkImageView> swapchainImageViews = CreateSwapchainImageViews(vulkanDevice, swapchainImages, swapchainImageFormat);

    if(swapchainImageViews.Length() != swapchainImages.Length())
    {
        return nullptr;
    }

    return Ref<VulkanManager>(::std::move(vulkan), vkDebugMessenger, window, surface, physicalDevice.Device, ::std::move(vulkanDevice), swapchain, swapchainImages, swapchainImageViews, swapchainSize, swapchainImageFormat);
}
