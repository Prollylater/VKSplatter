#pragma once

#include "Scene.h"
#include "RenderQueue.h"
#include "SwapChain.h"
#include "FrameHandler.h"

class VulkanContext;

// Todo:Inherit from frame data handler class ? Or framehandler object (better)

//Todo: Renderer might be too "FrontEndy + BackEndy"
//Either the whole Class might just become RendererVulkan e

class Renderer
{
public:
//Todo remove those 
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
    int requestPipeline(const PipelineLayoutConfig& config,
                              const std::string &vertexPath,
                              const std::string &fragmentPath);

private:
//Todo: Typically all  that here is really specific too Vulkan
//OpenGL wouldn't need it
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
