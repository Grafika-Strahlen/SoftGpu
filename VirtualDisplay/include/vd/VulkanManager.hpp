/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <DynArray.hpp>
#include <vulkan/vulkan.h>
#include <ReferenceCountingPointer.hpp>
#include <Safeties.hpp>

namespace tau::vd {

class VulkanInstance;
class VulkanDevice;
class VulkanCommandPools;
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
        DynArray<VkImage>&& swapchainImages,
        DynArray<VkImageView>&& swapchainImageViews,
        DynArray<VkPresentModeKHR>&& presentModes,
        VkExtent2D swapchainSize,
        VkFormat swapchainImageFormat,
        const VkSurfaceFormatKHR& surfaceFormat,
        const VkSurfaceCapabilitiesKHR& surfaceCapabilities,
        VkFence frameFence,
        VkSemaphore imageAvailableSemaphore,
        VkSemaphore renderFinishedSemaphore
    ) noexcept;

    ~VulkanManager() noexcept;

    void TransitionSwapchain(Ref<VulkanCommandPools>& commandPools) noexcept;

    u32 WaitForFrame() noexcept;
    void SubmitCommandBuffers(const u32 commandBufferCount, const VkCommandBuffer* const commandBuffers) noexcept;
    void Present(const u32 frameIndex) noexcept;

    void RebuildSwapchain() noexcept;

    [[nodiscard]] const StrongRef<VulkanDevice>& Device() const noexcept { return m_Device; }
    [[nodiscard]] const DynArray<VkImage>& SwapchainImages() const noexcept { return m_SwapchainImages; }
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
    DynArray<VkPresentModeKHR> m_PresentModes;
    VkExtent2D m_SwapchainSize;
    VkFormat m_SwapchainImageFormat;
    VkSurfaceFormatKHR m_SurfaceFormat;
    VkSurfaceCapabilitiesKHR m_SurfaceCapabilities;
    VkFence m_FrameFence;
    VkSemaphore m_ImageAvailableSemaphore;
    VkSemaphore m_RenderFinishedSemaphore;
};

}
