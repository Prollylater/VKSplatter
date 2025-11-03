
#pragma once

// SOme need stuff is removable
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

    // Todo: Unsatisfying position
    DescriptorManager mMaterialManager;
    GBuffers mGBuffers;

    /*
    Once the archtiecture get clearer
    // Global
    vkUtils::DeletionQueue deletionQueue;

    deletionQueue.push([=]() {
        vkDestroyBuffer(device, buffer, nullptr);
    });
    deletionQueue.flush();
    */

    const GBuffers &getDepthResources() const { return mGBuffers; }
    GBuffers &getDepthResources() { return mGBuffers; }

    // Todo: Buffer should not be kept
    // std::vector<Buffer> mBufferM;
    // const std::vector<Buffer> &getBufferManager() const { return mBufferM; }
    // std::vector<Buffer> &getBufferManager() { return mBufferM; }

    // Getters returning const references
    // const VulkanInstanceManager &getInstanceManager() const { return mInstanceM; }
    // VulkanInstanceManager &getInstanceManager() { return mInstanceM; }

    const PhysicalDeviceManager &getPhysicalDeviceManager() const { return mPhysDeviceM; }
    const LogicalDeviceManager &getLogicalDeviceManager() const { return mLogDeviceM; }

    const RenderPassManager &getRenderPassManager() const { return mRenderPassM; }
    const PipelineManager &getPipelineManager() const { return mPipelineM; }
    const SwapChainManager &getSwapChainManager() const { return mSwapChainM; }

    PhysicalDeviceManager &getPhysicalDeviceManager() { return mPhysDeviceM; }
    LogicalDeviceManager &getLogicalDeviceManager() { return mLogDeviceM; }

    RenderPassManager &getRenderPassManager() { return mRenderPassM; }
    PipelineManager &getPipelineManager() { return mPipelineM; }
    SwapChainManager &getSwapChainManager() { return mSwapChainM; }

    // Not too sure about what init  or not and in private or not

    void initVulkanBase(GLFWwindow *window, ContextCreateInfo &createInfo);
    void initRenderInfrastructure();
    void initMaterialPool()
    {
        //Depending of the maximum of sets (Probably hidden in some device properties)
        //I would just have three frames sets in a pool and switch the data.
        mMaterialManager.createDescriptorPool(mLogDeviceM.getLogicalDevice(), 10, {});
    };

    void initPipelineAndDescriptors(const PipelineLayoutDescriptor &layoutConfig, VertexFlags flag);
    int requestPipeline(VkDescriptorSetLayout materialLayout,
                        const std::string &vertPath,
                        const std::string &fragPath,
                        VertexFlags flags);

    void recreateSwapchain(GLFWwindow *window);

    void destroyAll();

    // Todo: do Differently also this is probably always empty
    PipelineLayoutDescriptor mSceneLayout;

private:
    // TODO: Memory allocator, debug messenger, etc.
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
