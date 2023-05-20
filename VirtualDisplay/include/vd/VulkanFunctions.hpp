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
    { }

    ~VulkanDevice() noexcept
    {
        vkDestroyDevice(m_Device, m_Allocator);
    }

    [[nodiscard]] VkDevice Device() const noexcept { return m_Device; }
private:
    VkDevice m_Device;
    const VkAllocationCallbacks* m_Allocator;
public:
    [[nodiscard]] static ReferenceCountingPointer<VulkanDevice> LoadDeviceFunctions(VkDevice device, const u32 vulkanVersion, const VkAllocationCallbacks* const allocator = nullptr) noexcept;
};

}
