#include "vd/VulkanDeviceExtensions.hpp"
#include "vd/MemoryRecovery.hpp"
#include <String.hpp>
#include <ConPrinter.hpp>

namespace tau::vd {

static constexpr ConstExprString DesiredDeviceExtensions[] = {
};

static constexpr ConstExprString Desired1_0DeviceExtensions[] = {
};

static constexpr ConstExprString Desired1_1DeviceExtensions[] = {
};

static constexpr ConstExprString Desired1_2DeviceExtensions[] = {
};

static constexpr ConstExprString Desired1_3DeviceExtensions[] = {
};

[[nodiscard]] static DynArray<VkExtensionProperties> GetDeviceExtensions(VkPhysicalDevice physicalDevice) noexcept
{
    u32 propertyCount;
    VkResult result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &propertyCount, nullptr);

    if(result == VK_ERROR_OUT_OF_HOST_MEMORY)
    {
        tau::vd::RecoverSacrificialMemory();
        ConPrinter::PrintLn("Ran out of system memory while querying number of device extensions.");
        return { };
    }

    if(result == VK_ERROR_OUT_OF_DEVICE_MEMORY)
    {
        ConPrinter::PrintLn("Ran out of device memory while querying number of device extensions.");
        return { };
    }

    if(result != VK_SUCCESS && result != VK_INCOMPLETE)
    {
        ConPrinter::PrintLn("Encountered undefined error while querying number of device extensions: {}.", result);
        return { };
    }

    do
    {
        DynArray<VkExtensionProperties> extensions(propertyCount);

        result = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &propertyCount, extensions);

        if(result == VK_ERROR_OUT_OF_HOST_MEMORY)
        {
            tau::vd::RecoverSacrificialMemory();
            ConPrinter::PrintLn("Ran out of system memory while querying device extensions.");
            return { };
        }

        if(result == VK_ERROR_OUT_OF_DEVICE_MEMORY)
        {
            ConPrinter::PrintLn("Ran out of device memory while querying device extensions.");
            return { };
        }

        if(result == VK_SUCCESS)
        {
            return extensions;
        }
    } while(result == VK_INCOMPLETE);

    ConPrinter::PrintLn("Encountered undefined error while querying device extensions: 0x{XP0}.", result);
    return { };
}

DynArray<const char*> GetRequestedDeviceExtensions(VkPhysicalDevice physicalDevice, const u32 deviceVulkanVersion, u32* const extensionCount) noexcept
{
    *extensionCount = 0;
    DynArray<VkExtensionProperties> deviceExtensions = GetDeviceExtensions(physicalDevice);

    if(deviceExtensions.Length() == 0)
    {
        return { };
    }

    for(const ConstExprString& extension : RequiredDeviceExtensions)
    {
        bool found = false;

        for(const VkExtensionProperties& availableExtension : deviceExtensions)
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
            ConPrinter::PrintLn("Failed to find required device extension {}.", extension);
            return { };
        }
    }

    DynArray<const char*> enabledInstanceExtensions(deviceExtensions.Length());

    uSys insertIndex = 0;

    for(const ConstExprString& extension : RequiredDeviceExtensions)
    {
        enabledInstanceExtensions[insertIndex++] = extension;
    }

    for(const ConstExprString& extension : DesiredDeviceExtensions)
    {
        for(const VkExtensionProperties& availableExtension : deviceExtensions)
        {
            if(extension == availableExtension.extensionName)
            {
                enabledInstanceExtensions[insertIndex++] = extension;
                break;
            }
        }
    }

    if(deviceVulkanVersion == VK_API_VERSION_1_0)
    {
        for(const ConstExprString& extension : Desired1_0DeviceExtensions)
        {
            for(const VkExtensionProperties& availableExtension : deviceExtensions)
            {
                if(extension == availableExtension.extensionName)
                {
                    enabledInstanceExtensions[insertIndex++] = extension;
                    break;
                }
            }
        }
    }
    else if(deviceVulkanVersion == VK_API_VERSION_1_1)
    {
        for(const ConstExprString& extension : Desired1_1DeviceExtensions)
        {
            for(const VkExtensionProperties& availableExtension : deviceExtensions)
            {
                if(extension == availableExtension.extensionName)
                {
                    enabledInstanceExtensions[insertIndex++] = extension;
                    break;
                }
            }
        }
    }
    else if(deviceVulkanVersion == VK_API_VERSION_1_2)
    {
        for(const ConstExprString& extension : Desired1_2DeviceExtensions)
        {
            for(const VkExtensionProperties& availableExtension : deviceExtensions)
            {
                if(extension == availableExtension.extensionName)
                {
                    enabledInstanceExtensions[insertIndex++] = extension;
                    break;
                }
            }
        }
    }
    else if(deviceVulkanVersion == VK_API_VERSION_1_3)
    {
        for(const ConstExprString& extension : Desired1_3DeviceExtensions)
        {
            for(const VkExtensionProperties& availableExtension : deviceExtensions)
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
