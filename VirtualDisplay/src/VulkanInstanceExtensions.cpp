#include "vd/VulkanInstanceExtensions.hpp"
#include "vd/MemoryRecovery.hpp"
#include "vd/Config.hpp"
#include <vulkan/vulkan.h>
#include <String.hpp>
#include <ConPrinter.hpp>

namespace tau::vd {

#if defined(VK_EXT_debug_utils) && VK_EXT_debug_utils
static constexpr ConstExprString VkExtDebugUtilsName(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

static constexpr ConstExprString DesiredInstanceExtensions[] = {
#if defined(VK_EXT_debug_utils) && VK_EXT_debug_utils
    VkExtDebugUtilsName,
#endif
#if defined(VK_KHR_get_surface_capabilities2) && VK_KHR_get_surface_capabilities2
    VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
#endif
};

static constexpr ConstExprString Desired1_0InstanceExtensions[] = {
#if defined(VK_KHR_get_physical_device_properties2) && VK_KHR_get_physical_device_properties2 && !VD_FORCE_DISABLE_VK_KHR_get_physical_device_properties2
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
#endif
};

static constexpr ConstExprString Desired1_1InstanceExtensions[] = {
};

static constexpr ConstExprString Desired1_2InstanceExtensions[] = {
};

static constexpr ConstExprString Desired1_3InstanceExtensions[] = {
};

[[nodiscard]] static DynArray<VkExtensionProperties> GetInstanceExtensions() noexcept
{
    u32 propertyCount;
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, nullptr);

    if(result == VK_ERROR_OUT_OF_HOST_MEMORY)
    {
        tau::vd::RecoverSacrificialMemory();
        ConPrinter::PrintLn("Ran out of system memory while querying number of instance extensions.");
        return { };
    }

    if(result == VK_ERROR_OUT_OF_DEVICE_MEMORY)
    {
        ConPrinter::PrintLn("Ran out of device memory while querying number of instance extensions.");
        return { };
    }

    if(result != VK_SUCCESS && result != VK_INCOMPLETE)
    {
        ConPrinter::PrintLn("Encountered undefined error while querying number of instance extensions: {}.", result);
        return { };
    }

    do
    {
        DynArray<VkExtensionProperties> extensions(propertyCount);

        result = vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, extensions);

        if(result == VK_ERROR_OUT_OF_HOST_MEMORY)
        {
            tau::vd::RecoverSacrificialMemory();
            ConPrinter::PrintLn("Ran out of system memory while querying instance extensions.");
            return { };
        }

        if(result == VK_ERROR_OUT_OF_DEVICE_MEMORY)
        {
            ConPrinter::PrintLn("Ran out of device memory while querying instance extensions.");
            return { };
        }

        if(result == VK_SUCCESS)
        {
            return extensions;
        }
    } while(result == VK_INCOMPLETE);

    ConPrinter::PrintLn("Encountered undefined error while querying instance extensions: 0x{XP0}.", result);
    return { };
}

DynArray<const char*> GetRequestedInstanceExtensions(const u32 vulkanVersion, u32* const extensionCount, bool* hasDebugExt) noexcept
{
    *extensionCount = 0;
    *hasDebugExt = false;
    DynArray<VkExtensionProperties> instanceExtensions = GetInstanceExtensions();

    if(instanceExtensions.Length() == 0)
    {
        return { };
    }

    for(const ConstExprString& extension : RequiredInstanceExtensions)
    {
        bool found = false;

        for(const VkExtensionProperties& availableExtension : instanceExtensions)
        {
            if(extension == availableExtension.extensionName)
            {
                found = true;
                break;
            }
        }

        // We failed to find a required extension.
        if(!found)
        {
            ConPrinter::PrintLn("Failed to find required instance extension {}.", extension);
            return { };
        }
    }

    DynArray<const char*> enabledInstanceExtensions(instanceExtensions.Length());

    uSys insertIndex = 0;

    for(const ConstExprString& extension : RequiredInstanceExtensions)
    {
        enabledInstanceExtensions[insertIndex++] = extension;
    }

    for(const ConstExprString& extension : DesiredInstanceExtensions)
    {
        for(const VkExtensionProperties& availableExtension : instanceExtensions)
        {
            if(extension == availableExtension.extensionName)
            {
#if defined(VK_EXT_debug_utils) && VK_EXT_debug_utils
                if(extension.Equals(VkExtDebugUtilsName))
                {
                    *hasDebugExt = true;
                }
#endif

                enabledInstanceExtensions[insertIndex++] = extension;
                break;
            }
        }
    }

    if(vulkanVersion == VK_API_VERSION_1_0)
    {
        for(const ConstExprString& extension : Desired1_0InstanceExtensions)
        {
            for(const VkExtensionProperties& availableExtension : instanceExtensions)
            {
                if(extension == availableExtension.extensionName)
                {
                    enabledInstanceExtensions[insertIndex++] = extension;
                    break;
                }
            }
        }
    }
    else if(vulkanVersion == VK_API_VERSION_1_1)
    {
        for(const ConstExprString& extension : Desired1_1InstanceExtensions)
        {
            for(const VkExtensionProperties& availableExtension : instanceExtensions)
            {
                if(extension == availableExtension.extensionName)
                {
                    enabledInstanceExtensions[insertIndex++] = extension;
                    break;
                }
            }
        }
    }
    else if(vulkanVersion == VK_API_VERSION_1_2)
    {
        for(const ConstExprString& extension : Desired1_2InstanceExtensions)
        {
            for(const VkExtensionProperties& availableExtension : instanceExtensions)
            {
                if(extension == availableExtension.extensionName)
                {
                    enabledInstanceExtensions[insertIndex++] = extension;
                    break;
                }
            }
        }
    }
    else if(vulkanVersion == VK_API_VERSION_1_3)
    {
        for(const ConstExprString& extension : Desired1_3InstanceExtensions)
        {
            for(const VkExtensionProperties& availableExtension : instanceExtensions)
            {
                if(extension == availableExtension.extensionName)
                {
                    enabledInstanceExtensions[insertIndex++] = extension;
                    break;
                }
            }
        }
    }

    *extensionCount = static_cast<u32>(insertIndex);

    return enabledInstanceExtensions;
}

}
