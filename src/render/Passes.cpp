
#include "Passes.h"


const VkFramebuffer &SwapChainResources::getFramebuffers(uint32_t index) const
{
    return mFFrameBuffer[static_cast<size_t>(index)];
}

void SwapChainResources::destroyFramebuffers(VkDevice device)
{
    for (auto framebuffer : mFFrameBuffer)
    {
        if (framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }
    mFFrameBuffer.fill(VK_NULL_HANDLE);
}

// Todo: Inconsistent  order of argupments
void SwapChainResources::createFrameBuffer(VkDevice device, VkRenderPass renderPass, const std::vector<VkImageView> &attachments, const VkExtent2D extent, uint32_t index)
{
    
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    //Todo: Look around onto use case for this
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &mFFrameBuffer[index]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create framebuffer!");
    }
}
