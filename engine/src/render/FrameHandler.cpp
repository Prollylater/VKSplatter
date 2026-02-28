#include "FrameHandler.h"
#include "utils/PipelineHelper.h"
#include "utils/RessourceHelper.h"

#include "Scene.h" //Exist solely due to SizeOfSceneData
#include "Texture.h"
#include "config/RessourceConfigs.h"

FrameResources &FrameHandler::getCurrentFrameData() // const
{
    // Todo: This condition shouldn't usually happen
    // assert(mFramesData.size() < currentFrame );

    return mFramesData[currentFrame];
}

const FrameResources &FrameHandler::getFrameData(int index) const {
    return mFramesData[index];
};

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
        depthConfig.height = LightSystem::TEXTURE_SIZE;
        depthConfig.width = LightSystem::TEXTURE_SIZE;
        depthConfig.format = VK_FORMAT_D32_SFLOAT;
        depthConfig.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        // depthConfig.allocator = deviceM.getVmaAllocator();
        depthConfig.arrayLayers = MAX_SHDW_CASCADES;
        frameData.cascadePoolArray.createImage(depthConfig);

        vkUtils::Texture::ImageViewCreateConfig config = {
            .device = depthConfig.device,
            .image = frameData.cascadePoolArray.getImage(),
            .format = VK_FORMAT_D32_SFLOAT,
            .aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
            .viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
            .levelCount = 1,
            .baseArrayLayer = 0};

        // Texture view
        frameData.cascadePoolArray.createImageView(config);

        // We then create the sampler
        vkUtils::Texture::ImageSamplerConfig samplerConfig = {
            .device = depthConfig.device,
            .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE};

        frameData.cascadePoolArray.createImageSampler(depthConfig.device, depthConfig.physDevice);

        for (uint32_t i = 0; i < MAX_SHDW_CASCADES; i++)
        {
            vkUtils::Texture::ImageViewCreateConfig depthViewConfig = {
                .device = depthConfig.device,
                .image = frameData.cascadePoolArray.getImage(),
                .format = VK_FORMAT_D32_SFLOAT,
                .aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .levelCount = 1,
                .baseArrayLayer = i,
                .layerCount = 1};

            VkImageView imageView = vkUtils::Texture::createImageView(depthViewConfig);
            frameData.depthView.push_back(imageView);
        };
        advanceFrame();
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
};

void FrameHandler::destroyFrameData(VkDevice device)
{
    // Todo some clearing to do in destruction Descriptor
    auto &frameData = mFramesData[currentFrame];
    frameData.mSyncObjects.destroy(device);
    frameData.mCommandPool.destroyCommandPool();
    frameData.mDescriptor.destroyDescriptorLayout(device);
    frameData.mDescriptor.destroyDescriptorPool(device);

    for (auto &depth : frameData.depthView)
    {
        vkDestroyImageView(device, depth, nullptr);
    }
    frameData.cascadePoolArray.destroyImage(device);
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

void FrameHandler::createFrameDescriptor(VkDevice device, std::vector<VkDescriptorSetLayoutBinding> &layout, uint32_t frameIndex, uint32_t setIndex)
{
    auto &descriptor = mFramesData[frameIndex].mDescriptor;
    descriptor.getOrCreateSetLayout(device, layout);
    descriptor.allocateDescriptorSets(device);
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

// Todo: This is more or less just hardcoded
void FrameHandler::writeFramesDescriptors(VkDevice device, int setIndex)
{
    return;
};
