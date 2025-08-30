/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#include "vd/VulkanFunctions.hpp"
#ifdef _WIN32
#include <vulkan/vk_enum_string_helper.h>
#endif
#include <SDL3/SDL_vulkan.h>
#include <ConPrinter.hpp>

#include "vd/MemoryRecovery.hpp"

#define VK_ERROR_FUNCTION_NOT_FOUND (VK_ERROR_UNKNOWN)

#define LoadFunc(BaseName, Loader, LoaderParam) Vk##BaseName = reinterpret_cast<PFN_vk##BaseName>(Loader((LoaderParam), "vk" #BaseName))

#define LoadNonInstanceFunc(BaseName) LoadFunc(BaseName, GetInstanceProcAddr, nullptr)
#define LoadInstanceFunc(BaseName) LoadFunc(BaseName, GetInstanceProcAddr, m_Instance)
#define LoadDeviceFunc(BaseName) LoadFunc(BaseName, m_Vulkan->VkGetDeviceProcAddr, m_Device)

#define LoadFunc2(BaseName0, BaseName1, Loader, LoaderParam) \
    do { \
        LoadFunc(BaseName0, Loader, LoaderParam); \
        if(!Vk##BaseName0) { \
            const auto LoadFunc(BaseName1, Loader, LoaderParam); \
            Vk##BaseName0 = Vk##BaseName1; \
        } \
    } while(false)

#define LoadNonInstanceFunc2(BaseName0, BaseName1) LoadFunc2(BaseName0, BaseName1, GetInstanceProcAddr, nullptr)
#define LoadInstanceFunc2(BaseName0, BaseName1) LoadFunc2(BaseName0, BaseName1, GetInstanceProcAddr, m_Instance)
#define LoadDeviceFunc2(BaseName0, BaseName1) LoadFunc2(BaseName0, BaseName1, m_Vulkan->VkGetDeviceProcAddr, m_Device)

VulkanDeclFunc(EnumerateInstanceVersion);

namespace tau::vd {

static PFN_vkVoidFunction GetInstanceProcAddr(
    VkInstance instance,
    const char* pName)
{
    static PFN_vkGetInstanceProcAddr getInstanceProcAddr = nullptr;

    if(!getInstanceProcAddr)
    {
        getInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
    }

    if(!getInstanceProcAddr)
    {
        return nullptr;
    }

    return getInstanceProcAddr(instance, pName);
}

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
    LoadInstanceFunc(GetDeviceProcAddr);

    LoadInstanceFunc(CreateDebugUtilsMessengerEXT);
    LoadInstanceFunc(DestroyDebugUtilsMessengerEXT);

#ifdef _WIN32
    LoadInstanceFunc(CreateWin32SurfaceKHR);
#endif
    LoadInstanceFunc(DestroySurfaceKHR);

    LoadInstanceFunc(EnumeratePhysicalDevices);

    LoadInstanceFunc(GetPhysicalDeviceSurfaceSupportKHR);
    LoadInstanceFunc(GetPhysicalDeviceSurfaceCapabilitiesKHR);
    LoadInstanceFunc(GetPhysicalDeviceSurfaceFormatsKHR);
    LoadInstanceFunc(GetPhysicalDeviceSurfacePresentModesKHR);

    LoadInstanceFunc(GetPhysicalDeviceSurfaceCapabilities2KHR);
    LoadInstanceFunc(GetPhysicalDeviceSurfaceFormats2KHR);

    LoadInstanceFunc(GetPhysicalDeviceMemoryProperties);

    LoadInstanceFunc(CreateDevice);

    LoadInstanceFunc2(GetPhysicalDeviceProperties2, GetPhysicalDeviceProperties2KHR);
    LoadInstanceFunc2(GetPhysicalDeviceQueueFamilyProperties2, GetPhysicalDeviceQueueFamilyProperties2KHR);
    LoadInstanceFunc2(GetPhysicalDeviceMemoryProperties2, GetPhysicalDeviceMemoryProperties2KHR);

    if(m_Version < VK_API_VERSION_1_1)
    {
    }
    else
    {
    }

    if(m_Version < VK_API_VERSION_1_2)
    {

    }
    else
    {
        
    }

    if(m_Version < VK_API_VERSION_1_3)
    {

    }
    else
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

#ifdef _WIN32
VkResult VulkanInstance::CreateWin32SurfaceKHR(const VkWin32SurfaceCreateInfoKHR* const pCreateInfo, VkSurfaceKHR* const pSurface) const noexcept
{
    if(!VkCreateWin32SurfaceKHR)
    {
        return VK_ERROR_FUNCTION_NOT_FOUND;
    }

    return VkCreateWin32SurfaceKHR(m_Instance, pCreateInfo, m_Allocator, pSurface);
}
#endif

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
    LoadDeviceFunc(QueueSubmit);
    LoadDeviceFunc2(QueueSubmit2, QueueSubmit2KHR);
    LoadDeviceFunc(QueueWaitIdle);

    LoadDeviceFunc(CreateSwapchainKHR);
    LoadDeviceFunc(DestroySwapchainKHR);
    LoadDeviceFunc(GetSwapchainImagesKHR);
    LoadDeviceFunc(AcquireNextImageKHR);
    LoadDeviceFunc(QueuePresentKHR);

    LoadDeviceFunc(CreateImageView);
    LoadDeviceFunc(DestroyImageView);

    LoadDeviceFunc(CreateFence);
    LoadDeviceFunc(DestroyFence);
    LoadDeviceFunc(WaitForFences);
    LoadDeviceFunc(ResetFences);

    LoadDeviceFunc(CreateSemaphore);
    LoadDeviceFunc(DestroySemaphore);

    LoadDeviceFunc(CreateCommandPool);
    LoadDeviceFunc(ResetCommandPool);
    LoadDeviceFunc(DestroyCommandPool);

    LoadDeviceFunc(AllocateCommandBuffers);
    LoadDeviceFunc(ResetCommandBuffer);
    LoadDeviceFunc(FreeCommandBuffers);
    LoadDeviceFunc(BeginCommandBuffer);
    LoadDeviceFunc(EndCommandBuffer);

    LoadDeviceFunc2(TrimCommandPool, TrimCommandPoolKHR);

    LoadDeviceFunc(CmdCopyBufferToImage);
    LoadDeviceFunc(CmdPipelineBarrier);

    LoadDeviceFunc(GetBufferMemoryRequirements);
    LoadDeviceFunc(GetImageMemoryRequirements);

    LoadDeviceFunc2(GetBufferMemoryRequirements2, GetBufferMemoryRequirements2KHR);
    LoadDeviceFunc2(GetImageMemoryRequirements2, GetImageMemoryRequirements2KHR);

    if(m_DeviceVersion < VK_API_VERSION_1_1)
    {
    }
    else
    {
    }

    if(m_DeviceVersion < VK_API_VERSION_1_2)
    {

    }
    else
    {
        
    }

    if(m_DeviceVersion < VK_API_VERSION_1_3)
    {

    }
    else
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

VkResult VulkanDevice::CreateFence(const VkFenceCreateInfo* const pCreateInfo, VkFence* const pFence) const noexcept
{
    if(!VkCreateFence)
    {
        return VK_ERROR_FUNCTION_NOT_FOUND;
    }

    return VkCreateFence(m_Device, pCreateInfo, m_Allocator, pFence);
}

void VulkanDevice::DestroyFence(const VkFence fence) const noexcept
{
    if(!VkDestroyFence)
    {
        return;
    }

    VkDestroyFence(m_Device, fence, m_Allocator);
}

VkResult VulkanDevice::CreateSemaphore(const VkSemaphoreCreateInfo* const pCreateInfo, VkSemaphore* const pSemaphore) const noexcept
{
    if(!VkCreateSemaphore)
    {
        return VK_ERROR_FUNCTION_NOT_FOUND;
    }

    return VkCreateSemaphore(m_Device, pCreateInfo, m_Allocator, pSemaphore);
}

void VulkanDevice::DestroySemaphore(const VkSemaphore semaphore) const noexcept
{
    if(!VkDestroySemaphore)
    {
        return;
    }

    VkDestroySemaphore(m_Device, semaphore, m_Allocator);
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
    if(!VkTrimCommandPool)
    {
        return;
    }

    VkTrimCommandPool(m_Device, commandPool, flags);
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

    VkDestroyCommandPool(m_Device, commandPool, m_Allocator);
}

VkResult VulkanDevice::AllocateCommandBuffers(const VkCommandBufferAllocateInfo* const pAllocateInfo, VkCommandBuffer* const pCommandBuffers) const noexcept
{
    if(!VkAllocateCommandBuffers)
    {
        return VK_ERROR_FUNCTION_NOT_FOUND;
    }

    return VkAllocateCommandBuffers(m_Device, pAllocateInfo, pCommandBuffers);
}

void VulkanDevice::FreeCommandBuffers(const VkCommandPool commandPool, const u32 commandBufferCount, const VkCommandBuffer* const pCommandBuffers) const noexcept
{
    if(!VkFreeCommandBuffers)
    {
        return;
    }

    VkFreeCommandBuffers(m_Device, commandPool, commandBufferCount, pCommandBuffers);
}

void VulkanDevice::GetBufferMemoryRequirements(const VkBuffer buffer, VkMemoryRequirements* const pMemoryRequirements) const noexcept
{
    if(!VkGetBufferMemoryRequirements)
    {
        return;
    }

    VkGetBufferMemoryRequirements(m_Device, buffer, pMemoryRequirements);
}

void VulkanDevice::GetImageMemoryRequirements(const VkImage image, VkMemoryRequirements* const pMemoryRequirements) const noexcept
{
    if(!VkGetImageMemoryRequirements)
    {
        return;
    }

    VkGetImageMemoryRequirements(m_Device, image, pMemoryRequirements);
}

void VulkanDevice::GetBufferMemoryRequirements2(const VkBufferMemoryRequirementsInfo2* const pInfo, VkMemoryRequirements2* const pMemoryRequirements) const noexcept
{
    if(!VkGetBufferMemoryRequirements2)
    {
        return;
    }

    VkGetBufferMemoryRequirements2(m_Device, pInfo, pMemoryRequirements);
}

void VulkanDevice::GetImageMemoryRequirements2(const VkImageMemoryRequirementsInfo2* const pInfo, VkMemoryRequirements2* const pMemoryRequirements) const noexcept
{
    if(!VkGetImageMemoryRequirements2)
    {
        return;
    }

    VkGetImageMemoryRequirements2(m_Device, pInfo, pMemoryRequirements);
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

#ifdef _WIN32
    ConPrinter::PrintLn(u8"[{}]: {} {}\n", string_VkResult(result), message, source);
#else
    ConPrinter::PrintLn(u8"[{}]: {} {}\n", static_cast<int>(result), message, source);
#endif
}

}
