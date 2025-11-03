#pragma once

#include "ContextController.h"

#include "Scene.h"
#include "RenderQueue.h"
#include "SwapChain.h"



class Renderer
{
public:
    Renderer() = default;
    ~Renderer() = default;

    // It could also do more work ?
    void initialize(VulkanContext &context)
    {
        //Todo: Pass the context rather than injecting it
        mContext = &context;
    }

  
    void updateUniformBuffers(VkExtent2D swapChainExtent);
    void recordCommandBuffer(uint32_t imageIndex);
    void recordCommandBufferD(uint32_t imageIndex);

    void drawFrame(bool framebufferResized, GLFWwindow *window);

    void uploadScene();
    void initSceneRessources();
    void deinitSceneRessources();
    VertexFlags flag;

private:
    VulkanContext *mContext;
    Scene mScene;
    RenderQueue renderQueue;
    std::vector<MeshGPU> gpuMeshes;
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