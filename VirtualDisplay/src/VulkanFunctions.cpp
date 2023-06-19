#include "vd/VulkanFunctions.hpp"
#include <vulkan/vk_enum_string_helper.h>
#include <ConPrinter.hpp>

#include "vd/MemoryRecovery.hpp"

#define VK_ERROR_FUNCTION_NOT_FOUND (VK_ERROR_UNKNOWN)

#define VD_STR(X) #X
#define VD_XSTR(X) VD_STR(X)

#define LoadNonInstanceFunc(BaseName) Vk##BaseName = reinterpret_cast<PFN_vk##BaseName>(vkGetInstanceProcAddr(nullptr, "vk" #BaseName))
#define LoadInstanceFunc(BaseName) Vk##BaseName = reinterpret_cast<PFN_vk##BaseName>(vkGetInstanceProcAddr(m_Instance, "vk" #BaseName))
#define LoadDeviceFunc(BaseName) Vk##BaseName = reinterpret_cast<PFN_vk##BaseName>(vkGetDeviceProcAddr(m_Device, "vk" #BaseName))

VulkanDeclFunc(EnumerateInstanceVersion);

namespace tau::vd {

void LoadNonInstanceFunctions() noexcept
{
    LoadNonInstanceFunc(EnumerateInstanceVersion);
}

StrongRef<VulkanInstance> VulkanInstance::LoadInstanceFunctions(VkInstance instance, const VkAllocationCallbacks* const allocator, const u32 vulkanVersion) noexcept
{
    StrongRef<VulkanInstance> ret(instance, allocator, vulkanVersion);

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

    LoadInstanceFunc(CreateDevice);

    if(m_Version <= VK_API_VERSION_1_0)
    {
        LoadInstanceFunc(GetPhysicalDeviceProperties2KHR);
        LoadInstanceFunc(GetPhysicalDeviceQueueFamilyProperties2KHR);
    }

    if(m_Version <= VK_API_VERSION_1_1)
    {
        LoadInstanceFunc(GetPhysicalDeviceProperties2);
        LoadInstanceFunc(GetPhysicalDeviceQueueFamilyProperties2);
    }

    if(m_Version <= VK_API_VERSION_1_2)
    {

    }

    if(m_Version <= VK_API_VERSION_1_3)
    {

    }
}

VkResult VulkanInstance::CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* const pCreateInfo, VkDebugUtilsMessengerEXT* const pMessenger) const noexcept
{
    if(!VkCreateDebugUtilsMessengerEXT)
    {
        return VK_ERROR_FUNCTION_NOT_FOUND;
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
        return VK_ERROR_FUNCTION_NOT_FOUND;
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
        return VK_ERROR_FUNCTION_NOT_FOUND;
    }

    return VkEnumeratePhysicalDevices(m_Instance, pPhysicalDeviceCount, pPhysicalDevices);
}

StrongRef<VulkanDevice> VulkanDevice::LoadDeviceFunctions(
    const WeakRef<VulkanInstance>& vulkan, 
    VkDevice device, 
    const u32 deviceVulkanVersion,
    const u32 graphicsQueueFamilyIndex,
    const u32 presentQueueFamilyIndex,
    const VkAllocationCallbacks* const allocator
) noexcept
{
    StrongRef<VulkanDevice> ret(vulkan, device, allocator, deviceVulkanVersion, graphicsQueueFamilyIndex, presentQueueFamilyIndex);

    ret->LoadDeviceFunctions();

    return ret;
}

void VulkanDevice::LoadDeviceFunctions() noexcept
{
    LoadDeviceFunc(DestroyDevice);
    LoadDeviceFunc(DeviceWaitIdle);
    LoadDeviceFunc(GetDeviceQueue);
    LoadDeviceFunc(CreateSwapchainKHR);
    LoadDeviceFunc(DestroySwapchainKHR);
    LoadDeviceFunc(GetSwapchainImagesKHR);
    LoadDeviceFunc(CreateImageView);
    LoadDeviceFunc(DestroyImageView);
    LoadDeviceFunc(CreateCommandPool);
    LoadDeviceFunc(DestroyCommandPool);
    LoadDeviceFunc(CreateCommandPool);
    LoadDeviceFunc(ResetCommandPool);
    LoadDeviceFunc(DestroyCommandPool);

    if(m_DeviceVersion <= VK_API_VERSION_1_0)
    {
        LoadDeviceFunc(TrimCommandPoolKHR);
    }

    if(m_DeviceVersion <= VK_API_VERSION_1_1)
    {
        LoadDeviceFunc(TrimCommandPool);
    }

    if(m_DeviceVersion <= VK_API_VERSION_1_2)
    {

    }

    if(m_DeviceVersion <= VK_API_VERSION_1_3)
    {

    }
}

void VulkanDevice::GetDeviceQueue(const uint32_t queueFamilyIndex, const uint32_t queueIndex, VkQueue* const pQueue) const noexcept
{
    if(!VkGetDeviceQueue)
    {
        return;
    }

    VkGetDeviceQueue(m_Device, queueFamilyIndex, queueIndex, pQueue);
}

VkResult VulkanDevice::CreateSwapchainKHR(const VkSwapchainCreateInfoKHR* const pCreateInfo, VkSwapchainKHR* const pSwapchain) const noexcept
{
    if(!VkCreateSwapchainKHR)
    {
        return VK_ERROR_FUNCTION_NOT_FOUND;
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
        return VK_ERROR_FUNCTION_NOT_FOUND;
    }

    return VkGetSwapchainImagesKHR(m_Device, swapchain, pSwapchainImageCount, pSwapchainImages);
}

VkResult VulkanDevice::CreateImageView(const VkImageViewCreateInfo* const pCreateInfo, VkImageView* const pView) const noexcept
{
    if(!VkCreateImageView)
    {
        return VK_ERROR_FUNCTION_NOT_FOUND;
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

VkResult VulkanDevice::CreateCommandPool(const VkCommandPoolCreateInfo* const pCreateInfo, VkCommandPool* const pCommandPool) const noexcept
{
    if(!VkCreateCommandPool)
    {
        return VK_ERROR_FUNCTION_NOT_FOUND;
    }

    return VkCreateCommandPool(m_Device, pCreateInfo, m_Allocator, pCommandPool);
}

void VulkanDevice::TrimCommandPool(const VkCommandPool commandPool, const VkCommandPoolTrimFlags flags) const noexcept
{
    if(VkTrimCommandPool)
    {
        VkTrimCommandPool(m_Device, commandPool, flags);
    }
    else if(VkTrimCommandPoolKHR)
    {
        VkTrimCommandPoolKHR(m_Device, commandPool, flags);
    }
}

VkResult VulkanDevice::ResetCommandPool(const VkCommandPool commandPool, const VkCommandPoolResetFlags flags) const noexcept
{
    if(!VkResetCommandPool)
    {
        return VK_ERROR_FUNCTION_NOT_FOUND;
    }

    return VkResetCommandPool(m_Device, commandPool, flags);
}

void VulkanDevice::DestroyCommandPool(const VkCommandPool commandPool) const noexcept
{
    if(!VkDestroyCommandPool)
    {
        return;
    }

    return VkDestroyCommandPool(m_Device, commandPool, m_Allocator);
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
