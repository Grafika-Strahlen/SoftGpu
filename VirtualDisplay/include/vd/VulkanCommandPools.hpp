#pragma once

#include <Objects.hpp>
#include <DynArray.hpp>
#include <Safeties.hpp>

#include "VulkanFunctions.hpp"

namespace tau::vd {

class VulkanCommandPools final
{
    DELETE_CM(VulkanCommandPools);
public:
    VulkanCommandPools(const WeakRef<VulkanDevice>& device, DynArray<VkCommandPool>&& pools, const u32 threadCount, const u32 frameCount) noexcept;

    ~VulkanCommandPools() noexcept;

    [[nodiscard]] VkCommandPool GetCommandPool(const u32 threadIndex, const u32 frameIndex) noexcept
    {
        return m_Pools[ComputePoolIndex(threadIndex, frameIndex)];
    }

    VkResult ResetCommandPools(const VkCommandPoolResetFlags flags = 0) noexcept;
    VkResult ResetCommandPool(const u32 threadIndex, const u32 frameIndex, const VkCommandPoolResetFlags flags = 0) noexcept;
    void TrimCommandPools(const VkCommandPoolTrimFlags flags = 0) noexcept;
    void TrimCommandPool(const u32 threadIndex, const u32 frameIndex, const VkCommandPoolTrimFlags flags = 0) noexcept;
private:
    [[nodiscard]] u32 ComputePoolIndex(const u32 threadIndex, const u32 frameIndex) noexcept { return threadIndex * m_ThreadCount + frameIndex; }
private:
    WeakRef<VulkanDevice> m_Device;
    DynArray<VkCommandPool> m_Pools;
    u32 m_ThreadCount;
    u32 m_FrameCount;
public:
    [[nodiscard]] static Ref<VulkanCommandPools> CreateCommandPools(const WeakRef<VulkanDevice>& device, const u32 threadCount, const u32 frameCount, const u32 queueFamilyIndex) noexcept;
};

}
