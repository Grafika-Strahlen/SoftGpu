#include "vd/VulkanFunctions.hpp"
#include <vulkan/vk_enum_string_helper.h>
#include <ConPrinter.hpp>

#include "vd/MemoryRecovery.hpp"

#define VD_STR(X) #X
#define VD_XSTR(X) VD_STR(X)

#define LoadNonInstanceFunc(BaseName) Vk##BaseName = reinterpret_cast<PFN_vk##BaseName>(vkGetInstanceProcAddr(nullptr, "vk" #BaseName))
#define LoadInstanceFunc(BaseName) Vk##BaseName = reinterpret_cast<PFN_vk##BaseName>(vkGetInstanceProcAddr(m_Instance, "vk" #BaseName))
#define LoadDeviceFunc(BaseName) Vk##BaseName = reinterpret_cast<PFN_vk##BaseName>(vkGetDeviceProcAddr(m_Device, "vk" #BaseName))

PFN_vkEnumerateInstanceVersion VkEnumerateInstanceVersion;

namespace tau::vd {

void LoadNonInstanceFunctions() noexcept
{
    LoadNonInstanceFunc(EnumerateInstanceVersion);
}

ReferenceCountingPointer<VulkanInstance> VulkanInstance::LoadInstanceFunctions(VkInstance instance, const VkAllocationCallbacks* const allocator, const u32 vulkanVersion) noexcept
{
    ReferenceCountingPointer<VulkanInstance> ret(instance, allocator, vulkanVersion);

    ret->LoadInstanceFunctions();

    return ret;
}

void VulkanInstance::LoadInstanceFunctions() noexcept
{
    LoadInstanceFunc(CreateDebugUtilsMessengerEXT);
    LoadInstanceFunc(DestroyDebugUtilsMessengerEXT);

    LoadInstanceFunc(CreateWin32SurfaceKHR);
    LoadInstanceFunc(DestroySurfaceKHR);

    LoadInstanceFunc(EnumeratePhysicalDevices);

    LoadInstanceFunc(GetPhysicalDeviceSurfaceSupportKHR);
    LoadInstanceFunc(GetPhysicalDeviceSurfaceCapabilitiesKHR);
    LoadInstanceFunc(GetPhysicalDeviceSurfaceFormatsKHR);
    LoadInstanceFunc(GetPhysicalDeviceSurfacePresentModesKHR);

    LoadInstanceFunc(GetPhysicalDeviceSurfaceCapabilities2KHR);
    LoadInstanceFunc(GetPhysicalDeviceSurfaceFormats2KHR);

    if(m_Version >= VK_API_VERSION_1_0)
    {
        LoadInstanceFunc(GetPhysicalDeviceProperties2KHR);
        LoadInstanceFunc(GetPhysicalDeviceQueueFamilyProperties2KHR);
    }

    if(m_Version >= VK_API_VERSION_1_1)
    {
        LoadInstanceFunc(GetPhysicalDeviceProperties2);
        LoadInstanceFunc(GetPhysicalDeviceQueueFamilyProperties2);
    }

    if(m_Version >= VK_API_VERSION_1_2)
    {

    }

    if(m_Version >= VK_API_VERSION_1_3)
    {

    }
}

VkResult VulkanInstance::CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* const pCreateInfo, VkDebugUtilsMessengerEXT* const pMessenger) const noexcept
{
    if(!VkCreateDebugUtilsMessengerEXT)
    {
        return VK_ERROR_UNKNOWN;
    }

    return VkCreateDebugUtilsMessengerEXT(m_Instance, pCreateInfo, m_Allocator, pMessenger);
}

void VulkanInstance::DestroyDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT messenger) const noexcept
{
    if(!VkDestroyDebugUtilsMessengerEXT)
    {
        return;
    }

    VkDestroyDebugUtilsMessengerEXT(m_Instance, messenger, m_Allocator);
}

VkResult VulkanInstance::CreateWin32SurfaceKHR(const VkWin32SurfaceCreateInfoKHR* const pCreateInfo, VkSurfaceKHR* const pSurface) const noexcept
{
    if(!VkCreateWin32SurfaceKHR)
    {
        return VK_ERROR_UNKNOWN;
    }

    return VkCreateWin32SurfaceKHR(m_Instance, pCreateInfo, m_Allocator, pSurface);
}

void VulkanInstance::DestroySurfaceKHR(VkSurfaceKHR surface) const noexcept
{
    if(!VkDestroySurfaceKHR)
    {
        return;
    }

    VkDestroySurfaceKHR(m_Instance, surface, m_Allocator);
}

VkResult VulkanInstance::EnumeratePhysicalDevices(uint32_t* const pPhysicalDeviceCount, VkPhysicalDevice* const pPhysicalDevices) const noexcept
{
    if(!VkEnumeratePhysicalDevices)
    {
        return VK_ERROR_UNKNOWN;
    }

    return VkEnumeratePhysicalDevices(m_Instance, pPhysicalDeviceCount, pPhysicalDevices);
}

ReferenceCountingPointer<VulkanDevice> VulkanDevice::LoadDeviceFunctions(VkDevice device, const u32 deviceVulkanVersion, const VkAllocationCallbacks* const allocator) noexcept
{
    ReferenceCountingPointer<VulkanDevice> ret(device, allocator, deviceVulkanVersion);

    ret->LoadDeviceFunctions();

    return ret;
}

void VulkanDevice::LoadDeviceFunctions() noexcept
{
    LoadDeviceFunc(CreateSwapchainKHR);
    LoadDeviceFunc(DestroySwapchainKHR);
    LoadDeviceFunc(GetSwapchainImagesKHR);
    LoadDeviceFunc(CreateImageView);
    LoadDeviceFunc(DestroyImageView);

    if(m_DeviceVersion >= VK_API_VERSION_1_0)
    {
    }

    if(m_DeviceVersion >= VK_API_VERSION_1_1)
    {
    }

    if(m_DeviceVersion >= VK_API_VERSION_1_2)
    {

    }

    if(m_DeviceVersion >= VK_API_VERSION_1_3)
    {

    }
}

VkResult VulkanDevice::CreateSwapchainKHR(const VkSwapchainCreateInfoKHR* const pCreateInfo, VkSwapchainKHR* const pSwapchain) const noexcept
{
    if(!VkCreateSwapchainKHR)
    {
        return VK_ERROR_UNKNOWN;
    }

    return VkCreateSwapchainKHR(m_Device, pCreateInfo, m_Allocator, pSwapchain);
}

void VulkanDevice::DestroySwapchainKHR(VkSwapchainKHR swapchain) const noexcept
{
    if(!VkDestroySwapchainKHR)
    {
        return;
    }

    return VkDestroySwapchainKHR(m_Device, swapchain, m_Allocator);
}

VkResult VulkanDevice::GetSwapchainImagesKHR(VkSwapchainKHR swapchain, u32* const pSwapchainImageCount, VkImage* const pSwapchainImages) const noexcept
{
    if(!VkGetSwapchainImagesKHR)
    {
        return VK_ERROR_UNKNOWN;
    }

    return VkGetSwapchainImagesKHR(m_Device, swapchain, pSwapchainImageCount, pSwapchainImages);
}

VkResult VulkanDevice::CreateImageView(const VkImageViewCreateInfo* const pCreateInfo, VkImageView* const pView) const noexcept
{
    if(!VkCreateImageView)
    {
        return VK_ERROR_UNKNOWN;
    }

    return VkCreateImageView(m_Device, pCreateInfo, m_Allocator, pView);
}

void VulkanDevice::DestroyImageView(VkImageView imageView) const noexcept
{
    if(!VkDestroyImageView)
    {
        return;
    }

    return VkDestroyImageView(m_Device, imageView, m_Allocator);
}

void PrintError(const VkResult result, const DynString& source) noexcept
{
    const c8* message;

    switch(result)
    {
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            RecoverSacrificialMemory();
            message = u8"Ran out of system memory while";
            break;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            message = u8"Ran out of device memory while";
            break;
        case VK_ERROR_INITIALIZATION_FAILED:
            message = u8"Initialization failed while ";
            break;
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            message = u8"An extension was not present while";
            break;
        case VK_ERROR_FEATURE_NOT_PRESENT:
            message = u8"A feature was not present while";
            break;
        case VK_ERROR_TOO_MANY_OBJECTS:
            message = u8"There were too many objects present while";
            break;
        case VK_ERROR_DEVICE_LOST:
            message = u8"The device was lost while";
            break;
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            message = u8"The native was already in use while";
            break;
        default:
            message = u8"Unknown error encountered while";
            break;
    }

    ConPrinter::PrintLn(u8"[{}]: {} {}", string_VkResult(result), message, source);
}

}
