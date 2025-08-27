/**
 * @file
 *
 * Copyright (c) 2025. Grafika Strahlen LLC
 * All rights reserved.
 */
#include "vd/VulkanInstanceExtensions.hpp"
#include "vd/MemoryRecovery.hpp"
#include <vulkan/vulkan.h>
#include <String.hpp>
#include <ConPrinter.hpp>

namespace tau::vd {

static constexpr ConstExprString DesiredLayers[] = {
    // "VK_LAYER_KHRONOS_validation"
};

[[nodiscard]] static DynArray<VkLayerProperties> GetInstanceLayers() noexcept
{
    u32 propertyCount;
    VkResult result = vkEnumerateInstanceLayerProperties(&propertyCount, nullptr);

    if(result == VK_ERROR_OUT_OF_HOST_MEMORY)
    {
        tau::vd::RecoverSacrificialMemory();
        ConPrinter::PrintLn("Ran out of system memory while querying number of instance layers.");
        return { };
    }

    if(result == VK_ERROR_OUT_OF_DEVICE_MEMORY)
    {
        ConPrinter::PrintLn("Ran out of device memory while querying number of instance layers.");
        return { };
    }

    if(result != VK_SUCCESS && result != VK_INCOMPLETE)
    {
        ConPrinter::PrintLn("Encountered undefined error while querying number of instance layers: {}.", result);
        return { };
    }

    do
    {
        DynArray<VkLayerProperties> layers(propertyCount);

        result = vkEnumerateInstanceLayerProperties(&propertyCount, layers);

        if(result == VK_ERROR_OUT_OF_HOST_MEMORY)
        {
            tau::vd::RecoverSacrificialMemory();
            ConPrinter::PrintLn("Ran system memory while querying instance layers.");
            return { };
        }

        if(result == VK_ERROR_OUT_OF_DEVICE_MEMORY)
        {
            ConPrinter::PrintLn("Ran device memory while querying instance layers.");
            return { };
        }

        if(result == VK_SUCCESS)
        {
            return layers;
        }
    } while(result == VK_INCOMPLETE);

    ConPrinter::PrintLn("Encountered undefined error while querying instance layers: {}.", result);
    return { };
}

DynArray<const char*> GetRequestedInstanceLayers(u32* const layerCount) noexcept
{
    DynArray<VkLayerProperties> instanceLayers = GetInstanceLayers();

    if(instanceLayers.Length() == 0)
    {
        return { };
    }

    DynArray<const char*> enabledInstanceLayers(instanceLayers.Length());

    uSys insertIndex = 0;

    for(const ConstExprString& layer : DesiredLayers)
    {
        for(const VkLayerProperties& availableLayer : instanceLayers)
        {
            if(layer == availableLayer.layerName)
            {
                enabledInstanceLayers[insertIndex++] = layer;
                break;
            }
        }
    }

    *layerCount = static_cast<u32>(insertIndex);

    return enabledInstanceLayers;
}

}
