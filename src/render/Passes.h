#include "RenderPass.h"
#include "ContextController.h"
#include "RenderScene.h"

// Todo: Some grip with this  being here
// Notes: I should take the time to draw the entire architecture at some point
#include "GBuffers.h"
#include "FrameHandler.h"
#include "logging/Logger.h"
// The local

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

    void init(VulkanContext &ctx, GBuffers &gbuffers, FrameHandler &frameHandler);
    void addPass(RenderPassType type, RenderPassConfig passesCfg);

    PassBackend &getBackend(RenderPassType type);

    void beginPass(RenderPassType type, VkCommandBuffer cmd);
    void endPass(RenderPassType type, VkCommandBuffer cmd);
   
    // Todo: Handle reload mecanism
    void destroyRessources(VkDevice device);
private:
    const VulkanContext *mContext;
    const GBuffers *mGBuffers;
    FrameHandler *mFrameHandler;
    std::array<PassBackend, (size_t)RenderPassType::Count> mPasses{};

    void setBeginPassTransition(PassBackend &backend, VkCommandBuffer cmd);
    void setEndPassTransition(PassBackend &backend, VkCommandBuffer cmd);
    void updateDynamicRenderingInfo(RenderPassType type,VkExtent2D extent);


    VkImageView resolveAttachment(const AttachmentSource &src);
    VkImage resolveImage(const AttachmentSource &src);
    VkImageAspectFlags aspectFromRole(AttachmentConfig::Role role);
    [[nodiscard]] VkExtent2D resolvePassExtent(const PassBackend &pass);
    VkExtent2D resolveAttachmentExtent(const AttachmentSource &src);

     void applyDependencyMasks(PassBackend &backend, vkUtils::Texture::ImageTransition &barrier,
                              uint32_t subpassIndex = 0);
};
