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

    RenderConfigType type = RenderConfigType::Dynamic;

    RenderTargetConfig dynamicConfig;
    DynamicPassInfo dynamicInfo{};

    // TODOl    Check for remnant of the old pattern
    RenderPassManager renderPassLegacy; // Shouldn't handle Pass type granularity anymore

    SwapChainResources frameBuffers;

    RenderPassFrame frames; // drawables per frame

    bool isDynamic() const { return type == RenderConfigType::Dynamic; }
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

    void addPass(RenderPassType type, RenderPassConfig legacyCfg)
    {
        auto &backend = mPasses[(size_t)type];
        backend.type = RenderConfigType::LegacyRenderPass;
        std::cout << "Init Render Infrastructure" << std::endl;

        const auto &mLogDeviceM = mContext->getLDevice();
        const auto &mPhysDeviceM = mContext->getPDeviceM();
        const auto &mSwapChainM = mContext->getSwapChainManager();

        const VkDevice &device = mLogDeviceM.getLogicalDevice();

        const VkFormat depthFormat = mPhysDeviceM.findDepthFormat();

        // Onward is just hardcoded stuff not meant to be dynamic yet
        const VmaAllocator &allocator = mLogDeviceM.getVmaAllocator();

        backend.renderPassLegacy.createRenderPass(device, 0, legacyCfg);

        for (int i = 0; i < mContext->mSwapChainM.GetSwapChainImageViews().size(); i++)
        {
            std::vector<VkImageView> FBViews;
            std::vector<VkImageView> colorViews;
            VkImageView depthView;

            for (const auto &binding : legacyCfg.attachments)
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

    void addPass(RenderPassType type, RenderTargetConfig dynConfig)
    {
        auto &backend = mPasses[(size_t)type];
        backend.type = RenderConfigType::Dynamic;
        backend.dynamicConfig = dynConfig;
    }

    PassBackend &getBackend(RenderPassType type) { return mPasses[(size_t)type]; }

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

        for (const auto &binding : backend.dynamicConfig.attachments)
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
            if (pass.type == RenderConfigType::LegacyRenderPass)
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
            return mFrameHandler->getCurrentFrameData().cascadePoolArray.getView();
        case AttachmentSource::Type::External:
            // return  could fetch from GPU Registry but wouldn't work if local
        default:
            return VK_NULL_HANDLE;
        }
    }
};
