
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

    RenderPassManager mRenderPassM;
    PipelineManager mPipelineM;
    SwapChainManager mSwapChainM;
    SwapChainResources mSwapChainRess;
    DepthRessources mDepthRessources;

    Buffer mBufferM;
    DescriptorManager mDescriptorM;
    TextureManager mTextureM;

    // Getters returning const references
    const VulkanInstanceManager &getInstanceManager() const { return mInstanceM; }
    const PhysicalDeviceManager &getPhysicalDeviceManager() const { return mPhysDeviceM; }
    const LogicalDeviceManager &getLogicalDeviceManager() const { return mLogDeviceM; }

    const RenderPassManager &getRenderPassManager() const { return mRenderPassM; }
    const PipelineManager &getPipelineManager() const { return mPipelineM; }
    const SwapChainManager &getSwapChainManager() const { return mSwapChainM; }

    const Buffer &getBufferManager() const { return mBufferM; }
    const DescriptorManager &getDescriptorManager() const { return mDescriptorM; }
    const TextureManager &getTextureManager() const { return mTextureM; }

    VulkanInstanceManager &getInstanceManager() { return mInstanceM; }
    PhysicalDeviceManager &getPhysicalDeviceManager() { return mPhysDeviceM; }
    LogicalDeviceManager &getLogicalDeviceManager() { return mLogDeviceM; }

    RenderPassManager &getRenderPassManager() { return mRenderPassM; }
    PipelineManager &getPipelineManager() { return mPipelineM; }
    SwapChainManager &getSwapChainManager() { return mSwapChainM; }

    const SwapChainResources &getSwapChainResources() const { return mSwapChainRess; }
    const DepthRessources &getDepthResources() const { return mDepthRessources; }

    SwapChainResources &getSwapChainResources() { return mSwapChainRess; }
    DepthRessources &getDepthResources() { return mDepthRessources; }

    Buffer &getBufferManager() { return mBufferM; }
    DescriptorManager &getDescriptorManager() { return mDescriptorM; }
    TextureManager &getTextureManager() { return mTextureM; }

    // Not too sure about what init  or not and in private or not

    void initVulkanBase(GLFWwindow *window);
    void initRenderInfrastructure();
    void initPipelineAndDescriptors();

    void initAll(GLFWwindow *window);
    void destroyAll();

private:
    // TODO: Memory allocator, debug messenger, etc.

    void initSceneAssets();
};

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
    void associateContext(VulkanContext &context)
    {
        mContext = &context;
    }
    // WHo should actually handle this ?
    void recreateSwapChain(VkDevice device, GLFWwindow *window);

    void updateUniformBuffers(uint32_t currentImage, VkExtent2D swapChainExtent);

    void drawFrame(bool framebufferResized, GLFWwindow *window);
    void recordCommandBuffer(uint32_t imageIndex);

    void registerSceneFormat(/*const Scene& scene*/)
    {
        // Renderer.cpp
        mScene.meshes.emplace_back(Mesh());
        Mesh &mesh = mScene.meshes.back();
        mesh.loadModel(MODEL_PATH);
        mesh.inputFlag = static_cast<VertexFlags>(Vertex_Pos | Vertex_Normal | Vertex_UV | Vertex_Color);
        VertexFormatRegistry::addFormat(mesh);
    }

    void uploadScene(/*const Scene& scene*/)
    {

        // gpuMesh = context.bufferManager().uploadMesh(mesh, format);

        for (const auto &mesh : mScene.meshes)
        {
            mContext->getBufferManager().uploadMesh(mesh, mContext->getLogicalDeviceManager(), mContext->getPhysicalDeviceManager());
            //   gpuMeshes.push_back(mContext->bufferManager().uploadMesh(mesh));
        }

        /*
        for (const auto& texture : mScene.textures) {
            gpuTextures.push_back(context->textureManager().uploadTexture(texture));
        }*/

        // create per-material descriptor sets
        //   createMaterialDescriptorSets(scene.materials);
    }
    // requestDescriptor text
private:
    VulkanContext *mContext;
    Scene mScene;
    // std::vector<GpuMesh> gpuMeshes;
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



    //With perhaps an Asset Manager class that deakl with Mesh then Texture and materials Not for now.
    //Then something for The UBO, that change each Frames (We suppose they all do for now)
    //Descriptor & pipelines

    //Command Buffer logic
    // Render is the oruiruty
    // maybe: void setScene(Scene* scene);
    ...
};


*/