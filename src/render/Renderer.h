#pragma once

#include "Scene.h"
#include "SwapChain.h"
#include "FrameHandler.h"
#include "GBuffers.h"
#include "GPUResourceSystem.h"
#include "ResourceSystem.h"
#include "Passes.h"

#include "ContextController.h"
#include "MaterialSystem.h"
#include "RenderScene.h"

class VulkanContext;

// Todo:Inherit from frame data handler class ? Or framehandler object (better)

// Todo: Renderer might be too "FrontEndy + BackEndy"
// Either the whole Class might just become RendererVulkan e

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

        auto device = context.getLDevice().getLogicalDevice();

       // mUseDynamic = true;
        // Setup Pipeline Cache and Pool
        mPipelineM.initialize(device, "");
        mGpuRegistry.initDevice(context);
        mPassesHandler.init(context, mGBuffers, mFrameHandler);
    }

    void addPass(RenderPassType type, RenderPassConfig legacyConfig)
    {
        mUseDynamic = false;
        
        mPassesHandler.addPass(type, legacyConfig);
    }

    void addPass(RenderPassType type, RenderTargetConfig dynConfig)
    {
        mUseDynamic = true;
        mPassesHandler.addPass(type, dynConfig);
    }

    void beginFrame(const SceneData &sceneData, GLFWwindow *window);
    void beginPass(RenderPassType type);
    //Type shouldn't be necessary at this point
    void drawFrame( RenderPassType type, const SceneData &sceneData, const DescriptorManager &materialDescriptor);

    void endPass(RenderPassType type);
    void endFrame(bool framebufferResized);

    // SetUp Function
    void initAllGbuffers(std::vector<VkFormat> gbufferFormats, bool depth);
    void initRenderingRessources(Scene &scene, const AssetRegistry &registry, MaterialSystem &system);
    void updateRenderingScene(const VisibilityFrame &vFrame, const AssetRegistry &registry, MaterialSystem &matSystem);
    void deinitSceneRessources();
    void createFramesData(uint32_t framesInFlightCount, const std::vector<VkDescriptorSetLayoutBinding> &bindings);

    const GBuffers &getDepthResources() const { return mGBuffers; }
    GBuffers &getDepthResources() { return mGBuffers; }

    GPUResourceRegistry &getGPURegistry() { return mGpuRegistry; }

    // GraphicPipeline
    int requestPipeline(const PipelineLayoutConfig &config,
                        const std::string &vertexPath,
                        const std::string &fragmentPath, RenderPassType type);

private:
    // Todo: Typically all  that here is really specific too Vulkan which make this Renderer not really Api Agnostic
    // OpenGL wouldn't need it
    VulkanContext *mContext;
    AssetRegistry *mRegistry; // Todo: This is never really used

    // Temp
    RenderScene mRScene;
    FrameHandler mFrameHandler;
    RenderPassHandler mPassesHandler;
    GBuffers mGBuffers;

    // Recent addition
    /// Todo: Lot of thing to discuss here
    // Notably the pattern of discussion with Material system
    GPUResourceRegistry mGpuRegistry;
    RenderPassHandler mPassHandler;
    PipelineManager mPipelineM;

    bool mUseDynamic = false;
    void initRenderInfrastructure(RenderPassType type, const RenderTargetConfig &cfg);
    void initRenderInfrastructure(RenderPassType type, const RenderPassConfig &cfg);

    // Todo: Remove
    uint32_t mIndexImage{};
};
