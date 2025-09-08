#pragma once

#include "ContextController.h"
#include "Texture.h"


class Scene
{
public:
    std::vector<Mesh> meshes;
    // std::vector<Material> materials;
    // std::vector<Textures> Textures;
    // std::vector<Lights> lights;
};

class Renderer
{
public:
    Renderer() = default;
    ~Renderer() = default;

    // Todo: This can be brittle, something should make it clear if not associated
    // It could also do more work ?
    void initialize(VulkanContext &context)
    {
        mContext = &context;
        //mContext->initRenderInfrastructure();
        //mContext->initPipelineAndDescriptors();

    }

    // WHo should actually handle this ?
    void recreateSwapChain(VkDevice device, GLFWwindow *window);

    void updateUniformBuffers(VkExtent2D swapChainExtent);
    void recordCommandBuffer(uint32_t imageIndex);

    void drawFrame(bool framebufferResized, GLFWwindow *window);

    void registerSceneFormat();

    void uploadScene();
    void initSceneRessources();

    VertexFlags flag;

private:
    VulkanContext *mContext;
    Scene mScene;
    std::vector<MeshGPUResources> gpuMeshes;
    std::vector<Texture> textures;

};
