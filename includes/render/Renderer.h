#pragma once

#include "Scene.h"
#include "RenderQueue.h"
#include "SwapChain.h"

class AssetRegistry;
class VulkanContext;

// Todo:Inherit from frame data handler class ? Or framehandler object (better)

class FrameHandler{

public:
    // Frame Data
    FrameResources &getCurrentFrameData();
    const int getCurrentFrameIndex() const;
    void advanceFrame();
    void createFramesData(VkDevice device, VkPhysicalDevice physDevice, uint32_t queueIndice, uint32_t framesInFlightCount);
    //void addFramesDescriptorSet(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding> &layouts);
    void createFramesDescriptorSet(VkDevice device, const std::vector<std::vector<VkDescriptorSetLayoutBinding>> &layouts);

    void createFramesDynamicRenderingInfo(const RenderTargetConfig &cfg,
                                          const std::vector<VkImageView> &colorViews,
                                          VkImageView depthView,const std::vector<VkImageView> swapChainViews, const VkExtent2D swapChainExtent);
    // Pass the attachments and used them to create framebuffers
    void createFrameBuffers(VkDevice device, const std::vector<VkImageView> &attachments, VkRenderPass renderPass, const VkExtent2D swapChainExtent);

    // Misleading
    // This add before the framebuffer attachments images views of the swapchain then create framebuffers
    void completeFrameBuffers(VkDevice device, const std::vector<VkImageView> &attachments, VkRenderPass renderPass, const std::vector<VkImageView> swapChainViews, const VkExtent2D swapChainExtent);

    void destroyFramesData(VkDevice device);

private:
    uint32_t currentFrame = 0;
    std::vector<FrameResources> mFramesData;

    void createFrameData(VkDevice device, VkPhysicalDevice physDevice, uint32_t queueIndice);
    void destroyFrameData(VkDevice device);

};


class Renderer
{
public:
    Renderer() = default;
    ~Renderer() = default;

    // It could also do more work ?
    void initialize(VulkanContext &context, AssetRegistry &registry)
    {
        // Todo: 
        mContext = &context;
        mRegistry = &registry;

        //Todo: Also
        VertexFlags sceneflag = STANDARD_STATIC_FLAG;
        VertexFormatRegistry::addFormat(sceneflag);
    }

    void updateUniformBuffers(VkExtent2D swapChainExtent);
    void recordCommandBuffer(uint32_t imageIndex);
    void recordCommandBufferD(uint32_t imageIndex);

    void drawFrame(bool framebufferResized, GLFWwindow *window);

    void uploadScene(const Scene &scene);
    
    void initRenderInfrastructure();
    void initSceneRessources(Scene &scene);
    void deinitSceneRessources( Scene &scene);
    VertexFlags flag;
    void createFramesData(uint32_t framesInFlightCount);


    const GBuffers &getDepthResources() const { return mGBuffers; }
    GBuffers &getDepthResources() { return mGBuffers; }

    //GraphicPipeline
    int requestPipeline(const PipelineLayoutConfig& config, VkDescriptorSetLayout materialLayout,
                              const std::string &vertexPath,
                              const std::string &fragmentPath);

private:
    VulkanContext *mContext;
    AssetRegistry *mRegistry;

    RenderScene mRScene;
    RenderQueue renderQueue;
    FrameHandler mFrameHandler;
    GBuffers mGBuffers;

    //Recent addition
    DescriptorManager mMaterialManager;
    RenderPassManager mRenderPassM;
    PipelineManager mPipelineM;
};

/*
for (const Drawable* draw : renderQueue.getDrawables()) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw->material->pipeline);
    vkCmdBindDescriptorSets(cmd, ... draw->material->descriptorSets ...);
    vkCmdBindVertexBuffers(cmd, 0, 1, &draw->mesh->getStreamBuffer(0), offsets);
    vkCmdBindIndexBuffer(cmd, draw->mesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
    vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &draw->transform);
    vkCmdDrawIndexed(cmd, draw->mesh->indexCount, 1, 0, 0, 0);
}

*/