#pragma once

#include <DynArray.hpp>
#include <vulkan/vulkan.h>
#include <ReferenceCountingPointer.hpp>

namespace tau::vd {

class VulkanInstance;
class VulkanDevice;
class Window;

class VulkanManager final
{
    DELETE_CM(VulkanManager);
public:
    VulkanManager(
        ReferenceCountingPointer<VulkanInstance>&& vulkan,
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
    ) noexcept;

    ~VulkanManager() noexcept;
public:
    static ReferenceCountingPointer<VulkanManager> CreateVulkanManager(const ReferenceCountingPointer<Window>& window) noexcept;
private:
    ReferenceCountingPointer<VulkanInstance> m_Vulkan;
    VkDebugUtilsMessengerEXT m_DebugMessenger;
    ReferenceCountingPointer<Window> m_Window;
    VkSurfaceKHR m_Surface;
    VkPhysicalDevice m_PhysicalDevice;
    ReferenceCountingPointer<VulkanDevice> m_Device;
    VkSwapchainKHR m_Swapchain;
    DynArray<VkImage> m_SwapchainImages;
    DynArray<VkImageView> m_SwapchainImageViews;
    VkExtent2D m_SwapchainSize;
    VkFormat m_SwapchainImageFormat;
};

}
