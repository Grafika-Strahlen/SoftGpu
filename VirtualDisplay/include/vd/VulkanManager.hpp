#pragma once

#include <vulkan/vulkan.h>
#include <ReferenceCountingPointer.hpp>

namespace tau::vd {

class VulkanManager final
{
public:
    VulkanManager(
        VkInstance vulkan, 
        const u32 vulkanVersion, 
        VkDebugUtilsMessengerEXT debugMessenger, 
        VkPhysicalDevice physicalDevice
    ) noexcept
        : m_Vulkan(vulkan)
        , m_VulkanVersion(vulkanVersion)
        , m_DebugMessenger(debugMessenger)
        , m_PhysicalDevice(physicalDevice)
    { }

    ~VulkanManager() noexcept;
public:
    static ReferenceCountingPointer<VulkanManager> CreateVulkanManager() noexcept;
private:
    VkInstance m_Vulkan;
    u32 m_VulkanVersion;
    VkDebugUtilsMessengerEXT m_DebugMessenger;
    VkPhysicalDevice m_PhysicalDevice;
};

}
