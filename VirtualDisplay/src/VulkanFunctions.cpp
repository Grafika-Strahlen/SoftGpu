#include "vd/VulkanFunctions.hpp"

#define VD_STR(X) #X
#define VD_XSTR(X) VD_STR(X)

#define LoadNonInstanceFunc(BaseName) Vk##BaseName = reinterpret_cast<PFN_vk##BaseName>(vkGetInstanceProcAddr(nullptr, "vk" #BaseName))
#define LoadInstanceFunc(BaseName, VulkanInstance) Vk##BaseName = reinterpret_cast<PFN_vk##BaseName>(vkGetInstanceProcAddr(VulkanInstance, "vk" #BaseName))
#define LoadDeviceFunc(BaseName, VulkanDevice) Vk##BaseName = reinterpret_cast<PFN_vk##BaseName>(vkGetDeviceProcAddr(VulkanDevice, "vk" #BaseName))


PFN_vkEnumerateInstanceVersion VkEnumerateInstanceVersion;

namespace tau::vd {

PFN_vkCreateDebugUtilsMessengerEXT InstanceFunctions::VkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT InstanceFunctions::VkDestroyDebugUtilsMessengerEXT;

PFN_vkGetPhysicalDeviceProperties2 InstanceFunctions::VkGetPhysicalDeviceProperties2;
PFN_vkGetPhysicalDeviceProperties2KHR InstanceFunctions::VkGetPhysicalDeviceProperties2KHR;

PFN_vkGetPhysicalDeviceQueueFamilyProperties2 InstanceFunctions::VkGetPhysicalDeviceQueueFamilyProperties2;
PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR InstanceFunctions::VkGetPhysicalDeviceQueueFamilyProperties2KHR;

void LoadNonInstanceFunctions() noexcept
{
    LoadNonInstanceFunc(EnumerateInstanceVersion);
}

void InstanceFunctions::LoadInstanceFunctions(VkInstance instance, const u32 vulkanVersion) noexcept
{
    LoadInstanceFunc(CreateDebugUtilsMessengerEXT, instance);
    LoadInstanceFunc(DestroyDebugUtilsMessengerEXT, instance);

    if(vulkanVersion >= VK_API_VERSION_1_0)
    {
        LoadInstanceFunc(GetPhysicalDeviceProperties2KHR, instance);
        LoadInstanceFunc(GetPhysicalDeviceQueueFamilyProperties2KHR, instance);
    }

    if(vulkanVersion >= VK_API_VERSION_1_1)
    {
        LoadInstanceFunc(GetPhysicalDeviceProperties2, instance);
        LoadInstanceFunc(GetPhysicalDeviceQueueFamilyProperties2, instance);
    }

    if(vulkanVersion >= VK_API_VERSION_1_2)
    {

    }

    if(vulkanVersion >= VK_API_VERSION_1_3)
    {

    }
}

ReferenceCountingPointer<VulkanDevice> VulkanDevice::LoadDeviceFunctions(VkDevice device, const u32 vulkanVersion, const VkAllocationCallbacks* const allocator) noexcept
{
    ReferenceCountingPointer<VulkanDevice> ret(device, allocator);



    return ret;
}

}
