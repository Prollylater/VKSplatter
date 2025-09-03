

#include "ContextController.h"

void VulkanContext::initVulkanBase(GLFWwindow *window)
{
    mInstanceM.createInstance();

    mInstanceM.setupDebugMessenger();

    // Surface //left alone so fare
    mSwapChainM.createSurface(mInstanceM.getInstance(), window);

    // Device
    mPhysDeviceM.pickPhysicalDevice(mInstanceM.getInstance(), mSwapChainM);
    const VkPhysicalDevice &physDevice = mPhysDeviceM.getPhysicalDevice();
    const QueueFamilyIndices &indicesFamily = mPhysDeviceM.getIndices();

    mLogDeviceM.createLogicalDevice(physDevice, indicesFamily);
    const VkDevice &device = mLogDeviceM.getLogicalDevice();

    // SwapChain
    SwapChainConfig defConfigSwapChain;
    mSwapChainM.createSwapChain(physDevice, device, window, defConfigSwapChain);

    // Not refactored SwapChain IMage Views
    mSwapChainM.createImageViews(device);
}

// Work on this
void VulkanContext::initRenderInfrastructure()
{
    std::cout << "initRenderInfrastructure" << std::endl;

    const VkDevice &device = mLogDeviceM.getLogicalDevice();
    const QueueFamilyIndices &indicesFamily = mPhysDeviceM.getIndices();

    RenderPassConfig defConfigRenderPass;
    defConfigRenderPass.colorFormat = mSwapChainM.getSwapChainImageFormat();
    defConfigRenderPass.depthFormat = mPhysDeviceM.findDepthFormat();

    mRenderPassM.createRenderPass(device, defConfigRenderPass);

    std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}};
    mDescriptorM.createDescriptorSetLayout(device, layoutBindings);

    mDepthRessources.createDepthBuffer(mLogDeviceM, mSwapChainM, mPhysDeviceM);

    mSwapChainRess.createFramebuffers(device, mSwapChainM, mDepthRessources, mRenderPassM.getRenderPass());
};

void VulkanContext::initPipelineAndDescriptors()
{
    std::cout << "initPipelineAndDescriptors" << std::endl;

    const VkDevice &device = mLogDeviceM.getLogicalDevice();
    const QueueFamilyIndices &indicesFamily = mPhysDeviceM.getIndices();

    // Just pass vkDevice in the regular way
    PipelineConfig defConfigPipeline;
    defConfigPipeline.fragShaderPath = fragPath;
    defConfigPipeline.vertShaderPath = vertPath;

    // There's no reason to keep this initialize
    mPipelineM.createGraphicsPipeline(device, mRenderPassM.getRenderPass(), defConfigPipeline,
                                      mDescriptorM.getDescriptorLat());
};

void VulkanContext::initSceneAssets() {};
// All the rest or fram eressourees
void VulkanContext::initAll(GLFWwindow *window)
{

    std::cout << "InitALl" << std::endl;
    const VkPhysicalDevice &physDevice = mPhysDeviceM.getPhysicalDevice();
    const QueueFamilyIndices &indicesFamily = mPhysDeviceM.getIndices();
    const VkDevice &device = mLogDeviceM.getLogicalDevice();

    // Texture creation
    // Should be global and scene tied
    mTextureM.createTextureImage(physDevice, mLogDeviceM, indicesFamily);
    mTextureM.createTextureImageView(device);
    mTextureM.createTextureSampler(device, physDevice);

    mDescriptorM.createDescriptorPool(device, ContextVk::contextInfo.MAX_FRAMES_IN_FLIGHT,
                                      {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, ContextVk::contextInfo.MAX_FRAMES_IN_FLIGHT},
                                       {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, ContextVk::contextInfo.MAX_FRAMES_IN_FLIGHT}}); // Coould be global
    // Frame rssoources bait

    mDescriptorM.allocateDescriptorSets(device, ContextVk::contextInfo.MAX_FRAMES_IN_FLIGHT);
    
    // Frame Ressources potential
    mSwapChainM.createFramesData(device, physDevice, indicesFamily.graphicsFamily.value());

    auto descriptorBuffer = mSwapChainM.getCurrentFrameData().mCameraBuffer.getDescriptor();
    auto descriptorTexture = mTextureM.getImage().getDescriptor();
    for (int i = 0; i < ContextVk::contextInfo.MAX_FRAMES_IN_FLIGHT; i++)
    {
        std::vector<VkWriteDescriptorSet> writes = {
            vkUtils::Descriptor::makeWriteDescriptor(mDescriptorM.getSet(i), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &descriptorBuffer),
            vkUtils::Descriptor::makeWriteDescriptor(mDescriptorM.getSet(i), 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr,&descriptorTexture) };
        mDescriptorM.updateDescriptorSet(device, i, writes);
        mSwapChainM.advanceFrame();
    }
};

void VulkanContext::destroyAll()
{

    // Have destructor call those function
    // Also why am i still passing device everywhere ?
    VkDevice device = mLogDeviceM.getLogicalDevice();

    for (auto &buffer : mBufferM)
    {
        buffer.destroyBuffer();
    }

    mDescriptorM.destroyDescriptorPools(device);

    mTextureM.destroyTexture(device);

    mDepthRessources.destroyDepthBuffer(device);
    mSwapChainRess.destroyFramebuffers(device);

    mSwapChainM.destroyFramesData(device);

    mRenderPassM.destroyRenderPass(device);
    mDescriptorM.destroyDescriptorLayout(device);
    mPipelineM.destroy(device);

    mSwapChainM.DestroyImageViews(device);
    mSwapChainM.DestroySwapChain(device);

    mLogDeviceM.DestroyDevice();
    mSwapChainM.DestroySurface();
    mInstanceM.destroyInstance();
};
////////////////////////////////////////////////////:
////////////////////////////////////////////////////:
////////////////////////////////////////////////////:
////////////////////////////////////////////////////:
////////////////////////////////////////////////////:

// Todo:  Should be handled by the swapchain also check in SwapChain Recreation
void Renderer::recreateSwapChain(VkDevice device, GLFWwindow *window)
{
    // Todo: Reintroduce this
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    // Pause application until gpu is done before recreating everything
    mContext->mLogDeviceM.waitIdle();
    mContext->mSwapChainRess.destroyFramebuffers(device);
    mContext->mSwapChainM.DestroyImageViews(device);
    mContext->mSwapChainM.DestroySwapChain(device);
    //mContext->mSwapChainM.destroyFramesData(device);

    // Swap Chain M should be able to handle this
    // But this would imply mswapCHainRess would be owned by it
    // SwapChain
    SwapChainConfig defConfigSwapChain;
    mContext->mSwapChainM.createSwapChain(mContext->mPhysDeviceM.getPhysicalDevice(), device, window, defConfigSwapChain);
    mContext->mSwapChainM.createImageViews(device);
    //mContext->mSwapChainM.createFramesData(device, mContext->mPhysDeviceM.getPhysicalDevice() ,  mContext->mPhysDeviceM.getIndices().graphicsFamily.value());
    mContext->mSwapChainRess.createFramebuffers(device, mContext->mSwapChainM, mContext->mDepthRessources, mContext->mRenderPassM.getRenderPass());
}

///////////////////////////////////
// Logical device handling
///////////////////////////////////

// Vulkan Function

// Semaphore and command buffer tied to frames
void Renderer::drawFrame(bool framebufferResized, GLFWwindow *window)
{
    // Handle fetching
    VkDevice device = mContext->mLogDeviceM.getLogicalDevice();
    SwapChainManager &chain = mContext->getSwapChainManager();

    // Todo: Not sure about exposing this as non const
    FrameResources &frameRess = chain.getCurrentFrameData();
    const uint32_t currentFrame = chain.getCurrentFrameIndex();

    // In a real app we could do other stuff while waiting on Fence
    // Or maybe multiple submitted task with each their own fence they get so that they can move unto a specific task
    frameRess.mSyncObjects.waitFenceSignal(device);

    uint32_t imageIndex;

    bool resultAcq = chain.aquireNextImage(device,
                                           frameRess.mSyncObjects.getImageAvailableSemaphore(),
                                           imageIndex);

    if (!resultAcq)
    {
        // Todo: Should be in swapchain
        recreateSwapChain(device, window);
        return;
    }

    frameRess.mSyncObjects.resetFence(device);

    // frameRess.mCommandPool.resetCommandBuffer();
    recordCommandBuffer(imageIndex);

    // Update the Uniform Buffers
    // Todo: HHHHHHHHHHHHHHHHHHHHHHHHHH
    updateUniformBuffers(currentFrame, mContext->mSwapChainM.getSwapChainExtent());

    // Submit Info set up
    mContext->mLogDeviceM.submitFrameToGQueue(
        frameRess.mCommandPool.get(),
        frameRess.mSyncObjects.getImageAvailableSemaphore(),
        frameRess.mSyncObjects.getRenderFinishedSemaphore(),
        frameRess.mSyncObjects.getInFlightFence());

    VkResult result = mContext->mLogDeviceM.presentImage(chain.GetChain(),
                                                         frameRess.mSyncObjects.getRenderFinishedSemaphore(),
                                                         imageIndex);

    // Recreate the Swap Chain if suboptimal

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
    chain.advanceFrame();
}

void Renderer::recordCommandBuffer(uint32_t imageIndex)
{
    // Handle fetching
    // Todo: Not sure about exposing this
    FrameResources &frameRess = mContext->getSwapChainManager().getCurrentFrameData();
    auto &commandPoolM = frameRess.mCommandPool;

    VkExtent2D frameExtent = mContext->mSwapChainM.getSwapChainExtent();
    const VkCommandBuffer command = commandPoolM.get();

    commandPoolM.beginRecord();
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
    firstVertex: Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
    firstInstance: Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
    */

    auto &uniqueMesh = gpuMeshes[0];
    // Currently this is the vertexBuffer
    VkBuffer vertexBuffers[] = {uniqueMesh.getStreamBuffer(0)};
    // Currently this is the index Buffer
    VkBuffer indexBuffer = uniqueMesh.getStreamBuffer(1);

    VkDeviceSize offsets[] = {0};
    // 0 is not quite right
    // I m also not quite sure how location is determined, just through the attributes description
    // 1 interleaved buffer here
    vkCmdBindVertexBuffers(command, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(command, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    const uint32_t currentFrame = mContext->getSwapChainManager().getCurrentFrameIndex();
    vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mContext->mPipelineM.getPipelineLayout(), 0, 1,
                            &mContext->mDescriptorM.getSet(currentFrame), 0,
                            nullptr);
    size_t bufferSize = uniqueMesh.getStream(1).mView.mSize/sizeof(uint32_t);
    vkCmdDrawIndexed(command, static_cast<uint32_t>(bufferSize), 1, 0, 0, 0);

    mContext->mRenderPassM.endPass(command);
    commandPoolM.endRecord();
}

void Renderer::updateUniformBuffers(uint32_t currentImage, VkExtent2D swapChainExtent)
{
    //Todo:
    // A more efficient way to pass a small buffer of data to shaders are push constants.
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);

    ubo.proj[1][1] *= -1;

    // Copy into persistently mapped buffer
    memcpy(mContext->mSwapChainM.getCurrentFrameData().mCameraMapping, &ubo, sizeof(ubo));
};

void Renderer::uploadScene(/*const Scene& scene*/)
{

    const auto &deviceM = mContext->getLogicalDeviceManager();
    const auto &physDevice = mContext->getPhysicalDeviceManager().getPhysicalDevice();
    const auto &indice = mContext->getPhysicalDeviceManager().getIndices().graphicsFamily.value();
    std::cout << "InitMesh" << std::endl;
    for (const auto &mesh : mScene.meshes)
    {
        mContext->getBufferManager().reserve(2);
        mContext->getBufferManager().push_back(Buffer());
        auto &vertexBuffer = mContext->getBufferManager().back();
        vertexBuffer.createVertexBuffers(deviceM.getLogicalDevice(), physDevice,
                                         mesh, deviceM, indice);

        mContext->getBufferManager().push_back(Buffer());
        auto &indexBuffer = mContext->getBufferManager().back();
        indexBuffer.createIndexBuffers(deviceM.getLogicalDevice(), physDevice,
                                       mesh, deviceM, indice);

        gpuMeshes.push_back(MeshGPUResources());
        auto &meshGpu = gpuMeshes.back();

        meshGpu.addStream(vertexBuffer.getView(), sizeof(glm::vec3), AttributeStream::Type::Attributes);
        meshGpu.addStream(indexBuffer.getView(), sizeof(uint32_t), AttributeStream::Type::Index);
    }

    /*
    for (const auto& texture : mScene.textures) {
        gpuTextures.push_back(context->textureManager().uploadTexture(texture));
    }*/

    // create per-material descriptor sets
    //   createMaterialDescriptorSets(scene.materials);
}
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