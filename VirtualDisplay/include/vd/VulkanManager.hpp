#pragma once

#include <DynArray.hpp>
#include <vulkan/vulkan.h>
#include <ReferenceCountingPointer.hpp>
#include <Safeties.hpp>

namespace tau::vd {

class VulkanInstance;
class VulkanDevice;
class Window;

class VulkanManager final
{
    DELETE_CM(VulkanManager);
public:
    VulkanManager(
        StrongRef<VulkanInstance>&& vulkan,
        VkDebugUtilsMessengerEXT debugMessenger,
        const ReferenceCountingPointer<Window>& window,
        VkSurfaceKHR surface,
        VkPhysicalDevice physicalDevice,
        StrongRef<VulkanDevice>&& device,
        VkSwapchainKHR swapchain,
        const DynArray<VkImage>& swapchainImages,
        const DynArray<VkImageView>& swapchainImageViews,
        VkExtent2D swapchainSize,
        VkFormat swapchainImageFormat
    ) noexcept;

    ~VulkanManager() noexcept;

    [[nodiscard]] StrongRef<VulkanDevice> Device() const noexcept { return m_Device; }
public:
    static Ref<VulkanManager> CreateVulkanManager(const Ref<Window>& window) noexcept;
private:
    StrongRef<VulkanInstance> m_Vulkan;
    VkDebugUtilsMessengerEXT m_DebugMessenger;
    Ref<Window> m_Window;
    VkSurfaceKHR m_Surface;
    VkPhysicalDevice m_PhysicalDevice;
    StrongRef<VulkanDevice> m_Device;
    VkSwapchainKHR m_Swapchain;
    DynArray<VkImage> m_SwapchainImages;
    DynArray<VkImageView> m_SwapchainImageViews;
    VkExtent2D m_SwapchainSize;
    VkFormat m_SwapchainImageFormat;
};

}
