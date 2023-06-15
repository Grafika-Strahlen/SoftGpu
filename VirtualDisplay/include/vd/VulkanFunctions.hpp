#pragma once

#include <vulkan/vulkan.h>

#include <Objects.hpp>
#include <ReferenceCountingPointer.hpp>
#include <String.hpp>
#include <StringFormat.hpp>

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

extern PFN_vkEnumerateInstanceVersion VkEnumerateInstanceVersion;

namespace tau::vd {

void LoadNonInstanceFunctions() noexcept;

class VulkanInstance;

using VulkanInstanceRef = ReferenceCountingPointer<VulkanInstance>;

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
    VkResult CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* const pCreateInfo, VkDebugUtilsMessengerEXT* const pMessenger) const noexcept;
    void DestroyDebugUtilsMessengerEXT(VkDebugUtilsMessengerEXT messenger) const noexcept;

    VkResult CreateWin32SurfaceKHR(const VkWin32SurfaceCreateInfoKHR* const pCreateInfo, VkSurfaceKHR* const pSurface) const noexcept;
    void DestroySurfaceKHR(VkSurfaceKHR surface) const noexcept;

    VkResult EnumeratePhysicalDevices(uint32_t* const pPhysicalDeviceCount, VkPhysicalDevice* const pPhysicalDevices) const noexcept;
public:
    PFN_vkCreateDebugUtilsMessengerEXT VkCreateDebugUtilsMessengerEXT = nullptr;
    PFN_vkDestroyDebugUtilsMessengerEXT VkDestroyDebugUtilsMessengerEXT = nullptr;

    PFN_vkCreateWin32SurfaceKHR VkCreateWin32SurfaceKHR = nullptr;
    PFN_vkDestroySurfaceKHR VkDestroySurfaceKHR = nullptr;

    PFN_vkEnumeratePhysicalDevices VkEnumeratePhysicalDevices = nullptr;

    PFN_vkGetPhysicalDeviceProperties2 VkGetPhysicalDeviceProperties2 = nullptr;
    PFN_vkGetPhysicalDeviceProperties2KHR VkGetPhysicalDeviceProperties2KHR = nullptr;

    PFN_vkGetPhysicalDeviceQueueFamilyProperties2 VkGetPhysicalDeviceQueueFamilyProperties2 = nullptr;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR VkGetPhysicalDeviceQueueFamilyProperties2KHR = nullptr;

    PFN_vkGetPhysicalDeviceSurfaceSupportKHR VkGetPhysicalDeviceSurfaceSupportKHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR VkGetPhysicalDeviceSurfaceCapabilitiesKHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR VkGetPhysicalDeviceSurfaceFormatsKHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR VkGetPhysicalDeviceSurfacePresentModesKHR = nullptr;

    PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR VkGetPhysicalDeviceSurfaceCapabilities2KHR = nullptr;
    PFN_vkGetPhysicalDeviceSurfaceFormats2KHR VkGetPhysicalDeviceSurfaceFormats2KHR = nullptr;
private:
    VkInstance m_Instance;
    const VkAllocationCallbacks* m_Allocator;
    u32 m_Version;
private:
    void LoadInstanceFunctions() noexcept;
public:
    static VulkanInstanceRef LoadInstanceFunctions(VkInstance instance, const VkAllocationCallbacks* const allocator, const u32 vulkanVersion) noexcept;
};

class VulkanDevice;

using VulkanDeviceRef = ReferenceCountingPointer<VulkanDevice>;

class VulkanDevice final
{
    DEFAULT_CM(VulkanDevice);
public:
    VulkanDevice(VkDevice device, const VkAllocationCallbacks* const allocator, const u32 deviceVersion) noexcept
        : m_Device(device)
        , m_Allocator(allocator)
        , m_DeviceVersion(deviceVersion)
        , m_GraphicsQueue(VK_NULL_HANDLE)
        , m_PresentQueue(VK_NULL_HANDLE)
    { }

    ~VulkanDevice() noexcept
    {
        vkDestroyDevice(m_Device, m_Allocator);
    }

    [[nodiscard]] VkDevice Device() const noexcept { return m_Device; }
    [[nodiscard]] const VkAllocationCallbacks* Allocator() const noexcept { return m_Allocator; }
    [[nodiscard]] u32 DeviceVersion() const noexcept { return m_DeviceVersion; }
    [[nodiscard]] VkQueue  GraphicsQueue() const noexcept { return m_GraphicsQueue; }
    [[nodiscard]] VkQueue& GraphicsQueue() noexcept { return m_GraphicsQueue; }
    [[nodiscard]] VkQueue  PresentQueue() const noexcept { return m_PresentQueue; }
    [[nodiscard]] VkQueue& PresentQueue() noexcept { return m_PresentQueue; }

    VkResult CreateSwapchainKHR(const VkSwapchainCreateInfoKHR* const pCreateInfo, VkSwapchainKHR* const pSwapchain) const noexcept;
    void DestroySwapchainKHR(VkSwapchainKHR swapchain) const noexcept;
    VkResult GetSwapchainImagesKHR(VkSwapchainKHR swapchain, u32* const pSwapchainImageCount, VkImage* const pSwapchainImages) const noexcept;
    VkResult CreateImageView(const VkImageViewCreateInfo* const pCreateInfo, VkImageView* const pView) const noexcept;
    void DestroyImageView(VkImageView imageView) const noexcept;
private:
    void LoadDeviceFunctions() noexcept;
private:
    VkDevice m_Device;
    const VkAllocationCallbacks* m_Allocator;
    u32 m_DeviceVersion;
    VkQueue m_GraphicsQueue;
    VkQueue m_PresentQueue;
public:
    PFN_vkCreateSwapchainKHR VkCreateSwapchainKHR = nullptr;
    PFN_vkDestroySwapchainKHR VkDestroySwapchainKHR = nullptr;
    PFN_vkGetSwapchainImagesKHR VkGetSwapchainImagesKHR = nullptr;
    PFN_vkCreateImageView VkCreateImageView = nullptr;
    PFN_vkDestroyImageView VkDestroyImageView = nullptr;
public:
    [[nodiscard]] static VulkanDeviceRef LoadDeviceFunctions(VkDevice device, const u32 deviceVulkanVersion, const VkAllocationCallbacks* const allocator = nullptr) noexcept;
};

void PrintError(const VkResult result, const DynString& source) noexcept;

}
