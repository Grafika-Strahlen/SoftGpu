#include "vd/VulkanCommandPools.hpp"

namespace tau::vd {

VulkanCommandPools::VulkanCommandPools(const WeakRef<VulkanDevice>& device, DynArray<VkCommandPool>&& pools, const u32 threadCount, const u32 frameCount) noexcept
    : m_Device(device)
    , m_Pools(::std::move(pools))
    , m_ThreadCount(threadCount)
    , m_FrameCount(frameCount)
{ }

VulkanCommandPools::~VulkanCommandPools() noexcept
{
    for(const VkCommandPool pool : m_Pools)
    {
        m_Device->DestroyCommandPool(pool);
    }
}

VkResult VulkanCommandPools::ResetCommandPools(const VkCommandPoolResetFlags flags) noexcept
{
    for(const VkCommandPool pool : m_Pools)
    {
        const VkResult result = m_Device->ResetCommandPool(pool, flags);

        if(result != VK_SUCCESS)
        {
            return result;
        }
    }

    return VK_SUCCESS;
}

VkResult VulkanCommandPools::ResetCommandPool(const u32 threadIndex, const u32 frameIndex, const VkCommandPoolResetFlags flags) noexcept
{
    return m_Device->ResetCommandPool(GetCommandPool(threadIndex, frameIndex), flags);
}

void VulkanCommandPools::TrimCommandPools(const VkCommandPoolTrimFlags flags) noexcept
{
    for(const VkCommandPool pool : m_Pools)
    {
        m_Device->TrimCommandPool(pool, flags);
    }
}

void VulkanCommandPools::TrimCommandPool(const u32 threadIndex, const u32 frameIndex, const VkCommandPoolTrimFlags flags) noexcept
{
    m_Device->TrimCommandPool(GetCommandPool(threadIndex, frameIndex), flags);
}

Ref<VulkanCommandPools> VulkanCommandPools::CreateCommandPools(const WeakRef<VulkanDevice>& device, const u32 threadCount, const u32 frameCount, const u32 queueFamilyIndex) noexcept
{
    VkCommandPoolCreateInfo createInfo { };
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    createInfo.queueFamilyIndex = queueFamilyIndex;

    const uSys poolCount = static_cast<uSys>(threadCount) * static_cast<uSys>(frameCount);

    DynArray<VkCommandPool> commandPools(poolCount);
    commandPools.Zero();

    for(uSys i = 0; i < poolCount; ++i)
    {
        VK_CALL_FMT(device->CreateCommandPool(&createInfo, &commandPools[i]), "creating command pool {}", i);
    }

    return Ref<VulkanCommandPools>(device, ::std::move(commandPools), threadCount, frameCount);

    VK_ERROR_HANDLER_NULL();
}

}
