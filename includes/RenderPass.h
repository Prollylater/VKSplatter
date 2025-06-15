

#pragma once
#include "BaseVk.h"
#include "CommandPool.h"
#include "SwapChain.h"
#include "CommandPool.h"

enum class RenderPassType {
    Forward,
    GBuffer,
    Lighting,
    ShadowMap,
    PostProcess,
};

//Expand it for Subpasses and multiple attachements
struct RenderPassConfig
{
    //Couldcome in multiple
    VkFormat colorFormat;                       // From swapchain
    VkFormat depthFormat = VK_FORMAT_UNDEFINED; // Optional
    bool enableDepth = true;
    bool enableMSAA = false;

    uint32_t subpassCount = 1;

    //Kind og ignoring this for now
    VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;
    VkAttachmentLoadOp colorLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp colorStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkAttachmentLoadOp depthLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkAttachmentStoreOp depthStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
};

/*
  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment
  VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: Images to be presented in the swap chain
  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: Images to be used as destination for a memory copy operation
*/

/*
Create/render pass definitions through a config files
*/
class RenderPassManager
{
public:
    RenderPassManager() = default;
    ~RenderPassManager() = default;

    VkRenderPass getRenderPass() const { return mRenderPass; }
    void createRenderPass(VkDevice device, const RenderPassConfig &);

    //Move this to higher function
    void startPass(const VkCommandBuffer &command, const VkFramebuffer &frameBuffer, const VkExtent2D &extent);
    void endPass(const VkCommandBuffer &);

    void destroyRenderPass(VkDevice device);

private:
    VkDevice mDevice = VK_NULL_HANDLE;
    VkRenderPass mRenderPass = VK_NULL_HANDLE;
};

/*On multiple render pass,
Should eventually have multiple of those
Typically for shadowPass where i guess only the depth from light view matter

Deferred Rendering with Gbuffer pass then coloring pass

Other stuff ? Each with their own shader

Maybe multi viewport from differnt pov

*/