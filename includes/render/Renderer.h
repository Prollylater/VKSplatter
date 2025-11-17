#pragma once

#include "Scene.h"
#include "RenderQueue.h"
#include "SwapChain.h"
#include "FrameHandler.h"

class VulkanContext;

// Todo:Inherit from frame data handler class ? Or framehandler object (better)



class Renderer
{
public:
    Renderer() = default;
    ~Renderer() = default;

    // It could also do more work ?
    void initialize(VulkanContext &context, AssetRegistry &registry)
    {
        mContext = &context;
        mRegistry = &registry;

        auto device = context.getLogicalDeviceManager().getLogicalDevice();
       
        // Setup Pipeline
        mPipelineM.initialize(device, "");
        mMaterialDescriptors.createDescriptorPool(device, 10, {});
    }

    void recordCommandBuffer(uint32_t imageIndex);
    void recordCommandBufferD(uint32_t imageIndex);

    void drawFrame(bool framebufferResized, GLFWwindow *window);

    
    void initRenderInfrastructure();
    void initRenderingRessources(Scene &scene,const AssetRegistry& registry);
    void deinitSceneRessources( Scene &scene);
    VertexFlags flag;
    void createFramesData(uint32_t framesInFlightCount,const std::vector<VkDescriptorSetLayoutBinding>& bindings);

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
    DescriptorManager mMaterialDescriptors;
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