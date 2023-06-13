#pragma once

#include <vulkan/vulkan.h>

#include <Objects.hpp>
#include <ReferenceCountingPointer.hpp>

extern PFN_vkEnumerateInstanceVersion VkEnumerateInstanceVersion;

namespace tau::vd {

void LoadNonInstanceFunctions() noexcept;

struct InstanceFunctions final
{
    DELETE_CONSTRUCT(InstanceFunctions);
    DELETE_DESTRUCT(InstanceFunctions);
    DELETE_CM(InstanceFunctions);
public:
    static PFN_vkCreateDebugUtilsMessengerEXT VkCreateDebugUtilsMessengerEXT;
    static PFN_vkDestroyDebugUtilsMessengerEXT VkDestroyDebugUtilsMessengerEXT;

    static PFN_vkGetPhysicalDeviceProperties2 VkGetPhysicalDeviceProperties2;
    static PFN_vkGetPhysicalDeviceProperties2KHR VkGetPhysicalDeviceProperties2KHR;

    static PFN_vkGetPhysicalDeviceQueueFamilyProperties2 VkGetPhysicalDeviceQueueFamilyProperties2;
    static PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR VkGetPhysicalDeviceQueueFamilyProperties2KHR;

    static PFN_vkGetPhysicalDeviceSurfaceSupportKHR VkGetPhysicalDeviceSurfaceSupportKHR;
    static PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR VkGetPhysicalDeviceSurfaceCapabilitiesKHR;
    static PFN_vkGetPhysicalDeviceSurfaceFormatsKHR VkGetPhysicalDeviceSurfaceFormatsKHR;
    static PFN_vkGetPhysicalDeviceSurfacePresentModesKHR VkGetPhysicalDeviceSurfacePresentModesKHR;

    static PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR VkGetPhysicalDeviceSurfaceCapabilities2KHR;
    static PFN_vkGetPhysicalDeviceSurfaceFormats2KHR VkGetPhysicalDeviceSurfaceFormats2KHR;
public:
    static void LoadInstanceFunctions(VkInstance instance, const u32 vulkanVersion) noexcept;
};

class VulkanDevice final
{
    DEFAULT_CM(VulkanDevice);
public:
    VulkanDevice(VkDevice device, const VkAllocationCallbacks* const allocator) noexcept
        : m_Device(device)
        , m_Allocator(allocator)
        , m_GraphicsQueue(VK_NULL_HANDLE)
        , m_PresentQueue(VK_NULL_HANDLE)
        , VkCreateSwapchainKHR(nullptr)
    { }

    ~VulkanDevice() noexcept
    {
        vkDestroyDevice(m_Device, m_Allocator);
    }

    [[nodiscard]] VkDevice Device() const noexcept { return m_Device; }
    [[nodiscard]] VkQueue  GraphicsQueue() const noexcept { return m_GraphicsQueue; }
    [[nodiscard]] VkQueue& GraphicsQueue() noexcept { return m_GraphicsQueue; }
    [[nodiscard]] VkQueue  PresentQueue() const noexcept { return m_PresentQueue; }
    [[nodiscard]] VkQueue& PresentQueue() noexcept { return m_PresentQueue; }

    VkResult CreateSwapchainKHR(const VkSwapchainCreateInfoKHR* const pCreateInfo, VkSwapchainKHR* const pSwapchain) noexcept;
    void DestroySwapchainKHR(VkSwapchainKHR swapchain) noexcept;
    VkResult GetSwapchainImagesKHR(VkSwapchainKHR swapchain, u32* const pSwapchainImageCount, VkImage* const pSwapchainImages) noexcept;
    VkResult CreateImageView(const VkImageViewCreateInfo* const pCreateInfo, VkImageView* const pView) noexcept;
    void DestroyImageView(VkImageView imageView) noexcept;
private:
    void LoadDeviceFunctions(const u32 deviceVulkanVersion) noexcept;
private:
    VkDevice m_Device;
    const VkAllocationCallbacks* m_Allocator;
    VkQueue m_GraphicsQueue;
    VkQueue m_PresentQueue;
public:
    PFN_vkCreateSwapchainKHR VkCreateSwapchainKHR;
    PFN_vkDestroySwapchainKHR VkDestroySwapchainKHR;
    PFN_vkGetSwapchainImagesKHR VkGetSwapchainImagesKHR;
    PFN_vkCreateImageView VkCreateImageView;
    PFN_vkDestroyImageView VkDestroyImageView;
public:
    [[nodiscard]] static ReferenceCountingPointer<VulkanDevice> LoadDeviceFunctions(VkDevice device, const u32 deviceVulkanVersion, const VkAllocationCallbacks* const allocator = nullptr) noexcept;
};

}
