
#include "RenderPass.h"
#include <array>

// This is the basic RenderPass not dealing with Other Attachement type
// With method to create each i guess ?

void RenderPassManager::createRenderPass(VkDevice device, const RenderPassConfig &configStruct)
{
    std::vector<VkAttachmentDescription> attachments;
    attachments.reserve(configStruct.attachments.size());
    for (auto &att : configStruct.attachments)
    {
        VkAttachmentDescription description{};
        description.flags = att.flags;
        description.format = att.format;
        description.samples = att.samples;
        description.loadOp = att.loadOp;
        description.storeOp = att.storeOp;
        description.stencilLoadOp = att.stencilLoadOp;
        description.stencilStoreOp = att.stencilStoreOp;
        description.initialLayout = att.initialLayout;
        description.finalLayout = att.finalLayout;
        attachments.push_back(description);
    }

    std::vector<VkSubpassDescription> vkSubpasses(configStruct.subpasses.size());
    std::vector<std::vector<VkAttachmentReference>> subpassColorRefs(configStruct.subpasses.size());
    std::vector<std::vector<VkAttachmentReference>> subpassInputRefs(configStruct.subpasses.size());
    std::vector<VkAttachmentReference> subpassDepthRefs(configStruct.subpasses.size());

    for (size_t index = 0; index < configStruct.subpasses.size(); index++)
    {
        const auto &configSubpass = configStruct.subpasses[index];

        // Input Attachments
        for (const auto &ref : configSubpass.colorAttachments)
        {
            subpassColorRefs[index].push_back({ref.index, ref.layout});
        }

        for (const auto &ref : configSubpass.inputAttachments)
        {
            subpassInputRefs[index].push_back({ref.index, ref.layout});
        }

        const bool hasDepth = configSubpass.depthAttachment.has_value();
        if (hasDepth)
        {
            const auto &ref = configSubpass.depthAttachment.value();
            // Depth Attachment
            subpassDepthRefs[index] = {ref.index, ref.layout};
        }

        // Subpass

        VkSubpassDescription &subpassHandle = vkSubpasses[index];

        subpassHandle.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        // Todo:
        // subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE; and RayTracing also exist ?
        subpassHandle.colorAttachmentCount = static_cast<uint32_t>(configSubpass.colorAttachments.size());
        subpassHandle.pColorAttachments = subpassColorRefs[index].data();
        subpassHandle.pDepthStencilAttachment = (hasDepth) ? &subpassDepthRefs[index] : nullptr;
        subpassHandle.pInputAttachments = (subpassInputRefs[index].size()) ? subpassInputRefs[index].data() : nullptr;
        // subpassHandle.pPreserveAttachments = nullptr;
        // subpassHandle.pResolveAttachments = nullptr;
    }

    // Actual Render Pass creation
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = configStruct.subpasses.size();
    renderPassInfo.pSubpasses = vkSubpasses.data();
    renderPassInfo.dependencyCount = configStruct.dependencies.size();
    renderPassInfo.pDependencies = configStruct.dependencies.data();

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &mRenderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }
}

void RenderPassManager::destroyRenderPass(VkDevice device)
{
    if (mRenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(device, mRenderPass, nullptr);
        mRenderPass = VK_NULL_HANDLE;
    }
}


//DIfferent parameter
//MAybe keep the config or essentiall element around
void RenderPassManager::startPass(const VkCommandBuffer &command, const VkFramebuffer &frameBuffer, const VkExtent2D &extent)
{
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = mRenderPass;
    renderPassInfo.framebuffer = frameBuffer;

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = extent;

    // Todo: Should derivate from the number of Attachment
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.2f, 0.2f, 0.2f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    VkSubpassContents subpassContent = VK_SUBPASS_CONTENTS_INLINE;
    // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
    vkCmdBeginRenderPass(command, &renderPassInfo, subpassContent);
}

// vkCmdNextSubpass( command, subpass_contents )

void RenderPassManager::endPass(const VkCommandBuffer &command)
{
    vkCmdEndRenderPass(command);
}
