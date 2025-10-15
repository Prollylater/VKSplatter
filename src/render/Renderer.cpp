#include "Renderer.h"
#include "utils/PipelineHelper.h"
#include "utils/RessourceHelper.h"

/*
Thing not implementend:
Cleaning Image/Depth Stencil/Manually clean attachements
*/

void Renderer::recreateSwapChain(VkDevice device, GLFWwindow *window)
{
    // Todo: Reintroduce this
    std::cout << "Recreated Swapchain" << std::endl;
    std::cout << "Recreated Swapchain" << std::endl;
    std::cout << "Recreated Swapchain" << std::endl;
    return;
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    // Pause application until gpu is done before recreating everything
    mContext->mLogDeviceM.waitIdle();

    // SwapChainDo you call vkQueuePresentKHR every frame with the image you acquired?
    mContext->mSwapChainM.reCreateSwapChain(device, mContext->mPhysDeviceM.getPhysicalDevice(), window,
                                            mContext->mRenderPassM.getRenderPass(), mContext->mGBuffers,
                                            mContext->mPhysDeviceM.getIndices().graphicsFamily.value());
}

void Renderer::registerSceneFormat()
{
    // Renderer.cpp
    mScene.meshes.emplace_back(Mesh());
    Mesh &mesh = mScene.meshes.back();
    mesh.loadModel(MODEL_PATH);
    // Todo: Properly awful
    flag = mesh.getflag();
    VertexFormatRegistry::addFormat(mesh);
}

void Renderer::updateUniformBuffers(VkExtent2D swapChainExtent)
{
    // Todo:
    //  A more efficient way to pass a small buffer of data to shaders are push constants.
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

// All the rest or fram eressourees
void Renderer::initSceneRessources()
{
    // Resspirces allocations proper

    std::cout << "Init Scene Ressources" << std::endl;
    const VkPhysicalDevice &physDevice = mContext->getPhysicalDeviceManager().getPhysicalDevice();
    const QueueFamilyIndices &indicesFamily = mContext->getPhysicalDeviceManager().getIndices();
    const LogicalDeviceManager &deviceM = mContext->getLogicalDeviceManager();
    const VkDevice &device = mContext->getLogicalDeviceManager().getLogicalDevice();
    auto &swapChainM = mContext->getSwapChainManager();

    // Vertex Buffers
    // Index Buffers
    uploadScene();

    // Update/Store UBO
    // Pretty bad function all thing considerated

    updateUniformBuffers(swapChainM.getSwapChainExtent());

    // Texture
    // Limited custom when
    textures.push_back(Texture{});
    for (auto &texture : textures)
    {
        texture.createTextureImage(physDevice, deviceM, TEXTURE_PATH, indicesFamily);
        texture.createTextureImageView(device);
        texture.createTextureSampler(device, physDevice);
    }

    // Should be global and scene tied
    auto descriptorTexture = textures[0].getImage().getDescriptor();

    // Effective Binding of Desciptor
    for (int i = 0; i < swapChainM.GetSwapChainImageViews().size(); i++)
    {
        auto &frameData = swapChainM.getCurrentFrameData();
        auto descriptorBuffer = frameData.mCameraBuffer.getDescriptor();

        std::vector<VkWriteDescriptorSet> writes = {
            vkUtils::Descriptor::makeWriteDescriptor(frameData.mDescriptor.getSet(0), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &descriptorBuffer),
            vkUtils::Descriptor::makeWriteDescriptor(frameData.mDescriptor.getSet(0), 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &descriptorTexture)};
        frameData.mDescriptor.updateDescriptorSet(device, 0, writes);
        swapChainM.advanceFrame();
    }
};

void Renderer::uploadScene()
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
}

// Semaphore and command buffer tied to frames
void Renderer::drawFrame(bool framebufferResized, GLFWwindow *window)
{
    // Handle fetching
    VkDevice device = mContext->mLogDeviceM.getLogicalDevice();
    SwapChainManager &chain = mContext->getSwapChainManager();

    // Todo: Not sure about exposing this as non const
    FrameResources &frameRess = chain.getCurrentFrameData();

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
        // recreateSwapChain(device, window);
        // return;
    }

    frameRess.mSyncObjects.resetFence(device);

    // Update Camera
    updateUniformBuffers(mContext->mSwapChainM.getSwapChainExtent());

    //recordCommandBuffer(imageIndex);
    recordCommandBufferD(imageIndex);

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
        // recreateSwapChain(device, window);
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
    mContext->mRenderPassM.startPass(command, frameRess.mFramebuffer.GetFramebuffers().at(0), frameExtent);

    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, mContext->mPipelineM.getPipeline());

    // Command
    // Function in pipeline that check the size ?
    // Todo: THink about where to put this

    // Add this for dynamic Pipelin
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

    vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mContext->mPipelineM.getPipelineLayout(), 0, 1,
                            &frameRess.mDescriptor.getSet(0), 0,
                            nullptr);

    vkCmdPushConstants(command, mContext->mPipelineM.getPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(UniformBufferObject), frameRess.mCameraMapping);

    size_t bufferSize = uniqueMesh.getStream(1).mView.mSize / sizeof(uint32_t);
    vkCmdDrawIndexed(command, static_cast<uint32_t>(bufferSize), 1, 0, 0, 0);

    // Multi Viewport is basically this
    /*
    viewport.width = static_cast<float>(frameExtent.width);
    viewport.height = static_cast<float>(frameExtent.height/2);
    vkCmdSetViewport(command, 0, 1, &viewport);
    vkCmdDrawIndexed(command, static_cast<uint32_t>(bufferSize), 1, 0, 0, 0);
    */

    mContext->mRenderPassM.endPass(command);
    commandPoolM.endRecord();
}

// Create Transitin based on renderpassinfo when possible

void Renderer::recordCommandBufferD(uint32_t imageIndex)
{
    FrameResources &frameRess = mContext->getSwapChainManager().getCurrentFrameData();
    auto &commandPoolM = frameRess.mCommandPool;

    VkExtent2D frameExtent = mContext->mSwapChainM.getSwapChainExtent();
    const VkCommandBuffer command = commandPoolM.get();
    // Todo: Untie Frame Iamge and Other ?
    const auto &images = mContext->mSwapChainM.GetSwapChainImages();

    commandPoolM.beginRecord();

    // Transition swapchain image to COLOR_ATTACHMENT_OPTIMAL for rendering
    auto transObj = vkUtils::Texture::makeTransition(images[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    transObj.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    transObj.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    transObj.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    vkUtils::Texture::recordImageMemoryBarrier(command, transObj);
    //  Transition
  

    vkCmdBeginRendering(command, &frameRess.mDynamicPassInfo.info);
    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, mContext->mPipelineM.getPipeline());

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

    auto &uniqueMesh = gpuMeshes[0];
    // Currently this is the vertexBuffer
    VkBuffer vertexBuffers[] = {uniqueMesh.getStreamBuffer(0)};
    // Currently this is the index Buffer
    VkBuffer indexBuffer = uniqueMesh.getStreamBuffer(1);

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(command, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(command, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mContext->mPipelineM.getPipelineLayout(), 0, 1,
                            &frameRess.mDescriptor.getSet(0), 0,
                            nullptr);

    vkCmdPushConstants(command, mContext->mPipelineM.getPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(UniformBufferObject), frameRess.mCameraMapping);

    size_t bufferSize = uniqueMesh.getStream(1).mView.mSize / sizeof(uint32_t);
    vkCmdDrawIndexed(command, static_cast<uint32_t>(bufferSize), 1, 0, 0, 0);

    vkCmdEndRendering(command);

    // From here end the old ccode not using Render PAss
    //  Transition
    auto transObjb = vkUtils::Texture::makeTransition(images[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT);
    transObjb.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    transObjb.dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    transObj.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    transObj.dstAccessMask = 0;

    vkUtils::Texture::recordImageMemoryBarrier(command, transObjb);

    commandPoolM.endRecord();
}



/*
*/