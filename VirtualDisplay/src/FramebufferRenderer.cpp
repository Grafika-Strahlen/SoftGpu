#include "vd/FramebufferRenderer.hpp"
#include "vd/VulkanCommandPools.hpp"
#include <ConPrinter.hpp>
#include <vk_mem_alloc.h>

#include "vd/Window.hpp"

namespace tau::vd {

FramebufferRenderer::~FramebufferRenderer() noexcept
{
    vmaDestroyBuffer(m_Device->VmaAllocator(), m_StagingBuffer, m_StagingBufferAllocation);

    for(u32 i = 0; i < m_CommandBuffers.Count(); ++i)
    {
        m_Device->FreeCommandBuffers(m_CommandPools->GetCommandPool(m_ThreadIndex, i), 1, &m_CommandBuffers[i]);
    }
}

VkCommandBuffer FramebufferRenderer::Record(const u32 frameIndex, bool active) const noexcept
{
    if(frameIndex >= m_CommandBuffers.Length())
    {
        ConPrinter::PrintLn("Frame index {} was greater than the maximum number of registered frames {}.", frameIndex, m_CommandBuffers.Length());
        return VK_NULL_HANDLE;
    }

    const uSys dataSize = m_StagingBufferAllocationInfo.size / m_CommandBuffers.Count();
    const uSys writeIndex = dataSize * frameIndex;

    const uSys frameSize = static_cast<uSys>(m_Window->FramebufferWidth()) * static_cast<uSys>(m_Window->FramebufferHeight()) * 4;

    if(active)
    {
        (void) ::std::memcpy(static_cast<u8*>(m_StagingBufferAllocationInfo.pMappedData) + writeIndex, m_RawFramebuffer, frameSize);
    }
    else
    {
        (void) ::std::memcpy(static_cast<u8*>(m_StagingBufferAllocationInfo.pMappedData) + writeIndex, 0, frameSize);
    }

    VkCommandBuffer commandBuffer = m_CommandBuffers[frameIndex];

    VkImageMemoryBarrier barrier { };
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext = nullptr;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_Frames[frameIndex];
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    {
        VkCommandBufferBeginInfo beginInfo { };
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.pNext = nullptr;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        beginInfo.pInheritanceInfo = nullptr;

        VK_CALL(m_Device->VkResetCommandBuffer(commandBuffer, 0), "resetting command buffer for FrameBufferRenderer");

        VK_CALL(m_Device->VkBeginCommandBuffer(commandBuffer, &beginInfo), "beginning command buffer for FramebufferRenderer");
    }

    {
        barrier.srcAccessMask = VK_ACCESS_NONE;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        m_Device->VkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }

    {
        VkBufferImageCopy regions[1] = { };
        regions[0].bufferOffset = writeIndex;
        regions[0].bufferRowLength = 0;
        regions[0].bufferImageHeight = 0;
        regions[0].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        regions[0].imageSubresource.mipLevel = 0;
        regions[0].imageSubresource.baseArrayLayer = 0;
        regions[0].imageSubresource.layerCount = 1;
        regions[0].imageOffset.x = 0;
        regions[0].imageOffset.y = 0;
        regions[0].imageOffset.z = 0;
        regions[0].imageExtent.width = m_Window->FramebufferWidth();
        regions[0].imageExtent.height = m_Window->FramebufferHeight();
        regions[0].imageExtent.depth = 1;

        m_Device->VkCmdCopyBufferToImage(commandBuffer, m_StagingBuffer, m_Frames[frameIndex], m_FrameLayout, 1, regions);
    }

    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_NONE;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        m_Device->VkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }

    m_Device->VkEndCommandBuffer(commandBuffer);

    return commandBuffer;

    VK_ERROR_HANDLER_VK_NULL();
}

void FramebufferRenderer::RebuildBuffers(const DynArray<VkImage>& frames, const void* const rawFramebuffer) noexcept
{
    m_Frames = frames;
    m_RawFramebuffer = rawFramebuffer;

    if(m_StagingBuffer)
    {
        vmaDestroyBuffer(m_Device->VmaAllocator(), m_StagingBuffer, m_StagingBufferAllocation);
    }

    VkMemoryRequirements frameMemoryRequirements;
    m_Device->GetImageMemoryRequirements(m_Frames[0], &frameMemoryRequirements);

    VkBufferCreateInfo bufferCreateInfo { };
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.pNext = nullptr;
    bufferCreateInfo.size = frameMemoryRequirements.size * m_Frames.Count();
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.queueFamilyIndexCount = 0;
    bufferCreateInfo.pQueueFamilyIndices = nullptr;

    VmaAllocationCreateInfo bufferAllocationInfo { };
    bufferAllocationInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
    bufferAllocationInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    bufferAllocationInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    bufferAllocationInfo.preferredFlags = 0;
    bufferAllocationInfo.memoryTypeBits = 0;
    bufferAllocationInfo.pool = VK_NULL_HANDLE;
    bufferAllocationInfo.pUserData = nullptr;
    bufferAllocationInfo.priority = 0.0f;

    VK_CALL(vmaCreateBuffer(
        m_Device->VmaAllocator(),
        &bufferCreateInfo,
        &bufferAllocationInfo,
        &m_StagingBuffer,
        &m_StagingBufferAllocation,
        &m_StagingBufferAllocationInfo
    ), "allocating staging buffer for FrameBufferRenderer.");

    vmaSetAllocationName(m_Device->VmaAllocator(), m_StagingBufferAllocation, "FrameBufferRenderer Staging Buffer");

    VK_ERROR_HANDLER_VOID();
}

Ref<FramebufferRenderer> FramebufferRenderer::CreateFramebufferRenderer(
    const Ref<Window>& window,
    const WeakRef<VulkanDevice>& device, 
    const void* const rawFramebuffer, 
    const u32 threadIndex, 
    const Ref<VulkanCommandPools>& commandPools, 
    const DynArray<VkImage>& frames, 
    const VkImageLayout frameLayout
) noexcept
{
    DynArray<VkCommandBuffer> commandBuffers(frames.Count());

    VkMemoryRequirements frameMemoryRequirements;
    device->GetImageMemoryRequirements(frames[0], &frameMemoryRequirements);
    
    VkCommandBufferAllocateInfo allocateInfo { };
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.pNext = nullptr;

    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;

    // ReSharper disable once CppJoinDeclarationAndAssignment
    Ref<FramebufferRenderer> framebufferRenderer;

    for(u32 i = 0; i < frames.Count(); ++i)
    {
        allocateInfo.commandPool = commandPools->GetCommandPool(threadIndex, i);
        
        VK_CALL_FMT(device->AllocateCommandBuffers(&allocateInfo, &commandBuffers[i]), "allocate Command Buffer {} for FramebufferRenderer", i);
    }
    
    framebufferRenderer = Ref<FramebufferRenderer>(
        window,
        device, 
        threadIndex, 
        frames, 
        frameLayout, 
        commandPools, 
        ::std::move(commandBuffers)
    );

    framebufferRenderer->RebuildBuffers(frames, rawFramebuffer);

    return framebufferRenderer;

    VK_ERROR_HANDLER_NULL();
}

}
