
#include "RenderPass.h"
#include <array>

//This is the basic RenderPass not dealing with Other Attachement type
void RenderPassManager::createRenderPass(VkDevice device, const RenderPassConfig &configStruct)
{
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkAttachmentReference> colorAttachmentRefs;
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = configStruct.colorFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments.push_back(colorAttachment);

    // Id == 0 and equvialent to location out vec4 outColor
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachmentRefs.push_back(colorAttachmentRef);

    VkAttachmentDescription depthAttachment{};
    VkAttachmentReference depthAttachmentRef{};

    const bool hasDepth = configStruct.enableDepth;
    if (hasDepth)
    {
        depthAttachment.format = configStruct.depthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        //Starting right after the color
        depthAttachmentRef.attachment = static_cast<uint32_t>(attachments.size());
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        attachments.push_back(depthAttachment);
    }

    // Subpass 
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size());
    subpass.pColorAttachments = colorAttachmentRefs.data();
    subpass.pDepthStencilAttachment = hasDepth ? &depthAttachmentRef : nullptr;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // Implicit Subpass
    dependency.dstSubpass = 0;

    // Operation to wait and stage
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;

    // Operation that can start after the wait
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    /*
    Forgot what it was
    The index of the attachment in this array is directly referenced from the fragment shader with the layout(location = 0) out vec4 outColor directive!
    pInputAttachments: Attachments that are read from a shader
    pResolveAttachments: Attachments used for multisampling color attachments
    pDepthStencilAttachment: Attachment for depth and stencil data
    pPreserveAttachments: Attachments that are not used by this subpass, but for which the data must be preserved
    */

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = configStruct.subpassCount;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &mRenderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }
}

void RenderPassManager::destroyRenderPass(VkDevice device)
{
    vkDestroyRenderPass(device, mRenderPass, nullptr);
}


//TODO:
//Create A rednderer using mRenderOass
void RenderPassManager::startPass(const VkCommandBuffer &command, const VkFramebuffer &frameBuffer, const VkExtent2D &extent)
{
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = mRenderPass;
    renderPassInfo.framebuffer = frameBuffer;

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = extent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.2f, 0.2f, 0.2f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    vkCmdBeginRenderPass(command, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void RenderPassManager::endPass(const VkCommandBuffer &command)
{
    vkCmdEndRenderPass(command);
}