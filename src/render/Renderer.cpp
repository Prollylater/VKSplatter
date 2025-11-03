#include "Renderer.h"
#include "utils/PipelineHelper.h"
#include "utils/RessourceHelper.h"
#include "Texture.h"
#include "Material.h"
/*
Thing not implementend:
Cleaning Image/Depth Stencil/Manually clean attachements
*/

void Renderer::updateUniformBuffers(VkExtent2D swapChainExtent)
{
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
    const VmaAllocator &allocator = mContext->getLogicalDeviceManager().getVmaAllocator();
    const VkDevice &device = mContext->getLogicalDeviceManager().getLogicalDevice();
    const uint32_t indice = indicesFamily.graphicsFamily.value();
    auto &swapChainM = mContext->getSwapChainManager();

    // Defer to some registry
    // Mesh reading
    mScene.registerSceneFormat();
    // Todo:Bad
    mContext->mSceneLayout = mScene.sceneLayout;
    // Texture reading
    mScene.textures.push_back(Texture{});
    for (auto &texture : mScene.textures)
    {
        texture.createTextureImage(physDevice, deviceM, TEXTURE_PATH, indice, allocator);
        texture.createTextureImageView(device);
        texture.createTextureSampler(device, physDevice);
    }

    // Materials reading
    mScene.material.push_back(Material{});
    for (auto &mat : mScene.material)
    {
        mat.albedoMap = &mScene.textures.back();
        mat.requestPipeline(*mContext, mScene.flag);
    }
    // Defer to some registry

    // Upload GPU Data
    //  Vertex Buffers
    //  Index Buffers
    uploadScene();

    // Effective Binding of Desciptor
    for (int i = 0; i < swapChainM.GetSwapChainImageViews().size(); i++)
    {
        auto &frameData = swapChainM.getCurrentFrameData();
        auto descriptorBuffer = frameData.mCameraBuffer.getDescriptor();

        std::cout<<"Updating Frame Data Set" << std::endl;
        //Todo: Should be elsewhere 
        // Update descriptor sets with actual data
        std::vector<VkWriteDescriptorSet> writes = {
            vkUtils::Descriptor::makeWriteDescriptor(frameData.mDescriptor.getSet(0), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &descriptorBuffer)};
        frameData.mDescriptor.updateDescriptorSet(device, 0, writes);
        swapChainM.advanceFrame();
    }

    // Update/Store UBO
    // Pretty bad function all thing considerated
    updateUniformBuffers(swapChainM.getSwapChainExtent());
    // Upload GPU Data
    std::cout<<"Scene Ressources Initialized"<<std::endl;
};

void Renderer::deinitSceneRessources()
{
    const auto allocator = mContext->getLogicalDeviceManager().getVmaAllocator();
    mScene.destroy(mContext->getLogicalDeviceManager().getLogicalDevice(), allocator);
}

void Renderer::uploadScene()
{

    const auto &deviceM = mContext->getLogicalDeviceManager();
    const auto &physDevice = mContext->getPhysicalDeviceManager().getPhysicalDevice();
    const auto &indice = mContext->getPhysicalDeviceManager().getIndices().graphicsFamily.value();
    const auto &allocator = mContext->getLogicalDeviceManager().getVmaAllocator();

    std::cout << "Upload Ressources" << std::endl;
    mScene.drawables.reserve(mScene.meshes.size());
    for (auto &mesh : mScene.meshes)
    {

        mScene.drawables.push_back(Drawable());
        mScene.drawables.back().mesh = &mesh;
        // Todo: SHould work like that
        //  mScene.drawables.back().material = &mesh.material;
        mScene.drawables.back().material = &mScene.material.back();
        mScene.drawables.back().createMeshGPU(deviceM, physDevice, indice);
        // Notes:
        // The above might also just be a material Id for sorting as full access to material is not needed pas upload ?
        // Same as for mesh to be honest ?

        mScene.drawables.back().createMaterialGPU(deviceM, mContext->mMaterialManager, physDevice, indice);
    }
    std::cout << "Ressourcess uploaded" << std::endl;
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

    //
    renderQueue.build(mScene);
    // recordCommandBuffer(imageIndex);
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

    for each view {
  bind global resourcees          // set 0

  for each shader {
    bind shader pipeline
    for each material {
      bind material resources  // sets 2,3
    }
  }
}
    */

    // 0 is not quite right
    // I m also not quite sure how location is determined, just through the attributes description
    // 1 interleaved buffer here

    // MESH DRAWING

    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, mContext->mPipelineM.getPipeline());

    VkDeviceSize offsets[] = {0};

    for (const Drawable *draw : renderQueue.getDrawables())
    {
        std::cout<<"Buffer Non D " << " Frame then material" << std::endl;
        std::vector<VkDescriptorSet> sets = {frameRess.mDescriptor.getSet(0),
                                             mContext->mMaterialManager.getSet(draw->materialGPU.descriptorIndex)};

        vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mContext->mPipelineM.getPipelineLayout(), 0, sets.size(),
                                sets.data(), 0,
                                nullptr);
        vkCmdBindVertexBuffers(command, 0, 1, &draw->meshGPU.vertexBuffer, offsets);
        vkCmdBindIndexBuffer(command, draw->meshGPU.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdPushConstants(command, mContext->mPipelineM.getPipelineLayout(),
                           VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(UniformBufferObject), frameRess.mCameraMapping);
        vkCmdDrawIndexed(command, draw->meshGPU.indexCount, 1, 0, 0, 0);
    }
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

// Create Transition based on renderpassinfo when possible
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

    // MESH DRAWING

    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, mContext->mPipelineM.getPipeline());

    VkDeviceSize offsets[] = {0};

    for (const Drawable *draw : renderQueue.getDrawables())
    {
        std::cout<<"Buffer D " << " Frame then material" << std::endl;

        std::vector<VkDescriptorSet> sets = {frameRess.mDescriptor.getSet(0),
                                             mContext->mMaterialManager.getSet(draw->materialGPU.descriptorIndex)};

        vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mContext->mPipelineM.getPipelineLayout(), 0, sets.size(),
                                sets.data(), 0,
                                nullptr);
        vkCmdBindVertexBuffers(command, 0, 1, &draw->meshGPU.vertexBuffer, offsets);
        vkCmdBindIndexBuffer(command, draw->meshGPU.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdPushConstants(command, mContext->mPipelineM.getPipelineLayout(),
                           VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(UniformBufferObject), frameRess.mCameraMapping);
        vkCmdDrawIndexed(command, draw->meshGPU.indexCount, 1, 0, 0, 0);
    }
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
