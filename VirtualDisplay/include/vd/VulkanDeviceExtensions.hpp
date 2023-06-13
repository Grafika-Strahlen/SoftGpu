#pragma once

#include <DynArray.hpp>
#include <String.hpp>
#include <vulkan/vulkan.h>

namespace tau::vd {

static constexpr ConstExprString RequiredDeviceExtensions[] = {
#if defined(VK_KHR_swapchain) && VK_KHR_swapchain
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#endif
};

[[nodiscard]] DynArray<const char*> GetRequestedDeviceExtensions(VkPhysicalDevice physicalDevice, const u32 deviceVulkanVersion, u32* const extensionCount) noexcept;

}
