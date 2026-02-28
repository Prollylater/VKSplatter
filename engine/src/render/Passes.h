#include "RenderPass.h"
#include "ContextController.h"
#include "RenderScene.h"

// Todo: Some grip with this  being here
// Notes: I should take the time to draw the entire architecture at some point
// RenderGraph might actually be simpler to work with to some degree
#include "GBuffers.h"
#include "FrameHandler.h"
#include "logging/Logger.h"

struct RenderContext
{
    VkCommandBuffer cmd;
    uint32_t frameIndex;

    // Internal Engine Refs forward
    class PassBackend &backend;
    class GPUResourceRegistry &registry;
    class PipelineManager &pipelines;
    const class DescriptorManager &materialDescriptors;
    const class DescriptorManager &passDescriptors;
    const struct FrameResources &frameRess;

    // --- Granular Helpers (Power User API) ---

    // Automatically resolves and binds sets based on the activeScopesMask
    void bindDescriptorSets(uint32_t pipelineIndex, uint32_t materialIndex) const;

    void bindGeometry(BufferKey vtxHandle, BufferKey idxHandle) const;

    void pushConstants(uint32_t pipelineIndex, VkShaderStageFlags stageFlags, const void *data, uint32_t size) const;
    void drawIndexed(const struct Drawable *draw) const;
    void drawDefaultQueues() const;
};

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
using PassExecuteFn = std::function<void(const RenderContext &)>;

struct PassBackend
{
    RenderPassConfig config;
    DynamicPassInfo dynamicInfo;
    RenderPassManager renderPassLegacy;

    SwapChainResources frameBuffers;
    RenderPassFrame frames; // drawables per frame

    VkExtent2D extent{}; // A passes is going to have one extent
    bool isDynamic() const { return config.mType == RenderConfigType::Dynamic; }

    // Descriptor related
    uint8_t activeScopesMask = 0;
    uint8_t descriptorSetIndex = UINT8_MAX;

    // Notes: Material is ignored this should be reflected
    VkDescriptorSet scopedSets[3][static_cast<size_t>(DescriptorScope::Count)] = {};
    void activateSet(DescriptorScope scope)
    {
        uint32_t scopeIdx = static_cast<uint32_t>(scope);
        activeScopesMask |= (1 << scopeIdx);
    }
    void linkSet(DescriptorScope scope, uint32_t frameIndex, VkDescriptorSet set)
    {
        uint32_t scopeIdx = static_cast<uint32_t>(scope);
        activeScopesMask |= (1 << scopeIdx);
        scopedSets[frameIndex][scopeIdx] = set;
    }

    PassExecuteFn execute = nullptr;

    void setExecuteCallback(PassExecuteFn callback)
    {
        execute = std::move(callback);
    }
};

class RenderPassHandler
{
public:
    RenderPassHandler() = default;
    ~RenderPassHandler() = default;
    void init(VulkanContext &ctx, GBuffers &gbuffers, FrameHandler &frameHandler);
    void addPass(RenderPassType type, RenderPassConfig passesCfg);

    PassBackend &getBackend(RenderPassType type);
    const std::vector<RenderPassType> &getExecutions() const
    {
        return mExecutionOrder;
    };

    void beginPass(RenderPassType type, VkCommandBuffer cmd);
    void endPass(RenderPassType type, VkCommandBuffer cmd);

    // Todo: Handle reload mecanism
    void destroyRessources(VkDevice device);
    DescriptorManager &getDescriptors()
    {
        return mDescriptorManager;
    }

private:
    const VulkanContext *mContext;
    const GBuffers *mGBuffers;
    FrameHandler *mFrameHandler; // This might be an undesirable dependencies
    std::array<PassBackend, (size_t)RenderPassType::Count> mPasses{};
    std::vector<RenderPassType> mExecutionOrder;
    DescriptorManager mDescriptorManager;

private:
    void setBeginPassTransition(PassBackend &backend, VkCommandBuffer cmd);
    void setEndPassTransition(PassBackend &backend, VkCommandBuffer cmd);
    void updateDynamicRenderingInfo(RenderPassType type, VkExtent2D extent);

    VkImageView resolveAttachment(const AttachmentSource &src);
    VkImage resolveImage(const AttachmentSource &src);
    VkImageAspectFlags aspectFromRole(AttachmentConfig::Role role);
    [[nodiscard]] VkExtent2D resolvePassExtent(const PassBackend &pass);
    VkExtent2D resolveAttachmentExtent(const AttachmentSource &src);
};