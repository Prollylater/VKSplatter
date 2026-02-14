

#pragma once
#include "config/PipelineConfigs.h"

class RenderPassManager
{
public:
   
    void createRenderPass(VkDevice device, int type, const RenderPassConfig &);

    // Move this to higher function
    void startPass(uint32_t id, const VkCommandBuffer &command, const VkFramebuffer &frameBuffer, const VkExtent2D &extent);
    void endPass(const VkCommandBuffer &);

    void destroyRenderPass(uint32_t id, VkDevice device);
    void destroyAll(VkDevice device);
   
    VkRenderPass getRenderPass(uint32_t id) const;
    //VkRenderPass getRenderPass(RenderPassType id) const ;
    //const RenderPassConfig &getConfiguration(uint32_t id)  const;
private:
    VkDevice mDevice = VK_NULL_HANDLE;
    struct RenderPassEntry
    {
        //RenderPassConfig config;
        VkRenderPass pass;
    };

    std::array<RenderPassEntry, (size_t)RenderPassType::Count> mRenderPasses{};
};
