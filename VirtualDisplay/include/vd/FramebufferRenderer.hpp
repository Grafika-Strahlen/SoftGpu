#pragma once

#include <DynArray.hpp>
#include <Objects.hpp>
#include <Safeties.hpp>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace tau::vd {

class Window;
class VulkanDevice;
class VulkanCommandPools;

class FramebufferRenderer final
{
    DELETE_CM(FramebufferRenderer);
public:
    FramebufferRenderer(
        const Ref<Window>& window,
        const WeakRef<VulkanDevice>& device,
        const u32 threadIndex,
        const DynArray<VkImage>& frames,
        const VkImageLayout frameLayout,
        const Ref<VulkanCommandPools>& commandPools,
        DynArray<VkCommandBuffer>&& commandBuffers
    ) noexcept
        : m_Window(window)
        , m_Device(device)
        , m_RawFramebuffer(nullptr)
        , m_ThreadIndex(threadIndex)
        , m_Frames(frames)
        , m_FrameLayout(frameLayout)
        , m_CommandPools(commandPools)
        , m_CommandBuffers(::std::move(commandBuffers))
        , m_StagingBuffer(VK_NULL_HANDLE)
        , m_StagingBufferAllocation(VK_NULL_HANDLE)
        , m_StagingBufferAllocationInfo{}
    { }

    ~FramebufferRenderer() noexcept;

    [[nodiscard]] VkCommandBuffer Record(const u32 frameIndex, bool active = false) const noexcept;

    void RebuildBuffers(const DynArray<VkImage>& frames, const void* const rawFramebuffer) noexcept;
public:
    [[nodiscard]] static Ref<FramebufferRenderer> CreateFramebufferRenderer(
        const Ref<Window>& window,
        const WeakRef<VulkanDevice>& device,
        const void* const rawFramebuffer, 
        const u32 threadIndex, 
        const Ref<VulkanCommandPools>& commandPools, 
        const DynArray<VkImage>& frames, 
        const VkImageLayout frameLayout
    ) noexcept;
private:
    Ref<Window> m_Window;
    WeakRef<VulkanDevice> m_Device;
    const void* m_RawFramebuffer;
    u32 m_ThreadIndex;
    DynArray<VkImage> m_Frames;
    VkImageLayout m_FrameLayout;
    Ref<VulkanCommandPools> m_CommandPools;
    DynArray<VkCommandBuffer> m_CommandBuffers;
    VkBuffer m_StagingBuffer;
    VmaAllocation m_StagingBufferAllocation;
    VmaAllocationInfo m_StagingBufferAllocationInfo;
};

}
