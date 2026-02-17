
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
    // Todo: Look around onto use case for this
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &mFFrameBuffer[index]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create framebuffer!");
    }
}


    void RenderPassHandler::init(VulkanContext &ctx, GBuffers &gbuffers, FrameHandler &frameHandler)
    {
        mContext = &ctx;
        mGBuffers = &gbuffers;
        mFrameHandler = &frameHandler;
    }
    
    //Todo: Recreating a pass and readding is UB
    void RenderPassHandler::addPass(RenderPassType type, RenderPassConfig passesCfg)
    {
        auto &backend = mPasses[(size_t)type];

        const auto &mLogDeviceM = mContext->getLDevice();
        const auto &mPhysDeviceM = mContext->getPDeviceM();
        const auto &mSwapChainM = mContext->getSwapChainManager();

        const VkDevice &device = mLogDeviceM.getLogicalDevice();

        // Onward is just hardcoded stuff not meant to be dynamic yet
        if (passesCfg.mType == RenderConfigType::LegacyRenderPass)
        {
            backend.renderPassLegacy.createRenderPass(device, 0, passesCfg);
            backend.config = passesCfg;

            for (int i = 0; i < mContext->mSwapChainM.GetSwapChainImageViews().size(); i++)
            {
                std::vector<VkImageView> FBViews;
                std::vector<VkImageView> colorViews;
                VkImageView depthView;

                for (const auto &binding : passesCfg.attachments)
                {
                    VkImageView view = resolveAttachment(binding.source);
                    switch (binding.config.role)
                    {
                    case AttachmentConfig::Role::Present:
                        FBViews.push_back(view);
                        break;

                    case AttachmentConfig::Role::Color:
                        colorViews.push_back(view);
                        break;

                    case AttachmentConfig::Role::Depth:
                        depthView = view;
                        break;
                    }
                }

                // Then regular colors
                FBViews.insert(
                    FBViews.end(),
                    colorViews.begin(),
                    colorViews.end());
                // Multiple FrameBUffer might be the same
                backend.frameBuffers.createFrameBuffer(device, backend.renderPassLegacy.getRenderPass(0), FBViews,
                                                       mContext->getSwapChainManager().getSwapChainExtent(), i);
            }
        }
        else
        {
            backend.config = passesCfg;
        }
        backend.extent = resolvePassExtent(backend);
        mExecutionOrder.push_back(type);
    }

    PassBackend & RenderPassHandler::getBackend(RenderPassType type) { return mPasses[(size_t)type]; }

    void RenderPassHandler::beginPass(RenderPassType type, VkCommandBuffer cmd)
    {
        auto &backend = mPasses[(size_t)type];

        VkExtent2D extent = mContext->getSwapChainManager().getSwapChainExtent();

        if (backend.isDynamic())
        {
            setBeginPassTransition(backend, cmd);

            updateDynamicRenderingInfo(type, extent);

            vkCmdBeginRendering(cmd, &backend.dynamicInfo.info);
        }
        else
        {
            backend.renderPassLegacy.startPass(
                0, cmd,
                backend.frameBuffers.getFramebuffers(mFrameHandler->getCurrentFrameIndex()),
                extent);
        }
    }

    void RenderPassHandler::endPass(RenderPassType type, VkCommandBuffer cmd)
    {
        auto &backend = mPasses[(size_t)type];

        if (backend.isDynamic())
        {
            vkCmdEndRendering(cmd);

            setEndPassTransition(backend, cmd);
        }
        else
        {
            backend.renderPassLegacy.endPass(cmd);
        }
    }

   


void RenderPassHandler::setBeginPassTransition(PassBackend &backend, VkCommandBuffer cmd)
{
    const auto &subpass = backend.config.subpasses[0];

    for (const auto &colorRef : subpass.colorAttachments)
    {
        const auto &binding = backend.config.attachments[colorRef.index];

        VkImage image = resolveImage(binding.source);

        auto barrier = vkUtils::Texture::makeTransition(
            image,
            binding.config.initialLayout,
            colorRef.layout,
            VK_IMAGE_ASPECT_COLOR_BIT);

        vkUtils::Texture::recordImageMemoryBarrier(cmd, barrier);
    }

    if (subpass.depthAttachment)
    {
        const auto &depthRef = *subpass.depthAttachment;
        const auto &binding = backend.config.attachments[depthRef.index];

        VkImage image = resolveImage(binding.source);

        auto barrier = vkUtils::Texture::makeTransition(
            image,
            binding.config.initialLayout,
            depthRef.layout,
            VK_IMAGE_ASPECT_DEPTH_BIT);

        vkUtils::Texture::recordImageMemoryBarrier(cmd, barrier);
    }

    // Todo: This should be customizable for some effect

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(backend.extent.width);
    viewport.height = static_cast<float>(backend.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = backend.extent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void RenderPassHandler::setEndPassTransition(PassBackend &backend,
                                             VkCommandBuffer cmd)
{
    const auto &subpass = backend.config.subpasses[0];

    for (const auto &colorRef : subpass.colorAttachments)
    {
        const auto &binding = backend.config.attachments[colorRef.index];

        VkImage image = resolveImage(binding.source);

        auto barrier = vkUtils::Texture::makeTransition(
            image,
            colorRef.layout,
            binding.config.finalLayout,
            VK_IMAGE_ASPECT_COLOR_BIT);

        vkUtils::Texture::recordImageMemoryBarrier(cmd, barrier);
    }

    if (subpass.depthAttachment)
    {
        const auto &depthRef = *subpass.depthAttachment;
        const auto &binding = backend.config.attachments[depthRef.index];

        VkImage image = resolveImage(binding.source);

        auto barrier = vkUtils::Texture::makeTransition(
            image,
            depthRef.layout,
            binding.config.finalLayout,
            VK_IMAGE_ASPECT_DEPTH_BIT);

        vkUtils::Texture::recordImageMemoryBarrier(cmd, barrier);
    }
}

void RenderPassHandler::updateDynamicRenderingInfo(
    RenderPassType type,
    VkExtent2D extent)
{
    auto &backend = mPasses[static_cast<uint32_t>(type)];
    auto &colorInfos = backend.dynamicInfo.colorAttachments;
    auto &depthInfo = backend.dynamicInfo.depthAttachment;
    auto &renderInfo = backend.dynamicInfo.info;

    // Todo:
    // Technically there's probably only one present Attachments ?
    colorInfos.clear();
    backend.dynamicInfo.depthAttachment = {};

    const auto &subpass = backend.config.subpasses[0];

    for (const auto &colorRef : subpass.colorAttachments)
    {
        const auto &binding = backend.config.attachments[colorRef.index];

        VkImageView view = resolveAttachment(binding.source);

        VkRenderingAttachmentInfo info = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = view,
            .imageLayout = colorRef.layout, // make more sense than previous  binding.config.finalLayout
            .loadOp = binding.config.loadOp,
            .storeOp = binding.config.storeOp};

        if (binding.config.role == AttachmentConfig::Role::Present)
        {
            info.clearValue.color = {0.2f, 0.2f, 0.2f, 1.0f};
            ;
        }
        else
        {
            info.clearValue.color = {{0.f, 0.f, 0.f, 1.f}};
        }
        colorInfos.push_back(info);
    }

    if (subpass.depthAttachment)
    {
        const auto &depthRef = *subpass.depthAttachment;
        const auto &binding = backend.config.attachments[depthRef.index];

        VkImageView view = resolveAttachment(binding.source);

        backend.dynamicInfo.depthAttachment = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = view,
            .imageLayout = depthRef.layout,
            .loadOp = binding.config.loadOp,
            .storeOp = binding.config.storeOp,
            .clearValue = {1.f, 0}};
    }

    renderInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {.offset = {0, 0}, .extent = backend.extent},
        .layerCount = 1,
        .colorAttachmentCount = static_cast<uint32_t>(colorInfos.size()),
        .pColorAttachments = colorInfos.data(),
        .pDepthAttachment = depthInfo.imageView != VK_NULL_HANDLE ? &depthInfo : nullptr};
}

// Todo: Handle reload mecanism
void RenderPassHandler::destroyRessources(VkDevice device)
{
    for (auto &pass : mPasses)
    {
        if (pass.isDynamic())
        {
            pass.renderPassLegacy.destroyAll(device);
            for (int i = 0; i < mContext->mSwapChainM.GetSwapChainImageViews().size(); i++)
            {
                pass.frameBuffers.destroyFramebuffers(device);
            }
        }
        else
        {
            // None of those are persistant
        }
        // Config is preserved
    }
}

VkImageView RenderPassHandler::resolveAttachment(const AttachmentSource &src)
{
    switch (src.type)
    {
    case AttachmentSource::Type::Swapchain:
        return mContext->getSwapChainManager().GetSwapChainImageViews()[mFrameHandler->getCurrentFrameIndex()];
    case AttachmentSource::Type::GBuffer:
        // TODO: Convenience
        if (src.id == UINT32_MAX) // THis currently imply our src is depth
        {
            return mGBuffers->getDepthImageView();
        }
        return mGBuffers->getColorImageView(src.id);
        // Ideally Depth would not be separated in GBuffers anymore
    case AttachmentSource::Type::FrameLocal:
        // TODO: Convenience
        // FrameHandler ought to be redesigned, and FrameLocal should be replace by GPURegistry
        // return mFrameHandler->getCurrentFrameData().cascadePoolArray.getView();
        return mFrameHandler->getCurrentFrameData().depthView[0];
    case AttachmentSource::Type::External:
        // return  could fetch from GPU Registry but wouldn't work if local
    default:
        return VK_NULL_HANDLE;
    }
}

VkImage RenderPassHandler::resolveImage(const AttachmentSource &src)
{
    switch (src.type)
    {
    case AttachmentSource::Type::Swapchain:
        return mContext->getSwapChainManager().GetSwapChainImages()[mFrameHandler->getCurrentFrameIndex()];
    case AttachmentSource::Type::GBuffer:
        // TODO: Convenience
        if (src.id == UINT32_MAX) // THis currently imply our src is depth
        {
            return mGBuffers->getDepthImage();
        }
        return mGBuffers->getColorImage(src.id);
        // Ideally Depth would not be separated in GBuffers anymore
    case AttachmentSource::Type::FrameLocal:
        // TODO: Convenience
        // FrameHandler ought to be redesigned, and FrameLocal should be replace by GPURegistry
        // return mFrameHandler->getCurrentFrameData().cascadePoolArray.getView();
        return mFrameHandler->getCurrentFrameData().cascadePoolArray.getImage();
    case AttachmentSource::Type::External:
        // return  could fetch from GPU Registry but wouldn't work if local
    default:
        return VK_NULL_HANDLE;
    }
}

VkImageAspectFlags RenderPassHandler::aspectFromRole(AttachmentConfig::Role role)
{
    switch (role)
    {
    case AttachmentConfig::Role::Depth:
        return VK_IMAGE_ASPECT_DEPTH_BIT;

    case AttachmentConfig::Role::Present:
    case AttachmentConfig::Role::Color:
    default:
        return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

VkExtent2D RenderPassHandler::resolvePassExtent(const PassBackend &pass)
{
    VkExtent2D result{0, 0};

    for (const auto &binding : pass.config.attachments)
    {
        VkExtent2D attachmentExtent = resolveAttachmentExtent(binding.source);
        result.width = std::max(result.width, attachmentExtent.width);
        result.height = std::max(result.height, attachmentExtent.height);
    }

    return result;
}

VkExtent2D RenderPassHandler::resolveAttachmentExtent(const AttachmentSource &src)
{
    switch (src.type)
    {
    case AttachmentSource::Type::GBuffer:
        // GBuffer all share one single size
        return mGBuffers->getSize();
    case AttachmentSource::Type::FrameLocal:{
        VkExtent3D extent3D = mFrameHandler->getCurrentFrameData().cascadePoolArray.getExtent();
        return {extent3D.width, extent3D.height};}
    case AttachmentSource::Type::External:
    case AttachmentSource::Type::Swapchain:
    default:
        return mContext->getSwapChainManager().getSwapChainExtent();
    }
}