

#include "ContextController.h"

void VulkanContext::initAll(GLFWwindow *window)
{
    // Push to scene
    mesh.loadModel(MODEL_PATH);
    mesh.inputFlag = static_cast<VertexFlags>(Vertex_Pos | Vertex_Normal | Vertex_UV | Vertex_Color);
    // Mesh creation
    VertexFormatRegistry::addFormat(mesh);

    mInstanceM.createInstance();

    mInstanceM.setupDebugMessenger();

    // Surface //left alone so fare
    mSwapChainM.CreateSurface(mInstanceM.getInstance(), window);

    // Device
    mPhysDeviceM.pickPhysicalDevice(mInstanceM.getInstance(), mSwapChainM);
    const VkPhysicalDevice &physDevice = mPhysDeviceM.getPhysicalDevice();
    const QueueFamilyIndices &indicesFamily = mPhysDeviceM.getIndices();

    mLogDeviceM.createLogicalDevice(physDevice, indicesFamily);
    const VkDevice &device = mLogDeviceM.getLogicalDevice();

    // SwapChain
    SwapChainConfig defConfigSwapChain;
    mSwapChainM.createSwapChain(physDevice, device, window, defConfigSwapChain);

    // Not refactored
    mSwapChainM.createImageViews(device);

    /*
    //The graphic Pipeline can  be divided into
    Shader stages: the shader modules that define the functionality of the programmable stages of the graphics pipeline
    Fixed-function state: all of the structures that define the fixed-function stages of the pipeline, like input assembly, rasterizer, viewport and color blending
    Pipeline layout: the uniform and push values referenced by the shader that can be updated at draw time
    Render pass: the attachments referenced by the pipeline stages and their usage
    */

    RenderPassConfig defConfigRenderPass;
    defConfigRenderPass.colorFormat = mSwapChainM.getSwapChainImageFormat();
    defConfigRenderPass.depthFormat = mPhysDeviceM.findDepthFormat();
    mRenderPassM.createRenderPass(device, defConfigRenderPass);
    mDescriptorM.createDescriptorSetLayout(device);

    // Just pass vkDevice in the regular way
    PipelineConfig defConfigPipeline;
    defConfigPipeline.fragShaderPath = fragPath;
    defConfigPipeline.vertShaderPath = vertPath;

    // There's no reason to keep this initialize
    mPipelineM.createGraphicsPipeline(device, mRenderPassM.getRenderPass(), defConfigPipeline,
                                      mDescriptorM.getDescriptorLat());
    mCommandPoolM.create(device, indicesFamily);

    mDepthRessources.createDepthBuffer(mLogDeviceM, mSwapChainM, mPhysDeviceM, mCommandPoolM);
    mSwapChainRess.createFramebuffers(device, mSwapChainM, mDepthRessources, mRenderPassM.getRenderPass());

    //Texture creation
    mTextureM.createTextureImage(physDevice, mLogDeviceM, mCommandPoolM, indicesFamily);
    mTextureM.createTextureImageView(device);
    mTextureM.createTextureSampler(device, physDevice);

    // Some way to know the buffer states
    mBufferM.createVertexBuffers(device, physDevice, mesh, mLogDeviceM, mCommandPoolM, mPhysDeviceM.getIndices());
    mBufferM.createIndexBuffers(device, physDevice, mesh, mLogDeviceM, mCommandPoolM, mPhysDeviceM.getIndices());

    // We are here
    mDescriptorM.createUniformBuffers(device, physDevice);
    mDescriptorM.createDescriptorPool(device);
    mDescriptorM.createDescriptorSets(device, mTextureM);

    mCommandBuffer.createCommandBuffers(device, mCommandPoolM.get());

    mSyncObjM.createSyncObjects(device, ContextVk::contextInfo.MAX_FRAMES_IN_FLIGHT);

    // Render Pass For Drawing
};

void VulkanContext::initInstanceAndSurface(GLFWwindow *window) {};
void VulkanContext::initDevice() {};
void VulkanContext::initSwapChain(GLFWwindow *window) {};
void VulkanContext::initRenderPipeline() {};
void VulkanContext::initResources() {};
void VulkanContext::initDescriptors() {};
void VulkanContext::initCommandBuffers() {}
void VulkanContext::initSyncObjects() {};

void VulkanContext::destroyAll()
{

    VkDevice device = mLogDeviceM.getLogicalDevice();

    mSyncObjM.destroy(device, ContextVk::contextInfo.MAX_FRAMES_IN_FLIGHT);
    mCommandPoolM.destroy(device);

    mBufferM.destroyVertexBuffer(device);
    mBufferM.destroyIndexBuffer(device);

    mDescriptorM.destroyUniformBuffer(device);
    mDescriptorM.destroyDescriptorPools(device);

    mDescriptorM.destroyDescriptorLayout(device);

    mDepthRessources.destroyDepthBuffer(device);
    mSwapChainRess.destroyFramebuffers(device);

    mSwapChainM.DestroyImageViews(device);
    mSwapChainM.DestroySwapChain(device);

    mTextureM.destroySampler(device);
    mTextureM.destroyTextureView(device);
    mTextureM.destroyTexture(device);

    mPipelineM.destroy(device);
    mRenderPassM.destroyRenderPass(device);

    mLogDeviceM.DestroyDevice();
    mSwapChainM.DestroySurface();
    mInstanceM.destroyInstance();
};
void Renderer::recreateSwapChain(VkDevice device, GLFWwindow *window)
{

    // Handle minimization
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);
    mContext->mSwapChainRess.destroyFramebuffers(device);
    mContext->mSwapChainM.DestroyImageViews(device);
    mContext->mSwapChainM.DestroySwapChain(device);

    // Swap Chain M should be able to handle this
    // But this would imply mswapCHainRess would be owned by it
    // SwapChain
    SwapChainConfig defConfigSwapChain;
    mContext->mSwapChainM.createSwapChain(mContext->mPhysDeviceM.getPhysicalDevice(), device, window, defConfigSwapChain);
    mContext->mSwapChainM.createImageViews(device);
    mContext->mSwapChainRess.createFramebuffers(device, mContext->mSwapChainM, mContext->mDepthRessources, mContext->mRenderPassM.getRenderPass());
}

///////////////////////////////////
// Logical device handling
///////////////////////////////////

// Vulkan Function

uint32_t currentFrame = 0;

// Semaphore and command buffer tied to frames
void Renderer::drawFrame(bool framebufferResized, GLFWwindow *window)
{
    VkDevice device = mContext->mLogDeviceM.getLogicalDevice();
    mContext->mSyncObjM.waitForFence(device, currentFrame);

    uint32_t imageIndex;
    // UINTMAX disable timeout
    bool resultAcq = mContext->mSwapChainM.aquireNextImage(device, mContext->mSyncObjM.getImageAvailableSemaphore(currentFrame),
                                                           imageIndex);
    if (!resultAcq)
    {
        recreateSwapChain(device, window);
        return;
    }
    // The reset Fence happen
    mContext->mSyncObjM.resetFence(device, currentFrame);

    // We draw after this once an image is available
    vkResetCommandBuffer(mContext->mCommandBuffer.get(currentFrame), 0);
    recordCommandBuffer(currentFrame, imageIndex);

    // Update the Uniform Buffers
    updateUniformBuffers(currentFrame, mContext->mSwapChainM.getSwapChainExtent());
    // Submit Info set up
    // Lot of thing to rethink here

    mContext->mLogDeviceM.submitToGraphicsQueue(
        mContext->mCommandBuffer.getCmdBufferHandle(currentFrame),
        mContext->mSyncObjM.getImageAvailableSemaphore(currentFrame),
        mContext->mSyncObjM.getRenderFinishedSemaphore(currentFrame),
        mContext->mSyncObjM.getInFlightFence(currentFrame));

    VkResult result = mContext->mLogDeviceM.presentImage(mContext->mSwapChainM.GetChain(), mContext->mSyncObjM.getRenderFinishedSemaphore(currentFrame),
                                                         imageIndex);

    // Recreate the Swap Chain if suboutptimal
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
    {
        framebufferResized = false;
        recreateSwapChain(device, window);
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    // Go to the next index
    currentFrame = (currentFrame + 1) % ContextVk::contextInfo.MAX_FRAMES_IN_FLIGHT;
}

void Renderer::recordCommandBuffer(uint32_t currentFrame, uint32_t imageIndex)
{
    mContext->mCommandBuffer.beginRecord(0, currentFrame);

    VkExtent2D frameExtent = mContext->mSwapChainM.getSwapChainExtent();

    const VkCommandBuffer command = mContext->mCommandBuffer.get(currentFrame);
    mContext->mRenderPassM.startPass(command, mContext->mSwapChainRess.GetFramebuffers()[imageIndex], frameExtent);

    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, mContext->mPipelineM.getPipeline());

    // Add this for dynamic Pipeline( Info should be stored somewhere to be passed there)
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(frameExtent.width);
    viewport.height = static_cast<float>(frameExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(command, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = frameExtent;
    vkCmdSetScissor(command, 0, 1, &scissor);

    /*
    vertexCount: Even though we don't have a vertex buffer, we technically still have 3 vertices to draw.
    instanceCount: Used for instanced rendering, use 1 if you're not doing that.
    firstVertex: Used as an offset into the v




ertex buffer, defines the lowest value of gl_VertexIndex.
    firstInstance: Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
    */

    VkBuffer vertexBuffers[] = {mContext->mBufferM.getVBuffer()};
    VkBuffer indexBuffer = mContext->mBufferM.getIDXBuffer();

    VkDeviceSize offsets[] = {0};
    // 0 is not quite right
    // I m also not quite sure how location is determined, just through the attributes description
    // 1 interleaved buffer here
    vkCmdBindVertexBuffers(command, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(command, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    // vkCmdDraw(command, static_cast<uint32_t>(mesh.vertexCount()), 1, 0, 0);
    vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, mContext->mPipelineM.getPipelineLayout(), 0, 1, &mContext->mDescriptorM.getSet(currentFrame), 0, nullptr);
    size_t bufferSize = mContext->mBufferM.mBufferInfo.indexCount;
    vkCmdDrawIndexed(command, static_cast<uint32_t>(bufferSize), 1, 0, 0, 0);

    // vkCmdBindIndexBuffer(command, vertexBuffers, indexOffset, VK_INDEX_TYPE_UINT32);
    mContext->mRenderPassM.endPass(command);
    mContext->mCommandBuffer.endRecord(currentFrame);
}

void Renderer::updateUniformBuffers(uint32_t currentImage, VkExtent2D swapChainExtent)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);

    ubo.proj[1][1] *= -1;
    memcpy(mContext->mDescriptorM.getMappedPointer(currentImage), &ubo, sizeof(ubo));
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
    std::vector<Material> Textures;
    std::vector<Lights> lights;
    Camera camera;
     `Mesh`, `Textures`, `Materials`, `Camera`, `Lights`.
};

*/