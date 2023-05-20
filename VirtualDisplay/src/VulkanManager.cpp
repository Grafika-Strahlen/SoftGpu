#include "vd/VulkanManager.hpp"
#include "vd/VulkanDebug.hpp"
#include "vd/MemoryRecovery.hpp"
#include "vd/VulkanFunctions.hpp"
#include "vd/Config.hpp"

#include <DynArray.hpp>
#include <String.hpp>
#include <ConPrinter.hpp>

tau::vd::VulkanManager::~VulkanManager() noexcept
{
    if(InstanceFunctions::VkDestroyDebugUtilsMessengerEXT && m_DebugMessenger)
    {
        InstanceFunctions::VkDestroyDebugUtilsMessengerEXT(m_Vulkan, m_DebugMessenger, nullptr);
    }

    vkDestroyInstance(m_Vulkan, nullptr);
}

#if defined(VK_KHR_win32_surface) && VK_KHR_win32_surface
static constexpr ConstExprString VkExtDebugUtilsExtName(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

static constexpr ConstExprString DesiredLayers[] = {
    "VK_LAYER_KHRONOS_validation"
};

static constexpr ConstExprString RequiredInstanceExtensions[] = {
#if defined(VK_KHR_surface) && VK_KHR_surface
    VK_KHR_SURFACE_EXTENSION_NAME,
#endif
#if defined(VK_KHR_win32_surface) && VK_KHR_win32_surface
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
};

static constexpr ConstExprString DesiredInstanceExtensions[] = {
#if defined(VK_KHR_win32_surface) && VK_KHR_win32_surface
    VkExtDebugUtilsExtName,
#endif
};

static constexpr ConstExprString Desired1_0InstanceExtensions[] = {
#if defined(VK_KHR_get_physical_device_properties2) && VK_KHR_get_physical_device_properties2 && !VD_FORCE_DISABLE_VK_KHR_get_physical_device_properties2
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
#endif
};

[[maybe_unused]] static constexpr ConstExprString RequiredDeviceExtensions[] = {
#if defined(VK_KHR_display_swapchain) && VK_KHR_display_swapchain
    VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME,
#endif
#if defined(VK_KHR_swapchain) && VK_KHR_swapchain
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#endif
};

[[maybe_unused]] static constexpr ConstExprString DesiredDeviceExtensions[] = {
};

static DynArray<VkLayerProperties> GetInstanceLayers() noexcept
{
    u32 propertyCount;
    VkResult result = vkEnumerateInstanceLayerProperties(&propertyCount, nullptr);

    if(result == VK_ERROR_OUT_OF_HOST_MEMORY)
    {
        tau::vd::RecoverSacrificialMemory();
        ConPrinter::PrintLn("Ran out of system memory while querying number of instance layers.");
        return { };
    }

    if(result == VK_ERROR_OUT_OF_DEVICE_MEMORY)
    {
        ConPrinter::PrintLn("Ran out of device memory while querying number of instance layers.");
        return { };
    }

    if(result != VK_SUCCESS && result != VK_INCOMPLETE)
    {
        ConPrinter::PrintLn("Encountered undefined error while querying number of instance layers: {}.", result);
        return { };
    }

    do
    {
        DynArray<VkLayerProperties> layers(propertyCount);

        result = vkEnumerateInstanceLayerProperties(&propertyCount, layers);

        if(result == VK_ERROR_OUT_OF_HOST_MEMORY)
        {
            tau::vd::RecoverSacrificialMemory();
            ConPrinter::PrintLn("Ran system memory while querying instance layers.");
            return { };
        }

        if(result == VK_ERROR_OUT_OF_DEVICE_MEMORY)
        {
            ConPrinter::PrintLn("Ran device memory while querying instance layers.");
            return { };
        }

        if(result == VK_SUCCESS)
        {
            return layers;
        }
    } while(result == VK_INCOMPLETE);

    ConPrinter::PrintLn("Encountered undefined error while querying instance layers: {}.", result);
    return { };
}

static DynArray<VkExtensionProperties> GetInstanceExtensions() noexcept
{
    u32 propertyCount;
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, nullptr);

    if(result == VK_ERROR_OUT_OF_HOST_MEMORY)
    {
        tau::vd::RecoverSacrificialMemory();
        ConPrinter::PrintLn("Ran out of system memory while querying number of instance extensions.");
        return { };
    }

    if(result == VK_ERROR_OUT_OF_DEVICE_MEMORY)
    {
        ConPrinter::PrintLn("Ran out of device memory while querying number of instance extensions.");
        return { };
    }

    if(result != VK_SUCCESS && result != VK_INCOMPLETE)
    {
        ConPrinter::PrintLn("Encountered undefined error while querying number of instance extensions: {}.", result);
        return { };
    }

    do
    {
        DynArray<VkExtensionProperties> extensions(propertyCount);

        result = vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, extensions);

        if(result == VK_ERROR_OUT_OF_HOST_MEMORY)
        {
            tau::vd::RecoverSacrificialMemory();
            ConPrinter::PrintLn("Ran out of system memory while querying instance extensions.");
            return { };
        }

        if(result == VK_ERROR_OUT_OF_DEVICE_MEMORY)
        {
            ConPrinter::PrintLn("Ran out of device memory while querying instance extensions.");
            return { };
        }

        if(result == VK_SUCCESS)
        {
            return extensions;
        }
    }
    while(result == VK_INCOMPLETE);

    ConPrinter::PrintLn("Encountered undefined error while querying instance extensions: 0x{XP0}.", result);
    return { };
}

static DynArray<const char*> GetRequestedInstanceLayers(u32* const layerCount) noexcept
{
    DynArray<VkLayerProperties> instanceLayers = GetInstanceLayers();

    if(instanceLayers.Length() == 0)
    {
        return { };
    }

    // ConPrinter::PrintLn("Found the following instance layers:");
    // for(const VkLayerProperties& props : instanceLayers)
    // {
    //     ConPrinter::PrintLn("  {}", props.layerName);
    // }

    DynArray<const char*> enabledInstanceLayers(instanceLayers.Length());

    uSys insertIndex = 0;
    
    for(uSys i = 0; i < ::std::size(DesiredLayers); ++i)
    {
        for(uSys j = 0; j < instanceLayers.Length(); ++j)
        {
            if(::std::strcmp(DesiredLayers[i], instanceLayers[j].layerName) == 0)
            {
                enabledInstanceLayers[insertIndex++] = DesiredLayers[i];
                break;
            }
        }
    }

    *layerCount = static_cast<u32>(insertIndex);

    return enabledInstanceLayers;
}

static DynArray<const char*> GetRequestedInstanceExtensions(const u32 vulkanVersion, u32* const extensionCount, bool* hasDebugExt) noexcept
{
    *extensionCount = 0;
    *hasDebugExt = false;
    DynArray<VkExtensionProperties> instanceExtensions = GetInstanceExtensions();

    if(instanceExtensions.Length() == 0)
    {
        return { };
    }

    for(const ConstExprString& extension : RequiredInstanceExtensions)
    {
        bool found = false;

        for(const VkExtensionProperties& availableExtension : instanceExtensions)
        {
            if(extension == availableExtension.extensionName)
            {
                found = true;
                break;
            }
        }

        // We failed to find a required extension.
        if(!found)
        {
            ConPrinter::PrintLn("Failed to find required instance extension {}.", extension);
            return { };
        }
    }

    DynArray<const char*> enabledInstanceExtensions(instanceExtensions.Length());

    uSys insertIndex = 0;

    for(const ConstExprString& extension : RequiredInstanceExtensions)
    {
        enabledInstanceExtensions[insertIndex++] = extension;
    }

    for(const ConstExprString& extension : DesiredInstanceExtensions)
    {
        for(const VkExtensionProperties& availableExtension : instanceExtensions)
        {
            if(extension == availableExtension.extensionName)
            {
#if defined(VK_KHR_win32_surface) && VK_KHR_win32_surface
                if(extension.Equals(VkExtDebugUtilsExtName))
                {
                    *hasDebugExt = true;
                }
#endif

                enabledInstanceExtensions[insertIndex++] = extension;
                break;
            }
        }
    }

    if(vulkanVersion == VK_API_VERSION_1_0)
    {
        for(const ConstExprString& extension : Desired1_0InstanceExtensions)
        {
            for(const VkExtensionProperties& availableExtension : instanceExtensions)
            {
                if(extension == availableExtension.extensionName)
                {
                    enabledInstanceExtensions[insertIndex++] = extension;
                    break;
                }
            }
        }
    }

    *extensionCount = static_cast<u32>(insertIndex);

    return enabledInstanceExtensions;
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

static i32 ComputeScoreOfDeviceProps(const VkPhysicalDeviceProperties& deviceProperties) noexcept
{
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
    ::std::memset(queueFamilyProperties, 0, sizeof(queueFamilyProperties[0]) * queueFamilyProperties.Length());

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

static i32 RankDevice(VkPhysicalDevice physicalDevice, const i32 index, const u32 vulkanVersion, iSys* const graphicsQueueIndex) noexcept
{
    (void) vulkanVersion;

    i32 rank = -index;

    i32 propsRank;

    if(tau::vd::InstanceFunctions::VkGetPhysicalDeviceProperties2)
    {
        VkPhysicalDeviceProperties2 deviceProperties = GetPhysicalDeviceProperties2<VkPhysicalDeviceProperties2, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2>(physicalDevice, tau::vd::InstanceFunctions::VkGetPhysicalDeviceProperties2);
        propsRank = ComputeScoreOfDeviceProps(deviceProperties.properties);
        
    }
    else if(tau::vd::InstanceFunctions::VkGetPhysicalDeviceProperties2KHR)
    {
        VkPhysicalDeviceProperties2KHR deviceProperties = GetPhysicalDeviceProperties2<VkPhysicalDeviceProperties2KHR, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR>(physicalDevice, tau::vd::InstanceFunctions::VkGetPhysicalDeviceProperties2KHR);
        propsRank = ComputeScoreOfDeviceProps(deviceProperties.properties);
    }
    else
    {
        VkPhysicalDeviceProperties deviceProperties { };

        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
        propsRank = ComputeScoreOfDeviceProps(deviceProperties);
    }

    if(propsRank == IntMaxMin<decltype(propsRank)>::Min)
    {
        return IntMaxMin<i32>::Min;
    }

    rank += propsRank;

    *graphicsQueueIndex = -1;

    if(tau::vd::InstanceFunctions::VkGetPhysicalDeviceQueueFamilyProperties2)
    {
        DynArray<VkQueueFamilyProperties2> queueFamilyProperties = GetPhysicalDeviceQueueFamilyProperties2<VkQueueFamilyProperties2, VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2>(physicalDevice, tau::vd::InstanceFunctions::VkGetPhysicalDeviceQueueFamilyProperties2);

        for(iSys i = 0; i < static_cast<iSys>(queueFamilyProperties.Length()); ++i)
        {
            if(queueFamilyProperties[i].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                *graphicsQueueIndex = i;
                break;
            }
        }
    }
    else if(tau::vd::InstanceFunctions::VkGetPhysicalDeviceQueueFamilyProperties2KHR)
    {
        DynArray<VkQueueFamilyProperties2KHR> queueFamilyProperties = GetPhysicalDeviceQueueFamilyProperties2<VkQueueFamilyProperties2KHR, VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2_KHR>(physicalDevice, tau::vd::InstanceFunctions::VkGetPhysicalDeviceQueueFamilyProperties2KHR);

        for(iSys i = 0; i < static_cast<iSys>(queueFamilyProperties.Length()); ++i)
        {
            if(queueFamilyProperties[i].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                *graphicsQueueIndex = i;
                break;
            }
        }
    }
    else
    {
        DynArray<VkQueueFamilyProperties> queueFamilyProperties = GetPhysicalDeviceQueueFamilyProperties2<VkQueueFamilyProperties, VK_STRUCTURE_TYPE_MAX_ENUM>(physicalDevice, vkGetPhysicalDeviceQueueFamilyProperties);

        for(iSys i = 0; i < static_cast<iSys>(queueFamilyProperties.Length()); ++i)
        {
            if(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                *graphicsQueueIndex = i;
                break;
            }
        }
    }

    if(*graphicsQueueIndex == -1)
    {
        return IntMaxMin<i32>::Min;
    }

    return rank;
}

static VkPhysicalDevice PickDevice(DynArray<VkPhysicalDevice> physicalDevices, const u32 vulkanVersion, iSys* const maxGraphicsQueueIndex) noexcept
{
    i32 maxRank = IntMaxMin<decltype(maxRank)>::Min;
    VkPhysicalDevice maxScoredDevice = VK_NULL_HANDLE;

    for(uSys i = 0; i < physicalDevices.Count(); ++i)
    {
        iSys graphicsQueueIndex;
        const i32 rank = RankDevice(physicalDevices[i], static_cast<i32>(i), vulkanVersion, &graphicsQueueIndex);
        if(rank == IntMaxMin<decltype(rank)>::Min)
        {
            continue;
        }

        if(rank > maxRank)
        {
            maxRank = rank;
            maxScoredDevice = physicalDevices[i];
            *maxGraphicsQueueIndex = graphicsQueueIndex;
        }
    }

    return maxScoredDevice;
}

ReferenceCountingPointer<tau::vd::VulkanManager> tau::vd::VulkanManager::CreateVulkanManager() noexcept
{
    const u32 vulkanVersion = GetVulkanVersion();

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

    VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo { };

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

    {
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

    VkPhysicalDevice physicalDevice;

    {
        const DynArray<VkPhysicalDevice> physicalDevices = GetPhysicalDevices(vkInstance);
        iSys maxGraphicsQueueIndex;
        physicalDevice = PickDevice(physicalDevices, vulkanVersion, &maxGraphicsQueueIndex);

        if(physicalDevice == VK_NULL_HANDLE)
        {
            return nullptr;
        }
    }
    
    return ReferenceCountingPointer<VulkanManager>(vkInstance, vulkanVersion, vkDebugMessenger, physicalDevice);
}
