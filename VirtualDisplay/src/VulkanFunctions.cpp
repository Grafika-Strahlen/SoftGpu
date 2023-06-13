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

PFN_vkGetPhysicalDeviceSurfaceSupportKHR InstanceFunctions::VkGetPhysicalDeviceSurfaceSupportKHR;
PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR InstanceFunctions::VkGetPhysicalDeviceSurfaceCapabilitiesKHR;
PFN_vkGetPhysicalDeviceSurfaceFormatsKHR InstanceFunctions::VkGetPhysicalDeviceSurfaceFormatsKHR;
PFN_vkGetPhysicalDeviceSurfacePresentModesKHR InstanceFunctions::VkGetPhysicalDeviceSurfacePresentModesKHR;

PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR InstanceFunctions::VkGetPhysicalDeviceSurfaceCapabilities2KHR;
PFN_vkGetPhysicalDeviceSurfaceFormats2KHR InstanceFunctions::VkGetPhysicalDeviceSurfaceFormats2KHR;

void LoadNonInstanceFunctions() noexcept
{
    LoadNonInstanceFunc(EnumerateInstanceVersion);
}

void InstanceFunctions::LoadInstanceFunctions(VkInstance instance, const u32 vulkanVersion) noexcept
{
    LoadInstanceFunc(CreateDebugUtilsMessengerEXT, instance);
    LoadInstanceFunc(DestroyDebugUtilsMessengerEXT, instance);

    LoadInstanceFunc(GetPhysicalDeviceSurfaceSupportKHR, instance);
    LoadInstanceFunc(GetPhysicalDeviceSurfaceCapabilitiesKHR, instance);
    LoadInstanceFunc(GetPhysicalDeviceSurfaceFormatsKHR, instance);
    LoadInstanceFunc(GetPhysicalDeviceSurfacePresentModesKHR, instance);

    LoadInstanceFunc(GetPhysicalDeviceSurfaceCapabilities2KHR, instance);
    LoadInstanceFunc(GetPhysicalDeviceSurfaceFormats2KHR, instance);

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

ReferenceCountingPointer<VulkanDevice> VulkanDevice::LoadDeviceFunctions(VkDevice device, const u32 deviceVulkanVersion, const VkAllocationCallbacks* const allocator) noexcept
{
    ReferenceCountingPointer<VulkanDevice> ret(device, allocator);

    ret->LoadDeviceFunctions(deviceVulkanVersion);

    return ret;
}

void VulkanDevice::LoadDeviceFunctions(const u32 deviceVulkanVersion) noexcept
{
    LoadDeviceFunc(CreateSwapchainKHR, m_Device);
    LoadDeviceFunc(DestroySwapchainKHR, m_Device);
    LoadDeviceFunc(GetSwapchainImagesKHR, m_Device);
    LoadDeviceFunc(CreateImageView, m_Device);
    LoadDeviceFunc(DestroyImageView, m_Device);

    if(deviceVulkanVersion >= VK_API_VERSION_1_0)
    {
    }

    if(deviceVulkanVersion >= VK_API_VERSION_1_1)
    {
    }

    if(deviceVulkanVersion >= VK_API_VERSION_1_2)
    {

    }

    if(deviceVulkanVersion >= VK_API_VERSION_1_3)
    {

    }
}

VkResult VulkanDevice::CreateSwapchainKHR(const VkSwapchainCreateInfoKHR* const pCreateInfo, VkSwapchainKHR* const pSwapchain) noexcept
{
    if(!VkCreateSwapchainKHR)
    {
        return VK_ERROR_UNKNOWN;
    }

    return VkCreateSwapchainKHR(m_Device, pCreateInfo, m_Allocator, pSwapchain);
}

void VulkanDevice::DestroySwapchainKHR(VkSwapchainKHR swapchain) noexcept
{
    if(!VkDestroySwapchainKHR)
    {
        return;
    }

    return VkDestroySwapchainKHR(m_Device, swapchain, m_Allocator);
}

VkResult VulkanDevice::GetSwapchainImagesKHR(VkSwapchainKHR swapchain, u32* const pSwapchainImageCount, VkImage* const pSwapchainImages) noexcept
{
    if(!VkGetSwapchainImagesKHR)
    {
        return VK_ERROR_UNKNOWN;
    }

    return VkGetSwapchainImagesKHR(m_Device, swapchain, pSwapchainImageCount, pSwapchainImages);
}

VkResult VulkanDevice::CreateImageView(const VkImageViewCreateInfo* const pCreateInfo, VkImageView* const pView) noexcept
{
    if(!VkCreateImageView)
    {
        return VK_ERROR_UNKNOWN;
    }

    return VkCreateImageView(m_Device, pCreateInfo, m_Allocator, pView);
}

void VulkanDevice::DestroyImageView(VkImageView imageView) noexcept
{
    if(!VkDestroyImageView)
    {
        return;
    }

    return VkDestroyImageView(m_Device, imageView, m_Allocator);
}

}
