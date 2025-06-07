#include "vd/VulkanDebug.hpp"
#include <ConPrinter.hpp>

namespace tau::vd {

VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    const VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* const userData
) noexcept
{
    if constexpr(true)
    {
        if(messageSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            return VK_FALSE;
        }
    }

    if(!callbackData)
    {
        return VK_FALSE;
    }

    if(callbackData->pMessage && ::std::strstr(callbackData->pMessage, "uses API version 1.2 which is older than the application specified API version of 1.3. May cause issues."))
    {
        return VK_FALSE;
    }

    switch(messageSeverity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            ConPrinter::Print("Vulkan Trace [");
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            ConPrinter::Print("Vulkan Info [");
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            ConPrinter::Print("Vulkan Warning [");
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            ConPrinter::Print("Vulkan Error [");
            break;
        default:
            ConPrinter::Print("Vulkan Unknown Message [");
            break;
    }

    if(messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
    {
        ConPrinter::Print('G');
    }
    else
    {
        ConPrinter::Print(' ');
    }

    if(messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
    {
        ConPrinter::Print('V');
    }
    else
    {
        ConPrinter::Print(' ');
    }

    if(messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
    {
        ConPrinter::Print('P');
    }
    else
    {
        ConPrinter::Print(' ');
    }

    if(messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT)
    {
        ConPrinter::Print('D');
    }
    else
    {
        ConPrinter::Print(' ');
    }

    ConPrinter::PrintLn("] [{}]: {}\n", callbackData->messageIdNumber, callbackData->pMessage);

    return VK_FALSE;
}

}
