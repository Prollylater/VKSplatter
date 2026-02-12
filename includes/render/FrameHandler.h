#pragma once

#include "SyncObjects.h"
#include "CommandPool.h"
#include "Descriptor.h"
#include "Buffer.h"
#include "Texture.h"
#include "config/PipelineConfigs.h"

struct FrameResources
{
    CommandPoolManager mCommandPool;
    FrameSyncObjects mSyncObjects;
    DescriptorManager mDescriptor;

    //Notes: This currently avoid the use of memory manager but make "assumption"
    //I need to reevaluate whether or not this scheme is fine
    Buffer mCameraBuffer;
    void *mCameraMapping;

    //Hold a Light Packet
    Buffer mPtLightsBuffer;
    void *mPtLightMapping;

    Buffer mDirLightsBuffer;
    void *mDirLightMapping;

    Buffer mShadowBuffer;
    void *mShadowMapping;

    //Notes: Currently always 1024*1024
    Image cascadePoolArray;
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
    void createShadowTextures(LogicalDeviceManager& device, VkPhysicalDevice physDevice, uint32_t queueIndice);

    // void addFramesDescriptorSet(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding> &layouts);
    void createFramesDescriptorSet(VkDevice device, const std::vector<std::vector<VkDescriptorSetLayoutBinding>> &layouts);

    void updateUniformBuffers(glm::mat4 data);
    void writeFramesDescriptors(VkDevice device, int setIndex);

    void destroyFramesData(VkDevice device);

private:
    uint32_t currentFrame = 0;
    std::vector<FrameResources> mFramesData;

    void createFrameData(VkDevice device, VkPhysicalDevice physDevice, uint32_t queueIndice);
    void destroyFrameData(VkDevice device);
};
