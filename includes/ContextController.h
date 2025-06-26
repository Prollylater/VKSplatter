
#include "QueueFam.h"
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
#include "Uniforms.h"
#include "Texture.h"


class VulkanContext
{
public:
    VulkanContext() = default;
    ~VulkanContext() = default;

    // Core Vulkan handles, direct access
    VulkanInstanceManager mInstanceM;
    PhysicalDeviceManager mPhysDeviceM;
    LogicalDeviceManager mLogDeviceM;

    CommandPoolManager mCommandPoolM;
    CommandBuffer mCommandBuffer;
    RenderPassManager mRenderPassM;
    PipelineManager mPipelineM;
    SwapChainManager mSwapChainM;
    SwapChainResources mSwapChainRess;
    DepthRessources mDepthRessources;

    Buffer mBufferM;
    DescriptorManager mDescriptorM;
    TextureManager mTextureM;

    SyncObjects mSyncObjM;
    Mesh mesh;

    // Getters returning const references
    const VulkanInstanceManager &getInstanceManager() const { return mInstanceM; }
    const PhysicalDeviceManager &getPhysicalDeviceManager() const { return mPhysDeviceM; }
    const LogicalDeviceManager &getLogicalDeviceManager() const { return mLogDeviceM; }

    const CommandPoolManager &getCommandPoolManager() const { return mCommandPoolM; }
    const CommandBuffer &getCommandBuffer() const { return mCommandBuffer; }
    const RenderPassManager &getRenderPassManager() const { return mRenderPassM; }
    const PipelineManager &getPipelineManager() const { return mPipelineM; }
    const SwapChainManager &getSwapChainManager() const { return mSwapChainM; }
    const SwapChainResources &getSwapChainResources() const { return mSwapChainRess; }
    const DepthRessources &getDepthResources() const { return mDepthRessources; }

    const Buffer &getBufferManager() const { return mBufferM; }
    const DescriptorManager &getDescriptorManager() const { return mDescriptorM; }
    const TextureManager &getTextureManager() const { return mTextureM; }

    const SyncObjects &getSyncObjects() const { return mSyncObjM; }
    void initAll(GLFWwindow *window);
    void destroyAll();

private:
    // TODO: Memory allocator, debug messenger, etc.

    void initVulkanBase(GLFWwindow* window);
    void initRenderInfrastructure();
    void initPipelineAndDescriptors();
    void initSceneAssets();
    /*
    void initRenderPipeline();
    void initResources();
    void initDescriptors();
    void initCommandBuffers();
    void initSyncObjects();*/
};

class Renderer
{
public:
    Renderer() = default;
    ~Renderer() = default;

    void associateContext(VulkanContext &context)
    {
        mContext = &context;
    }
    // WHo should actually handle this ?
    void recreateSwapChain(VkDevice device, GLFWwindow *window);

    uint32_t currentFrame = 0;

    void updateUniformBuffers(uint32_t currentImage, VkExtent2D swapChainExtent);

    void drawFrame(bool framebufferResized, GLFWwindow * window);
    void recordCommandBuffer(uint32_t currentFrame, uint32_t imageIndex);

private:
    VulkanContext* mContext;
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



class Scene {
public:
    std::vector<Mesh> meshes;
    std::vector<Material> materials;
};


class Renderer {
public:
    void init(GLFWwindow* window);
    void drawFrame();
    void cleanup();

    void uploadMesh(const Mesh& mesh);
    // maybe: void setScene(Scene* scene);

private:
    VulkanInstanceManager mInstance;
    SwapChainManager mSwapChain;
    ...
};


*/