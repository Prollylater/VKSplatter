#include "FrameHandler.h"
#include "utils/PipelineHelper.h"

#include "Scene.h" //Exist solely due to SizeOfSceneData

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

void FrameHandler::createFrameData(VkDevice device, VkPhysicalDevice physDevice, uint32_t queueIndice)
{
    auto &frameData = mFramesData[currentFrame];
    frameData.mSyncObjects.createSyncObjects(device);
    frameData.mCommandPool.createCommandPool(device, CommandPoolType::Frame, queueIndice);
    frameData.mCommandPool.createCommandBuffers(1);
    frameData.mDescriptor.createDescriptorPool(device, 4, {});

    //Temporary
    VkDeviceSize bufferSize = sizeof(SceneData);

    // UBO
    frameData.mCameraBuffer.createBuffer(device, physDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkDeviceSize plightBufferSize = 10 * sizeof(PointLight) + sizeof(uint32_t);
    VkDeviceSize dlightBufferSize = 10 * sizeof(DirectionalLight) + sizeof(uint32_t);

    frameData.mPtLightsBuffer.createBuffer(device, physDevice, plightBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    frameData.mDirLightsBuffer.createBuffer(device, physDevice, dlightBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Persistent Mapping
    // Todo:
    vkMapMemory(device, frameData.mCameraBuffer.getMemory(), 0, bufferSize, 0, &frameData.mCameraMapping);
    frameData.mPtLightMapping = frameData.mPtLightsBuffer.map();
    frameData.mDirLightMapping = frameData.mDirLightsBuffer.map();
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

//Todo: Remove this method
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

// Todo: Inconsistent  order of argupments
void FrameHandler::createFrameBuffers(VkDevice device, const std::vector<VkImageView> &attachments, VkRenderPass renderPass, RenderPassType type, const VkExtent2D swapChainExtent)
{

    for (int i = 0; i < mFramesData.size(); i++)
    {
        auto &frameBuffer = getCurrentFrameData().mFramebuffer;
        frameBuffer.createFramebuffer(static_cast<size_t>(type), device, swapChainExtent, attachments, renderPass);
        advanceFrame();
    }
}

// This one isn't much good either
void FrameHandler::completeFrameBuffers(VkDevice device, const std::vector<VkImageView> &attachments, VkRenderPass renderPass, RenderPassType type, const std::vector<VkImageView> swapChainViews, const VkExtent2D swapChainExtent)
{
    std::vector<VkImageView> fbAttachments(1 + attachments.size());
    // Copy from index 1 to end of the attachments
    std::copy(attachments.begin(), attachments.end(), fbAttachments.begin() + 1);

    for (int i = 0; i < mFramesData.size(); i++)
    {
        auto &frameBuffer = getCurrentFrameData().mFramebuffer;
        fbAttachments[0] = swapChainViews[getCurrentFrameIndex()];

        frameBuffer.createFramebuffer(static_cast<size_t>(type), device, swapChainExtent, fbAttachments, renderPass);
        advanceFrame();
    }
}

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

//Todo: This is more or less just hardcoded
void FrameHandler::writeFramesDescriptors(VkDevice device, int setIndex)
{
    for (auto &frame : mFramesData)
    {
        auto dscrptrCam = frame.mCameraBuffer.getDescriptor();
        auto dscrptrDirLght = frame.mDirLightsBuffer.getDescriptor();
        auto dscrptrPtLght = frame.mPtLightsBuffer.getDescriptor();

        std::vector<VkWriteDescriptorSet> writes = {
            vkUtils::Descriptor::makeWriteDescriptor(frame.mDescriptor.getSet(setIndex), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &dscrptrCam),
            vkUtils::Descriptor::makeWriteDescriptor(frame.mDescriptor.getSet(setIndex), 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &dscrptrDirLght),
            vkUtils::Descriptor::makeWriteDescriptor(frame.mDescriptor.getSet(setIndex), 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &dscrptrPtLght)};

        frame.mDescriptor.updateDescriptorSet(device, writes);
    }
};

 

void SwapChainResources::createFramebuffer(uint32_t index, VkDevice device, VkExtent2D extent, const std::vector<VkImageView> &attachments, VkRenderPass renderPass)
{
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    framebufferInfo.layers = 1; // sort of array

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &mPassFramebuffers[static_cast<size_t>(index)]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create framebuffer!");
    }
}
const VkFramebuffer &SwapChainResources::getFramebuffers(uint32_t index) const
{
    return mPassFramebuffers[static_cast<size_t>(index)];
}

void SwapChainResources::destroyFramebuffers(VkDevice device)
{
    for (auto framebuffer : mPassFramebuffers)
    {
        if (framebuffer != VK_NULL_HANDLE)
        {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
    }
    mPassFramebuffers.fill(VK_NULL_HANDLE);
}