

#pragma once
#include "BaseVk.h"
#include "CommandPool.h"
#include "SwapChain.h"
#include "CommandPool.h"




class RenderPassManager {
public:
    RenderPassManager() = default;
    ~RenderPassManager() = default;

    VkRenderPass getRenderPass() const { return mRenderPass; }
    void create(VkDevice device, const VkFormat, const VkFormat);

   void startPass(const VkCommandBuffer &command, const VkFramebuffer&  frameBuffer, const VkExtent2D& extent);
    void endPass(const VkCommandBuffer&);

    void destroy(VkDevice device);

private:
    VkDevice mDevice = VK_NULL_HANDLE;
    VkRenderPass mRenderPass = VK_NULL_HANDLE;
};




