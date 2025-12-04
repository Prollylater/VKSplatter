

#pragma once
#include "BaseVk.h"
#include "SwapChain.h"
#include "config/PipelineConfigs.h"
/*
Create/render pass definitions through a config files
*/
class RenderPassManager
{
public:
    VkRenderPass getRenderPass() const { return mRenderPass; };
    void createRenderPass(VkDevice device, const RenderPassConfig &);

    // Move this to higher function
    void startPass(const VkCommandBuffer &command, const VkFramebuffer &frameBuffer, const VkExtent2D &extent);
    void endPass(const VkCommandBuffer &);

    void destroyRenderPass(VkDevice device);

    void initConfiguration(RenderPassConfig config)
    {
        mConfiguration = config;
    }
    const RenderPassConfig &getConfiguration() const
    {
        return mConfiguration;
    }

private:
    VkDevice mDevice = VK_NULL_HANDLE;
    VkRenderPass mRenderPass = VK_NULL_HANDLE;
    RenderPassConfig mConfiguration; 
};
