#include "FrameHandler.h"
#include "utils/PipelineHelper.h"

#include "Scene.h" //Exist solely due to SizeOfSceneData
#include "Texture.h"
#include "config/RessourceConfigs.h"
FrameResources &FrameHandler::getCurrentFrameData() // const
{
    // Todo: This condition shouldn't usually happen
    // assert(mFramesData.size() < currentFrame );

    return mFramesData[currentFrame];
}

uint32_t FrameHandler::getFramesCount() const
{
    return mFramesData.size();
}

uint32_t FrameHandler::getCurrentFrameIndex() const
{
    return currentFrame;
}

void FrameHandler::advanceFrame()
{
    currentFrame++;

    if (currentFrame >= mFramesData.size())
    {
        currentFrame = 0;
    }
}

// Todo! with scene light being added, this will need to become more dynamic
void FrameHandler::createFramesData(VkDevice device, VkPhysicalDevice physDevice, uint32_t queueIndice, uint32_t frameInFlightCount)
{
    mFramesData.resize(frameInFlightCount);
    currentFrame = 0;
    for (int i = 0; i < mFramesData.size(); i++)
    {
        createFrameData(device, physDevice, queueIndice);
        advanceFrame();
    }
};

// Same thing this
void FrameHandler::createShadowTextures(LogicalDeviceManager &deviceM, VkPhysicalDevice physDevice, uint32_t queueIndice)
{
    // Todo: Handle deleting

    for (auto &frameData : mFramesData)
    {
        vkUtils::Texture::ImageCreateConfig depthConfig;
        depthConfig.device = deviceM.getLogicalDevice();
        depthConfig.physDevice = physDevice;
        depthConfig.height = CascadedShadow::TEXTURE_SIZE;
        depthConfig.width = CascadedShadow::TEXTURE_SIZE;
        depthConfig.format = VK_FORMAT_D32_SFLOAT;
        depthConfig.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        depthConfig.allocator = deviceM.getVmaAllocator();
        depthConfig.arrayLayers = CascadedShadow::MAX_CASCADES;
        frameData.cascadePoolArray.createImage(depthConfig);

        vkUtils::Texture::ImageViewCreateConfig config = {
            .device = depthConfig.device,
            .image = frameData.cascadePoolArray.getImage(),
            .format = VK_FORMAT_D32_SFLOAT,
            .aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
            .viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
            .levelCount = 1,
            .baseArrayLayer = 1};

        frameData.cascadePoolArray.createImageView(config);
        advanceFrame();

        // We then create the sampler
        vkUtils::Texture::ImageSamplerConfig samplerConfig = {
            .device = depthConfig.device,
            .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE};

        frameData.cascadePoolArray.createImageSampler(depthConfig.device, depthConfig.physDevice);

        // Todo:
        // VkFrameBuffer as it stand is not compatible with this
        // We would have to create an additional VKFrameBuffer, but this lead to multiples troubles
        // Alternatively, ShadowTextures might be moved outside and being used as a GBuffers, also imply some overhaul there
    }
};

void FrameHandler::createFrameData(VkDevice device, VkPhysicalDevice physDevice, uint32_t queueIndice)
{
    auto &frameData = mFramesData[currentFrame];
    frameData.mSyncObjects.createSyncObjects(device);
    frameData.mCommandPool.createCommandPool(device, CommandPoolType::Frame, queueIndice);
    frameData.mCommandPool.createCommandBuffers(1);
    frameData.mDescriptor.createDescriptorPool(device, 5, {});

    // Temporary
    VkDeviceSize bufferSize = sizeof(SceneData);
    VkDeviceSize plightBufferSize = 10 * sizeof(PointLight) + sizeof(uint32_t);
    VkDeviceSize dlightBufferSize = 10 * sizeof(DirectionalLight) + sizeof(uint32_t);
    VkDeviceSize shadowBufferSize = CascadedShadow::MAX_CASCADES * sizeof(Cascade);

    // Use VulkanContext here
    frameData.mCameraBuffer.createBuffer(device, physDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    frameData.mPtLightsBuffer.createBuffer(device, physDevice, plightBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    frameData.mDirLightsBuffer.createBuffer(device, physDevice, dlightBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    frameData.mShadowBuffer.createBuffer(device, physDevice, shadowBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    // Persistent Mapping
    // Todo:
    vkMapMemory(device, frameData.mCameraBuffer.getMemory(), 0, bufferSize, 0, &frameData.mCameraMapping);
    frameData.mPtLightMapping = frameData.mPtLightsBuffer.map();
    frameData.mDirLightMapping = frameData.mDirLightsBuffer.map();
    frameData.mShadowMapping = frameData.mShadowBuffer.map();
};

void FrameHandler::destroyFrameData(VkDevice device)
{
    // Todo some clearing to do in destruction Descriptor
    auto &frameData = mFramesData[currentFrame];
    frameData.mSyncObjects.destroy(device);
    frameData.mCommandPool.destroyCommandPool();
    frameData.mDescriptor.destroyDescriptorLayout(device);
    frameData.mDescriptor.destroyDescriptorPool(device);
    frameData.mCameraBuffer.destroyBuffer(device);
    frameData.mPtLightsBuffer.destroyBuffer(device);
    frameData.mDirLightsBuffer.destroyBuffer(device);
};

// Todo: Remove this method
void FrameHandler::createFramesDescriptorSet(VkDevice device, const std::vector<std::vector<VkDescriptorSetLayoutBinding>> &layouts)
{
    for (int i = 0; i < mFramesData.size(); i++)
    {
        auto &descriptor = getCurrentFrameData().mDescriptor;

        for (const auto &layout : layouts)
        {
            descriptor.getOrCreateSetLayout(device, layout);
        }
        // Frame ressoources bait
        descriptor.allocateDescriptorSets(device);
        advanceFrame();
    }
};

// This delete all frame data including those used in Pipeline
// Just don't use it for now
void FrameHandler::destroyFramesData(VkDevice device)
{
    currentFrame = 0;
    for (int i = 0; i < mFramesData.size(); i++)
    {
        destroyFrameData(device);
        advanceFrame();
    }
    currentFrame = 0;
};

void FrameHandler::updateUniformBuffers(glm::mat4 data)
{
    // Notes: This currently fill the uniformSceneData with garbage since the Descriptor are differently shaped
    // It only work for the push constants
    // Not that anyone care
    // Copy into persistently mapped buffer
    memcpy(getCurrentFrameData().mCameraMapping, &data, sizeof(glm::mat4));
};

// Todo: This is more or less just hardcoded
void FrameHandler::writeFramesDescriptors(VkDevice device, int setIndex)
{
    for (auto &frame : mFramesData)
    {
        auto dscrptrCam = frame.mCameraBuffer.getDescriptor();
        auto dscrptrDirLght = frame.mDirLightsBuffer.getDescriptor();
        auto dscrptrPtLght = frame.mPtLightsBuffer.getDescriptor();
        auto dscrptrShdw = frame.mShadowBuffer.getDescriptor();
        auto descriptor = frame.cascadePoolArray.getDescriptor();
        descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        //Duplication is actually unecessary
        std::vector<VkWriteDescriptorSet> writes = {
            vkUtils::Descriptor::makeWriteDescriptor(frame.mDescriptor.getSet(setIndex), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &dscrptrCam),
            vkUtils::Descriptor::makeWriteDescriptor(frame.mDescriptor.getSet(setIndex), 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &dscrptrDirLght),
            vkUtils::Descriptor::makeWriteDescriptor(frame.mDescriptor.getSet(setIndex), 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &dscrptrPtLght),
            vkUtils::Descriptor::makeWriteDescriptor(frame.mDescriptor.getSet(setIndex), 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &descriptor),
            vkUtils::Descriptor::makeWriteDescriptor(frame.mDescriptor.getSet(setIndex), 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &dscrptrShdw)};

        frame.mDescriptor.updateDescriptorSet(device, writes);
    }
};
