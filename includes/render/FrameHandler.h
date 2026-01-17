#pragma once

#include "SyncObjects.h"
#include "CommandPool.h"
#include "Descriptor.h"
#include "Buffer.h"
#include "config/PipelineConfigs.h"

class SwapChainResources
{
public:
    void createFramebuffer(uint32_t index, VkDevice device, VkExtent2D extent, const std::vector<VkImageView> &attachments, VkRenderPass renderPass);
    void destroyFramebuffers(VkDevice device);

    const VkFramebuffer &getFramebuffers(uint32_t index) const;

private:
    std::array<VkFramebuffer, 6> mPassFramebuffers;
};

struct FrameResources
{
    CommandPoolManager mCommandPool;
    FrameSyncObjects mSyncObjects;
    DescriptorManager mDescriptor;

    // Todo: Mapping is now included in Buffer
    Buffer mCameraBuffer;
    void *mCameraMapping;

    //Hold a Light Packet
    Buffer mPtLightsBuffer;
    void *mPtLightMapping;

    Buffer mDirLightsBuffer;
    void *mDirLightMapping;

    // Legacy
    SwapChainResources mFramebuffer;
};

class FrameHandler
{

public:
    // Frame Data
    FrameResources &getCurrentFrameData();
    uint32_t getCurrentFrameIndex() const;
    uint32_t getFramesCount() const;

    void advanceFrame();
    void createFramesData(VkDevice device, VkPhysicalDevice physDevice, uint32_t queueIndice, uint32_t framesInFlightCount);
    // void addFramesDescriptorSet(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding> &layouts);
    void createFramesDescriptorSet(VkDevice device, const std::vector<std::vector<VkDescriptorSetLayoutBinding>> &layouts);

    // Pass the attachments and used them to create framebuffers
    void createFrameBuffers(VkDevice device, const std::vector<VkImageView> &attachments, VkRenderPass renderPass, RenderPassType type, const VkExtent2D swapChainExtent);

    // Misleading
    // This add before the framebuffer attachments images views of the swapchain then create framebuffers
    void completeFrameBuffers(VkDevice device, const std::vector<VkImageView> &attachments, VkRenderPass renderPass, RenderPassType type, const std::vector<VkImageView> swapChainViews, const VkExtent2D swapChainExtent);

    void updateUniformBuffers(glm::mat4 data);
    void writeFramesDescriptors(VkDevice device, int setIndex);

    void destroyFramesData(VkDevice device);

private:
    uint32_t currentFrame = 0;
    std::vector<FrameResources> mFramesData;

    void createFrameData(VkDevice device, VkPhysicalDevice physDevice, uint32_t queueIndice);
    void destroyFrameData(VkDevice device);
};
