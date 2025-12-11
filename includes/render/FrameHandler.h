#pragma once

#include "SwapChain.h"

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
    void updateUniformBuffers(VkExtent2D swapChainExtent);
    void writeFramesDescriptors(VkDevice device, int setIndex);

    void destroyFramesData(VkDevice device);

private:
    uint32_t currentFrame = 0;
    std::vector<FrameResources> mFramesData;

    void createFrameData(VkDevice device, VkPhysicalDevice physDevice, uint32_t queueIndice);
    void destroyFrameData(VkDevice device);
};
