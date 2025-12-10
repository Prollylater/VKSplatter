

#pragma once
#include "BaseVk.h"
#include "SwapChain.h"
#include "config/PipelineConfigs.h"
/*
Create/render pass definitions through a config files
*/

/*
Todo:
Render pass state ?
Same for Render PAss
#Vid 18
*/
class RenderPassManager
{
public:
   
    void createRenderPass(VkDevice device, RenderPassType type, const RenderPassConfig &);

    // Move this to higher function
    void startPass(uint32_t id, const VkCommandBuffer &command, const VkFramebuffer &frameBuffer, const VkExtent2D &extent);
    void endPass(const VkCommandBuffer &);

    void destroyRenderPass(uint32_t id, VkDevice device);
    void destroyAll(VkDevice device);
   
    VkRenderPass getRenderPass(uint32_t id) const;
    VkRenderPass getRenderPass(RenderPassType id) const ;
    const RenderPassConfig &getConfiguration(uint32_t id)  const;
private:
    VkDevice mDevice = VK_NULL_HANDLE;
    struct RenderPassEntry
    {
        RenderPassConfig config;
        VkRenderPass pass;
    };

    std::array<RenderPassEntry, (size_t)RenderPassType::Count> mRenderPasses{};
   
    //std::array<VkRenderPass, (size_t)RenderPassType::Count> mRenderPasses;

    // std::array<size_t, (size_t)RenderPassType::Count> lookUp;
    //  RenderPassConfig mConfiguration;
};
