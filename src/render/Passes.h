#include "RenderPass.h"
#include "ContextController.h"
#include "RenderScene.h"

// Todo: Some grip with this  being here
// Notes: I should take the time to draw the entire architecture at some point
#include "GBuffers.h"
#include "FrameHandler.h"
// The local

// Ideally,-> We create a Pass -> We set up the "render infrastructure"
// Render Infrastructure imply:
// Dynamic rednering : Attachment Source and Prefill most of the Dynamic Rendering Info
// Legacy rednering : Attachment Source and immediately resolve it to the correct VkFrameBuffer
// Then resolve to the correct framebuffer at render time
// By that time GBuffers are intiialzied and this class has no control over it

struct DynamicPassInfo
{
    VkRenderingInfo info;
    // The two below are most likely useless i currently don't use them now that render info is dynamically rebuilt
    std::vector<VkRenderingAttachmentInfo> colorAttachments;
    VkRenderingAttachmentInfo depthAttachment;
};

// Naming is not very great
class SwapChainResources
{
public:
    void createFrameBuffer(VkDevice device, VkRenderPass renderPass, const std::vector<VkImageView> &attachments, const VkExtent2D swapChainExtent, uint32_t index);

    void destroyFramebuffers(VkDevice device);

    const VkFramebuffer &getFramebuffers(uint32_t index) const;

private:
    std::array<VkFramebuffer, 3> mFFrameBuffer;
};

// RenderPassManager should join this ? Or not ?
struct PassBackend
{
    RenderPassConfig config;
    DynamicPassInfo dynamicInfo;
    RenderPassManager renderPassLegacy;

    SwapChainResources frameBuffers;
    RenderPassFrame frames; // drawables per frame

    VkExtent2D extent{}; // A passes is going to have one extent
    bool isDynamic() const { return config.mType == RenderConfigType::Dynamic; }
};

class RenderPassHandler
{
public:
    RenderPassHandler() = default;
    ~RenderPassHandler() = default;

    void init(VulkanContext &ctx, GBuffers &gbuffers, FrameHandler &frameHandler)
    {
        mContext = &ctx;
        mGBuffers = &gbuffers;
        mFrameHandler = &frameHandler;
    }

    void addPass(RenderPassType type, RenderPassConfig passesCfg)
    {
        auto &backend = mPasses[(size_t)type];
        std::cout << "Init Render Infrastructure" << std::endl;

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
        resolvePassExtent(backend);
    }

    PassBackend &getBackend(RenderPassType type) { return mPasses[(size_t)type]; }

    void RenderPassHandler::beginPass(RenderPassType type, VkCommandBuffer cmd)
    {
        auto &backend = mPasses[(size_t)type];

        VkExtent2D extent = mContext->getSwapChainManager().getSwapChainExtent();

        if (backend.isDynamic())
        {
            transitionAttachmentsForBegin(backend, cmd);

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

    void endPass(RenderPassType type, VkCommandBuffer cmd)
    {
        auto &backend = mPasses[(size_t)type];

        if (backend.isDynamic())
        {
            vkCmdEndRendering(cmd);

            transitionAttachmentsForEnd(backend, cmd);
        }
        else
        {
            backend.renderPassLegacy.endPass(cmd);
        }
    }

    void transitionAttachmentsForBegin(PassBackend &backend,
                                       VkCommandBuffer cmd)
    {
        for (const auto &binding : backend.config.attachments)
        {
            VkImage image = resolveImage(binding.source);

            VkImageLayout targetLayout =
                (binding.config.role == AttachmentConfig::Role::Depth)
                    ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
                    : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            auto barrier = vkUtils::Texture::makeTransition(
                image,
                binding.config.initialLayout,
                targetLayout,
                aspectFromRole(binding.config.role));

            barrier.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            barrier.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            /*
                //Ideally we could just loop going from subpass then only adress the relevant attachment
                //This should also be the way in udpateFramesRendering instead of rebuilding from inpit
                //using subpasses we define all our transition
              defConfigRenderPass.addAttachment(AttachmentSource::Swapchain(), colorFormat, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, AttachmentConfig::Role::Present)
            .addAttachment(AttachmentSource::GBuffer(0), depthFormat, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, AttachmentConfig::Role::Depth)
            .addSubpass() // This is chained
            .useColorAttachment(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            .useDepthAttachment(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
            .addDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
            
            */
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

    void transitionAttachmentsForEnd(PassBackend &backend,
                                     VkCommandBuffer cmd)
    {
        for (const auto &binding : backend.config.attachments)
        {
            VkImage image = resolveImage(binding.source);

            VkImageLayout currentLayout =
                (binding.config.role == AttachmentConfig::Role::Depth)
                    ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
                    : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            auto barrier = vkUtils::Texture::makeTransition(
                image,
                currentLayout,
                binding.config.finalLayout,
                aspectFromRole(binding.config.role));

            barrier.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            barrier.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            vkUtils::Texture::recordImageMemoryBarrier(cmd, barrier);
        }
    }

    void updateDynamicRenderingInfo(
        RenderPassType type,
        VkExtent2D extent)
    {
        auto &backend = mPasses[static_cast<uint32_t>(type)];
        auto &colorInfos = backend.dynamicInfo.colorAttachments;
        auto &depthInfo = backend.dynamicInfo.depthAttachment;
        auto &renderInfo = backend.dynamicInfo.info;

        // Todo:
        // Technically there's probably only one present Attachments ?
        VkRenderingAttachmentInfo presentAttachments;
        std::vector<VkRenderingAttachmentInfo> colorAttachments;

        colorInfos.clear();

        for (const auto &binding : backend.config.attachments)
        {
            VkImageView view = resolveAttachment(binding.source);

            VkRenderingAttachmentInfo info{
                .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
                .imageView = view,
                .imageLayout = binding.config.finalLayout,
                .loadOp = binding.config.loadOp,
                .storeOp = binding.config.storeOp};

            switch (binding.config.role)
            {
            case AttachmentConfig::Role::Present:
                info.clearValue = {{0.2f, 0.2f, 0.2f, 1.0f}};
                colorInfos.push_back(info);
                break;

            case AttachmentConfig::Role::Color:
                info.clearValue = {{0.f, 0.f, 0.f, 1.f}};

                colorAttachments.push_back(info);
                break;

            case AttachmentConfig::Role::Depth:
                info.clearValue = {1.0f, 0}; // Important...
                depthInfo = info;
                break;
            }
        }

        // Ensure that if a "present attachment"  is there, they are first in the declaration order
        // So they can be at location = 0
        // Notes: Alternatively user could modulate their attachment themselves
        /*This assume multiple presents, it was removed but no mecanism prevent multiple views
        colorInfos.insert(
            colorInfos.end(),
            presentAttachments.begin(),
            presentAttachments.end());*/

        // Then regular colors
        colorInfos.insert(
            colorInfos.end(),
            colorAttachments.begin(),
            colorAttachments.end());

        renderInfo = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .renderArea = {.offset = {0, 0}, .extent = extent},
            .layerCount = 1,
            .colorAttachmentCount = static_cast<uint32_t>(colorInfos.size()),
            .pColorAttachments = colorInfos.data(),
            .pDepthAttachment = depthInfo.imageView != VK_NULL_HANDLE ? &depthInfo : nullptr};
        // Could also use still enableDepth but i need to rethink this
        //  Todo: set renderArea / layerCount / viewMask as needed
    }

    // Todo: Handle reload mecanism
    void destroyRessources(VkDevice device)
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

private:
    const VulkanContext *mContext;
    const GBuffers *mGBuffers;
    FrameHandler *mFrameHandler;
    std::array<PassBackend, (size_t)RenderPassType::Count> mPasses{};

    VkImageView resolveAttachment(const AttachmentSource &src)
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

    VkImage resolveImage(const AttachmentSource &src)
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

    VkImageAspectFlags aspectFromRole(AttachmentConfig::Role role)
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

    VkExtent2D resolvePassExtent(const PassBackend &pass)
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

    VkExtent2D resolveAttachmentExtent(const AttachmentSource &src)
    {
        switch (src.type)
        {
        case AttachmentSource::Type::GBuffer:
            // GBuffer all share one single size
            return mGBuffers->getSize();
        case AttachmentSource::Type::FrameLocal:
            VkExtent3D extent3D = mFrameHandler->getCurrentFrameData().cascadePoolArray.getExtent();
            return {extent3D.width, extent3D.height};
        case AttachmentSource::Type::External:
        case AttachmentSource::Type::Swapchain:
        default:
            return mContext->getSwapChainManager().getSwapChainExtent();
        }
    }
};
