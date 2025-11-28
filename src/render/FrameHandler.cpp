#include "FrameHandler.h"
#include "utils/PipelineHelper.h"
#include "Scene.h"
/*const*/ FrameResources &FrameHandler::getCurrentFrameData() // const
{
    //Todo: This condition shouldn't usually happen 
    //assert(mFramesData.size() < currentFrame );

    return mFramesData[currentFrame];
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

void FrameHandler::createFrameData(VkDevice device, VkPhysicalDevice physDevice, uint32_t queueIndice)
{
    auto &frameData = mFramesData[currentFrame];
    frameData.mSyncObjects.createSyncObjects(device);
    frameData.mCommandPool.createCommandPool(device, CommandPoolType::Frame, queueIndice);
    frameData.mCommandPool.createCommandBuffers(1);
    frameData.mDescriptor.createDescriptorPool(device, 4, {});

    VkDeviceSize bufferSize = sizeof(SceneData);

    // UBO
    frameData.mCameraBuffer.createBuffer(device, physDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    // Persistent Mapping
    vkMapMemory(device, frameData.mCameraBuffer.getMemory(), 0, bufferSize, 0, &frameData.mCameraMapping);
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
};

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

// Todo:
void FrameHandler::createFramesDescriptorSet(VkDevice device, const std::vector<std::vector<VkDescriptorSetLayoutBinding>> &layouts)
{
    for (const auto &layout : layouts)
    {
        // Todo:
        //  5 is just a magic number for a number of uniform that seemed "fine"
        // poolSize.push_back({layout.descriptorType, 10});
    }

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

void FrameHandler::createFramesDynamicRenderingInfo(const RenderTargetConfig &cfg,
                                                    const std::vector<VkImageView> &gbufferViews,
                                                    VkImageView depthView, const std::vector<VkImageView> swapChainViews, const VkExtent2D swapChainExtent)
{
     for (int i = 0; i < mFramesData.size(); i++)
    {
        auto &frameData = getCurrentFrameData();
        auto &renderColorInfos = frameData.mDynamicPassInfo.colorAttachments;
        auto &renderDepthInfo = frameData.mDynamicPassInfo.depthAttachment;
        auto &renderInfo = frameData.mDynamicPassInfo.info;

        // Todo: Really confusing method
        // Here we specifically handle the relevant swapChainView first
        renderColorInfos.clear();
        const VkImageView imageView = swapChainViews[getCurrentFrameIndex()];

        // SwapChain image
        VkRenderingAttachmentInfo swapchainColor;
        swapchainColor = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
        swapchainColor.imageView = imageView;

        swapchainColor.imageLayout = cfg.attachments[0].finalLayout; // target layout for rendering
        swapchainColor.loadOp = cfg.attachments[0].loadOp;
        swapchainColor.storeOp = cfg.attachments[0].storeOp;
        swapchainColor.clearValue = {{0.2f, 0.2f, 0.2f, 1.0f}};
        renderColorInfos.push_back(swapchainColor);

        for (size_t i = 0; i < gbufferViews.size(); ++i)
        {
            VkRenderingAttachmentInfo colorInfo{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            colorInfo.imageView = gbufferViews[i];
            colorInfo.imageLayout = cfg.attachments[i + 1].finalLayout; // target layout for rendering
            colorInfo.loadOp = cfg.attachments[i + 1].loadOp;
            colorInfo.storeOp = cfg.attachments[i + 1].storeOp;
            // colorInfo.clearValue = ... set per-pass per-frame
            renderColorInfos.push_back(colorInfo);
        }

        if (cfg.enableDepth && depthView != VK_NULL_HANDLE)
        {
            renderDepthInfo = {VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
            renderDepthInfo.imageView = depthView;
            renderDepthInfo.imageLayout = cfg.attachments.back().finalLayout;
            renderDepthInfo.loadOp = cfg.attachments.back().loadOp;
            renderDepthInfo.storeOp = cfg.attachments.back().storeOp;
            renderDepthInfo.clearValue = {1.0f, 0}; // Important...
        }
        // fill depth load/store...
        renderInfo = {.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                      .renderArea = {.offset = {0, 0}, .extent = swapChainExtent},
                      .layerCount = 1,
                      .colorAttachmentCount = static_cast<uint32_t>(renderColorInfos.size()),
                      .pColorAttachments = renderColorInfos.data(),
                      .pDepthAttachment = cfg.enableDepth ? &renderDepthInfo : nullptr};
        // Todo: set renderArea / layerCount / viewMask as needed
        advanceFrame();
    }
};

void FrameHandler::createFrameBuffers(VkDevice device, const std::vector<VkImageView> &attachments, VkRenderPass renderPass, const VkExtent2D swapChainExtent)
{

    for (int i = 0; i < mFramesData.size(); i++)
    {
        auto &frameBuffer = getCurrentFrameData().mFramebuffer;
        frameBuffer.createFramebuffers(device, swapChainExtent, attachments, renderPass);
        advanceFrame();
    }
}

void FrameHandler::completeFrameBuffers(VkDevice device, const std::vector<VkImageView> &attachments, VkRenderPass renderPass, const std::vector<VkImageView> swapChainViews, const VkExtent2D swapChainExtent)
{
    std::vector<VkImageView> fbAttachments(1 + attachments.size());
    // Copy from index 1 to end of the attachments
    std::copy(attachments.begin(), attachments.end(), fbAttachments.begin() + 1);

    for (int i = 0; i < mFramesData.size(); i++)
    {
        auto &frameBuffer = getCurrentFrameData().mFramebuffer;
        fbAttachments[0] = swapChainViews[getCurrentFrameIndex()];

        frameBuffer.createFramebuffers(device, swapChainExtent, fbAttachments, renderPass);
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
    //Notes: This currently fill the uniformSceneData with garbage
    //It only work for the push constants
    //Not that anyone care
    // Copy into persistently mapped buffer
    memcpy(getCurrentFrameData().mCameraMapping, &data, sizeof(glm::mat4));
};

void FrameHandler::updateUniformBuffers(VkExtent2D swapChainExtent)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
/*
    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);

    ubo.proj[1][1] *= -1;

    // Copy into persistently mapped buffer
    memcpy(getCurrentFrameData().mCameraMapping, &ubo, sizeof(ubo));*/
};

void FrameHandler::writeFramesDescriptors(VkDevice device, int setIndex)
{
    for (auto &frame : mFramesData)
    {
        auto descriptorBuffer = frame.mCameraBuffer.getDescriptor();
        std::vector<VkWriteDescriptorSet> writes = {
            vkUtils::Descriptor::makeWriteDescriptor(frame.mDescriptor.getSet(setIndex), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &descriptorBuffer)};
        frame.mDescriptor.updateDescriptorSet(device, writes);
    }
};
