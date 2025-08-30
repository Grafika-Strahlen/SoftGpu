/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <Objects.hpp>
#include <Safeties.hpp>
#include <String.hpp>
#include <StringFormat.hpp>

#undef CreateSemaphore

#define VK_ERROR_HANDLER_LABEL_NAME VulkanErrorHandlerBlock_AJ8296L

#define VK_ERROR_HANDLER(Statement) \
    VK_ERROR_HANDLER_LABEL_NAME: \
        Statement

#define VK_ERROR_HANDLER_VOID() VK_ERROR_HANDLER(return)
#define VK_ERROR_HANDLER_VAL(Val) VK_ERROR_HANDLER(return Val)
#define VK_ERROR_HANDLER_NULL() VK_ERROR_HANDLER_VAL(nullptr)
#define VK_ERROR_HANDLER_VK_NULL() VK_ERROR_HANDLER_VAL(VK_NULL_HANDLE)
#define VK_ERROR_HANDLER_0() VK_ERROR_HANDLER_VAL(0)
#define VK_ERROR_HANDLER_N1() VK_ERROR_HANDLER_VAL(-1)
#define VK_ERROR_HANDLER_BRACE() VK_ERROR_HANDLER_VAL({ })

#define VK_CALL(Call, Message) \
    do { \
        const VkResult vkResultForCapturedCall = Call; \
        if(vkResultForCapturedCall != VK_SUCCESS && vkResultForCapturedCall != VK_INCOMPLETE) { \
            ::tau::vd::PrintError(vkResultForCapturedCall, Message); \
            goto VK_ERROR_HANDLER_LABEL_NAME; \
        } \
    }  while(false);

#define VK_CALL_FMT(Call, Fmt, ...) \
    do { \
        const VkResult vkResultForCapturedCall = Call; \
        if(vkResultForCapturedCall != VK_SUCCESS && vkResultForCapturedCall != VK_INCOMPLETE) { \
            ::tau::vd::PrintError(vkResultForCapturedCall, ::Format<char>(Fmt, __VA_ARGS__)); \
            goto VK_ERROR_HANDLER_LABEL_NAME; \
        } \
    }  while(false);

#define VK_CALL0(Call) VK_CALL(Call, #Call)

#define VulkanDeclFunc(Name) PFN_vk##Name Vk##Name

extern VulkanDeclFunc(EnumerateInstanceVersion);

namespace tau::vd {

void LoadNonInstanceFunctions() noexcept;

class VulkanInstance final
{
    DELETE_CM(VulkanInstance);
public:
    VulkanInstance(VkInstance instance, const VkAllocationCallbacks* const allocator, const u32 version) noexcept
        : m_Instance(instance)
        , m_Allocator(allocator)
        , m_Version(version)
    { }

    ~VulkanInstance() noexcept
    {
        vkDestroyInstance(m_Instance, m_Allocator);
    }

    [[nodiscard]] VkInstance Instance() const noexcept { return m_Instance; }
    [[nodiscard]] const VkAllocationCallbacks* Allocator() const noexcept { return m_Allocator; }
    [[nodiscard]] u32 Version() const noexcept { return m_Version; }
public:
    [[nodiscard]] VkResult CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* const pCreateInfo, VkDebugUtilsMessengerEXT* const pMessenger) const noexcept;
    void DestroyDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT messenger) const noexcept;

#ifdef _WIN32
    [[nodiscard]] VkResult CreateWin32SurfaceKHR(const VkWin32SurfaceCreateInfoKHR* const pCreateInfo, VkSurfaceKHR* const pSurface) const noexcept;
#endif
    void DestroySurfaceKHR(VkSurfaceKHR surface) const noexcept;

    [[nodiscard]] VkResult EnumeratePhysicalDevices(uint32_t* const pPhysicalDeviceCount, VkPhysicalDevice* const pPhysicalDevices) const noexcept;
public:
    VulkanDeclFunc(GetDeviceProcAddr) = nullptr;

    VulkanDeclFunc(CreateDebugUtilsMessengerEXT) = nullptr;
    VulkanDeclFunc(DestroyDebugUtilsMessengerEXT) = nullptr;

#ifdef _WIN32
    VulkanDeclFunc(CreateWin32SurfaceKHR) = nullptr;
#endif
    VulkanDeclFunc(DestroySurfaceKHR) = nullptr;

    VulkanDeclFunc(EnumeratePhysicalDevices) = nullptr;

    VulkanDeclFunc(GetPhysicalDeviceProperties2) = nullptr;

    VulkanDeclFunc(GetPhysicalDeviceQueueFamilyProperties2) = nullptr;

    VulkanDeclFunc(GetPhysicalDeviceSurfaceSupportKHR) = nullptr;
    VulkanDeclFunc(GetPhysicalDeviceSurfaceCapabilitiesKHR) = nullptr;
    VulkanDeclFunc(GetPhysicalDeviceSurfaceFormatsKHR) = nullptr;
    VulkanDeclFunc(GetPhysicalDeviceSurfacePresentModesKHR) = nullptr;

    VulkanDeclFunc(GetPhysicalDeviceSurfaceCapabilities2KHR) = nullptr;
    VulkanDeclFunc(GetPhysicalDeviceSurfaceFormats2KHR) = nullptr;

    VulkanDeclFunc(GetPhysicalDeviceMemoryProperties) = nullptr;
    VulkanDeclFunc(GetPhysicalDeviceMemoryProperties2) = nullptr;

    VulkanDeclFunc(CreateDevice) = nullptr;
private:
    VkInstance m_Instance;
    const VkAllocationCallbacks* m_Allocator;
    u32 m_Version;
private:
    void LoadInstanceFunctions() noexcept;
public:
    static StrongRef<VulkanInstance> LoadInstanceFunctions(VkInstance instance, const VkAllocationCallbacks* const allocator, const u32 vulkanVersion) noexcept;
};

class VulkanDevice final
{
    DEFAULT_CM(VulkanDevice);
public:
    VulkanDevice(
        const WeakRef<VulkanInstance>& vulkan, 
        VkDevice device, 
        const VkAllocationCallbacks* const allocator, 
        const u32 deviceVersion,
        const u32 graphicsQueueFamilyIndex,
        const u32 presentQueueFamilyIndex
    ) noexcept
        : m_Vulkan(vulkan)
        , m_Device(device)
        , m_Allocator(allocator)
        , m_VmaAllocator(nullptr)
        , m_DeviceVersion(deviceVersion)
        , m_GraphicsQueue(VK_NULL_HANDLE)
        , m_PresentQueue(VK_NULL_HANDLE)
        , m_GraphicsQueueFamilyIndex(graphicsQueueFamilyIndex)
        , m_PresentQueueFamilyIndex(presentQueueFamilyIndex)
    { }

    ~VulkanDevice() noexcept
    {
        vmaDestroyAllocator(m_VmaAllocator);

        if(VkDeviceWaitIdle)
        {
            VkDeviceWaitIdle(m_Device);
        }

        if(VkDestroyDevice)
        {
            VkDestroyDevice(m_Device, m_Allocator);
        }
    }

    [[nodiscard]] const WeakRef<VulkanInstance>& Vulkan() const noexcept { return m_Vulkan; }
    [[nodiscard]] VkDevice Device() const noexcept { return m_Device; }
    [[nodiscard]] const VkAllocationCallbacks* Allocator() const noexcept { return m_Allocator; }
    [[nodiscard]] ::VmaAllocator& VmaAllocator() noexcept { return m_VmaAllocator; }
    [[nodiscard]] ::VmaAllocator VmaAllocator() const noexcept { return m_VmaAllocator; }
    [[nodiscard]] u32 DeviceVersion() const noexcept { return m_DeviceVersion; }
    [[nodiscard]] VkQueue  GraphicsQueue() const noexcept { return m_GraphicsQueue; }
    [[nodiscard]] VkQueue& GraphicsQueue() noexcept { return m_GraphicsQueue; }
    [[nodiscard]] VkQueue  PresentQueue() const noexcept { return m_PresentQueue; }
    [[nodiscard]] VkQueue& PresentQueue() noexcept { return m_PresentQueue; }
    [[nodiscard]] u32 GraphicsQueueFamilyIndex() const noexcept { return m_GraphicsQueueFamilyIndex; }
    [[nodiscard]] u32 PresentQueueFamilyIndex() const noexcept { return m_PresentQueueFamilyIndex; }

    void GetDeviceQueue(const uint32_t queueFamilyIndex, const uint32_t queueIndex, VkQueue* const pQueue) const noexcept;

    [[nodiscard]] VkResult CreateSwapchainKHR(const VkSwapchainCreateInfoKHR* const pCreateInfo, VkSwapchainKHR* const pSwapchain) const noexcept;
    void DestroySwapchainKHR(VkSwapchainKHR swapchain) const noexcept;
    [[nodiscard]] VkResult GetSwapchainImagesKHR(VkSwapchainKHR swapchain, u32* const pSwapchainImageCount, VkImage* const pSwapchainImages) const noexcept;

    [[nodiscard]] VkResult CreateImageView(const VkImageViewCreateInfo* const pCreateInfo, VkImageView* const pView) const noexcept;
    void DestroyImageView(VkImageView imageView) const noexcept;

    [[nodiscard]] VkResult CreateFence(const VkFenceCreateInfo* const pCreateInfo, VkFence* const pFence) const noexcept;
    void DestroyFence(const VkFence fence) const noexcept;
    [[nodiscard]] VkResult CreateSemaphore(const VkSemaphoreCreateInfo* const pCreateInfo, VkSemaphore* const pSemaphore) const noexcept;
    void DestroySemaphore(const VkSemaphore semaphore) const noexcept;

    [[nodiscard]] VkResult CreateCommandPool(const VkCommandPoolCreateInfo* const pCreateInfo, VkCommandPool* const pCommandPool) const noexcept;
    void TrimCommandPool(const VkCommandPool commandPool, const VkCommandPoolTrimFlags flags) const noexcept;
    [[nodiscard]] VkResult ResetCommandPool(const VkCommandPool commandPool, const VkCommandPoolResetFlags flags) const noexcept;
    void DestroyCommandPool(const VkCommandPool commandPool) const noexcept;

    [[nodiscard]] VkResult AllocateCommandBuffers(const VkCommandBufferAllocateInfo* const pAllocateInfo, VkCommandBuffer* const pCommandBuffers) const noexcept;
    void FreeCommandBuffers(const VkCommandPool commandPool, const u32 commandBufferCount, const VkCommandBuffer* const pCommandBuffers) const noexcept;

    void GetBufferMemoryRequirements(const VkBuffer buffer, VkMemoryRequirements* const pMemoryRequirements) const noexcept;
    void GetImageMemoryRequirements(const VkImage image, VkMemoryRequirements* const pMemoryRequirements) const noexcept;

    void GetBufferMemoryRequirements2(const VkBufferMemoryRequirementsInfo2* const pInfo, VkMemoryRequirements2* const pMemoryRequirements) const noexcept;
    void GetImageMemoryRequirements2(const VkImageMemoryRequirementsInfo2* const pInfo, VkMemoryRequirements2* const pMemoryRequirements) const noexcept;
private:
    void LoadDeviceFunctions() noexcept;
private:
    WeakRef<VulkanInstance> m_Vulkan;
    VkDevice m_Device;
    const VkAllocationCallbacks* m_Allocator;
    ::VmaAllocator m_VmaAllocator;
    u32 m_DeviceVersion;
    VkQueue m_GraphicsQueue;
    VkQueue m_PresentQueue;
    u32 m_GraphicsQueueFamilyIndex;
    u32 m_PresentQueueFamilyIndex;
public:
    VulkanDeclFunc(DestroyDevice) = nullptr;
    VulkanDeclFunc(DeviceWaitIdle) = nullptr;

    VulkanDeclFunc(GetDeviceQueue) = nullptr;
    VulkanDeclFunc(QueueSubmit) = nullptr;
    VulkanDeclFunc(QueueSubmit2) = nullptr;
    VulkanDeclFunc(QueueWaitIdle) = nullptr;

    VulkanDeclFunc(CreateSwapchainKHR) = nullptr;
    VulkanDeclFunc(DestroySwapchainKHR) = nullptr;
    VulkanDeclFunc(GetSwapchainImagesKHR) = nullptr;
    VulkanDeclFunc(AcquireNextImageKHR) = nullptr;
    VulkanDeclFunc(QueuePresentKHR) = nullptr;

    VulkanDeclFunc(CreateImageView) = nullptr;
    VulkanDeclFunc(DestroyImageView) = nullptr;

    VulkanDeclFunc(CreateFence) = nullptr;
    VulkanDeclFunc(DestroyFence) = nullptr;
    VulkanDeclFunc(WaitForFences) = nullptr;
    VulkanDeclFunc(ResetFences) = nullptr;

    VulkanDeclFunc(CreateSemaphore) = nullptr;
    VulkanDeclFunc(DestroySemaphore) = nullptr;

    VulkanDeclFunc(CreateCommandPool) = nullptr;
    VulkanDeclFunc(TrimCommandPool) = nullptr;
    VulkanDeclFunc(ResetCommandPool) = nullptr;
    VulkanDeclFunc(DestroyCommandPool) = nullptr;

    VulkanDeclFunc(AllocateCommandBuffers) = nullptr;
    VulkanDeclFunc(ResetCommandBuffer) = nullptr;
    VulkanDeclFunc(FreeCommandBuffers) = nullptr;
    VulkanDeclFunc(BeginCommandBuffer) = nullptr;
    VulkanDeclFunc(EndCommandBuffer) = nullptr;

    VulkanDeclFunc(CmdCopyBufferToImage) = nullptr;
    VulkanDeclFunc(CmdPipelineBarrier) = nullptr;

    VulkanDeclFunc(GetBufferMemoryRequirements) = nullptr;
    VulkanDeclFunc(GetImageMemoryRequirements) = nullptr;
    VulkanDeclFunc(GetBufferMemoryRequirements2) = nullptr;
    VulkanDeclFunc(GetImageMemoryRequirements2) = nullptr;
public:
    [[nodiscard]] static StrongRef<VulkanDevice> LoadDeviceFunctions(
        const WeakRef<VulkanInstance>& vulkan, 
        VkDevice device, 
        const u32 deviceVulkanVersion, 
        const u32 graphicsQueueFamilyIndex, 
        const u32 presentQueueFamilyIndex, 
        const VkAllocationCallbacks* const allocator = nullptr
    ) noexcept;
};

void PrintError(const VkResult result, const DynString& source) noexcept;

}
