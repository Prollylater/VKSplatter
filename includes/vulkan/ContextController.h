
#pragma once

//SOme need stuff is removable
#include "VertexDescriptions.h"

#include "VulkanInstance.h"
#include "LogicalDevice.h"
#include "PhysicalDevice.h"
#include "SwapChain.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "CommandPool.h"

#include "SyncObjects.h"
#include "Buffer.h"


// Todo: Which heap allocation are absolutely needed
class VulkanContext
{
public:
    VulkanContext() = default;
    ~VulkanContext() = default;

    // Core Vulkan handles, direct access
    VulkanInstanceManager mInstanceM;
    PhysicalDeviceManager mPhysDeviceM;
    LogicalDeviceManager mLogDeviceM;

    RenderPassManager mRenderPassM;
    PipelineManager mPipelineM;
    SwapChainManager mSwapChainM;
    // SwapChainResources mSwapChainRess;
    DepthRessources mDepthRessources;
    const DepthRessources &getDepthResources() const { return mDepthRessources; }
    DepthRessources &getDepthResources() { return mDepthRessources; }

    // Todo: Buffer should not be kept
    std::vector<Buffer> mBufferM;

    // Getters returning const references
    // const VulkanInstanceManager &getInstanceManager() const { return mInstanceM; }
    // VulkanInstanceManager &getInstanceManager() { return mInstanceM; }

    const PhysicalDeviceManager &getPhysicalDeviceManager() const { return mPhysDeviceM; }
    const LogicalDeviceManager &getLogicalDeviceManager() const { return mLogDeviceM; }

    const RenderPassManager &getRenderPassManager() const { return mRenderPassM; }
    const PipelineManager &getPipelineManager() const { return mPipelineM; }
    const SwapChainManager &getSwapChainManager() const { return mSwapChainM; }

    const std::vector<Buffer> &getBufferManager() const { return mBufferM; }

    PhysicalDeviceManager &getPhysicalDeviceManager() { return mPhysDeviceM; }
    LogicalDeviceManager &getLogicalDeviceManager() { return mLogDeviceM; }

    RenderPassManager &getRenderPassManager() { return mRenderPassM; }
    PipelineManager &getPipelineManager() { return mPipelineM; }
    SwapChainManager &getSwapChainManager() { return mSwapChainM; }

    std::vector<Buffer> &getBufferManager() { return mBufferM; }

    // Not too sure about what init  or not and in private or not

    void initVulkanBase(GLFWwindow *window, ContextCreateInfo &createInfo);
    void initRenderInfrastructure();
    void initPipelineAndDescriptors(const PipelineLayoutDescriptor &layoutConfig, VertexFlags flag);

    void destroyAll();

private:
    // TODO: Memory allocator, debug messenger, etc.
    void initSceneAssets();
};

/*
A frame in flight refers to a rendering operation that
 has been submitted to the GPU but has not yet finished rendering in flight

    The CPU can prepare the next frame while (recordCommand Buffer)
    The GPU is still rendering the previous frame(s)
    So CPU could record Frame 1 and 2 while GPU is still on frame 0
    Cpu can then move on on more meaningful task if fence allow it

    Thingaffected:
    Frames, Uniform Buffer (Since we update and send it for each record)

    ThingUnaffected:
    OR not ? It's weird wait for the chapter
    Depth Buffer/Stencil Buffer (Only used during rendering. Sent for rendering, consumed there and  ignried)
*/

/*

| Current                                   | Suggestion                                        |
| ----------------------------------------- | ------------------------------------------------- |
| `SwapChainManager` + `SwapChainResources` | Merge into `SwapChain`                            |
| `CommandPoolManager` + `CommandBuffer`    | Combine into a `CommandSystem`                    |
| `PipelineManager` + `RenderPassManager`   | Abstract into `RenderPipeline`                    |
| `Buffer`, `Texture`, `DescriptorManager`  | Wrap in `ResourceManager` or split per asset type |
| `SyncObjects`                             | Embed into `Renderer` or `FrameContext` struct    |

*/