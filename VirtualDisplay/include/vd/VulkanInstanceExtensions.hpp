#pragma once

#include <DynArray.hpp>
#include <String.hpp>
#include <vulkan/vulkan.h>

namespace tau::vd {

static constexpr ConstExprString RequiredInstanceExtensions[] = {
#if defined(VK_KHR_surface) && VK_KHR_surface
    VK_KHR_SURFACE_EXTENSION_NAME,
#endif
#if defined(VK_KHR_win32_surface) && VK_KHR_win32_surface
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
};

[[nodiscard]] DynArray<const char*> GetRequestedInstanceExtensions(const u32 vulkanVersion, u32* const extensionCount, bool* hasDebugExt) noexcept;

}
