#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "HelloTriangle.h"

/*
 the general pattern that object creation function parameters in Vulkan follow is:

Pointer to struct with creation info
Pointer to custom allocator callbacks, always nullptr in this tutorial
Pointer to the variable that stores the handle to the new object
*/

/*
This kind of configs could be useful
struct AttachmentConfig {
    VkFormat format;
    VkImageLayout initialLayout;
    VkImageLayout finalLayout;
    VkAttachmentLoadOp loadOp;
    VkAttachmentStoreOp storeOp;
};
*/
// GLFW Functions
void HelloTriangleApplication::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

// Very long call so fare as i have yet to decide how to handle to pass so elements
void HelloTriangleApplication::initVulkan()
{
    // Instance
    mInstanceM.CreateInstance();
    mInstanceM.SetupDebugMessenger();
    // Surface
    mSwapChainM.CreateSurface(mInstanceM.getInstance(), window);
    // Device (Selection influenced by surfce)

    mPhysDeviceM.pickPhysicalDevice(mInstanceM.getInstance(), mSwapChainM);
    const VkPhysicalDevice &physDevice = mPhysDeviceM.getPhysicalDevice();
    const QueueFamilyIndices &indicesFamily = findQueueFamilies(physDevice, mSwapChainM.GetSurface());
    mLogDeviceM.createLogicalDevice(physDevice, indicesFamily);
    const VkDevice &device = mLogDeviceM.getLogicalDevice();

    mSwapChainM.CreateSwapChain(physDevice, device, window);

    mSwapChainM.CreateImageViews(device);

    /*
    //The graphic Pipeline can  be divided into
    Shader stages: the shader modules that define the functionality of the programmable stages of the graphics pipeline
    Fixed-function state: all of the structures that define the fixed-function stages of the pipeline, like input assembly, rasterizer, viewport and color blending
    Pipeline layout: the uniform and push values referenced by the shader that can be updated at draw time
    Render pass: the attachments referenced by the pipeline stages and their usage
    */
    mRenderPassM.create(device, mSwapChainM.getSwapChainImageFormat(), mPhysDeviceM.findDepthFormat());

    mDescriptorM.createDescriptorSetLayout(device);

    if (mPipelineM.initialize(device, mRenderPassM.getRenderPass()))
    {
        mPipelineM.createGraphicsPipeline(vertPath, fragPath,
                                          mDescriptorM.getDescriptorLat());
    }


    mCommandPoolM.create(device, indicesFamily);

    mesh.positions = {
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.5f, 0.5f, 0.0f},
        {-0.5f, 0.5f, 0.0f},

        {-0.5f, -0.5f, -0.5f},
        {0.5f, -0.5f, -0.5f},
        {0.5f, 0.5f, -0.5f},
        {-0.5f, 0.5f, -0.5f}

    
    
    };

    mesh.colors = {
        {1.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 1.0f}

    };

    mesh.uvs = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f}
    };

    mesh.indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4};

   
    mesh.vertexFormatName = "pos_color_interleaved";


    //Mesh creation
    VertexFormatRegistry::initBase();

    mDepthRessources.createDepthBuffer(mLogDeviceM, mSwapChainM,mPhysDeviceM, mCommandPoolM);
    mSwapChainRess.createFramebuffers(device, mSwapChainM, mDepthRessources, mRenderPassM.getRenderPass());
    
    mTextureM.createTextureImage(device, physDevice, mLogDeviceM, mCommandPoolM, indicesFamily);
    mTextureM.createTextureImageView(device);
    mTextureM.createTextureSampler(device, physDevice);


    mBufferM.createVertexBuffers(device, physDevice, mesh, mLogDeviceM, mCommandPoolM, mPhysDeviceM.getIndices());
    mBufferM.createIndexBuffers(device, physDevice, mesh, mLogDeviceM, mCommandPoolM, mPhysDeviceM.getIndices());

    mDescriptorM.createUniformBuffers(device, physDevice);
    mDescriptorM.createDescriptorPool(device);
    mDescriptorM.createDescriptorSets(device, mTextureM);

    mCommandBuffer.createCommandBuffers(device, mCommandPoolM.get());

    mSyncObjM.createSyncObjects(device, MAX_FRAMES_IN_FLIGHT);

    // Render Pass For Drawing
}

// WHo should actually handle this ?
void HelloTriangleApplication::recreateSwapChain(VkDevice device)
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
    mSwapChainRess.destroyFramebuffers(device);
    mSwapChainM.DestroyImageViews(device);
    mSwapChainM.DestroySwapChain(device);

    // Swap Chain M should be able to handle this
    // But this would imply mswapCHainRess would be owned by it
    mSwapChainM.CreateSwapChain(mPhysDeviceM.getPhysicalDevice(), device, window);
    mSwapChainM.CreateImageViews(device);
    mSwapChainRess.createFramebuffers(device, mSwapChainM,mDepthRessources, mRenderPassM.getRenderPass());
}

///////////////////////////////////
// Logical device handling
///////////////////////////////////

void HelloTriangleApplication::recordCommandBuffer(uint32_t currentFrame, uint32_t imageIndex)
{
    mCommandBuffer.beginRecord(0, currentFrame);

    VkExtent2D frameExtent = mSwapChainM.getSwapChainExtent();

    const VkCommandBuffer command = mCommandBuffer.get(currentFrame);
    mRenderPassM.startPass(command, mSwapChainRess.GetFramebuffers()[imageIndex], frameExtent);

    vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineM.getPipeline());

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

    VkBuffer vertexBuffers[] = {mBufferM.getVBuffer()};
    VkBuffer indexBuffer = mBufferM.getIDXBuffer();

    VkDeviceSize offsets[] = {0};
    // 0 is not quite right
    // I m also not quite sure how location is determined, just through the attributes description
    // 1 interleaved buffer here
    vkCmdBindVertexBuffers(command, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(command, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    // vkCmdDraw(command, static_cast<uint32_t>(mesh.vertexCount()), 1, 0, 0);
    vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineM.getPipelineLayout(), 0, 1, &mDescriptorM.getSet(currentFrame), 0, nullptr);
    vkCmdDrawIndexed(command, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);

    // vkCmdBindIndexBuffer(command, vertexBuffers, indexOffset, VK_INDEX_TYPE_UINT32);
    mRenderPassM.endPass(command);
    mCommandBuffer.endRecord(currentFrame);
}

void HelloTriangleApplication::mainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        drawFrame();
    }
    // Make sure the program exit properly once windows is closed
    vkDeviceWaitIdle(mLogDeviceM.getLogicalDevice());
}
// Vulkan Function

uint32_t currentFrame = 0;

// Semaphore and command buffer tied to frames
void HelloTriangleApplication::drawFrame()
{
    VkDevice device = mLogDeviceM.getLogicalDevice();
    mSyncObjM.waitForFence(device, currentFrame);

    uint32_t imageIndex;
    // UINTMAX disable timeout
    bool resultAcq = mSwapChainM.aquireNextImage(device, mSyncObjM.getImageAvailableSemaphore(currentFrame),
                                                 imageIndex);
    if (!resultAcq)
    {
        recreateSwapChain(device);
        return;
    }
    // The reset Fence happen
    mSyncObjM.resetFence(device, currentFrame);

    // We draw after this once an image is available
    vkResetCommandBuffer(mCommandBuffer.get(currentFrame), 0);
    recordCommandBuffer(currentFrame, imageIndex);

    // Update the Uniform Buffers
    mDescriptorM.updateUniformBuffers(currentFrame, mSwapChainM.getSwapChainExtent());
    // Submit Info set up
    // Lot of thing to rethink here

    mLogDeviceM.submitToGraphicsQueue(
        mCommandBuffer.getCmdBufferHandle(currentFrame),
        mSyncObjM.getImageAvailableSemaphore(currentFrame),
        mSyncObjM.getRenderFinishedSemaphore(currentFrame),
        mSyncObjM.getInFlightFence(currentFrame));

    VkResult result = mLogDeviceM.presentImage(mSwapChainM.GetChain(), mSyncObjM.getRenderFinishedSemaphore(currentFrame),
                                               imageIndex);

    // Recreate the Swap Chain if suboutptimal
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
    {
        framebufferResized = false;
        recreateSwapChain(device);
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to present swap chain image!");
    }

    // Go to the next index
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void HelloTriangleApplication::cleanup()
{
    VkDevice device = mLogDeviceM.getLogicalDevice();

    mSyncObjM.destroy(device, MAX_FRAMES_IN_FLIGHT);
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
    mRenderPassM.destroy(device);

    mLogDeviceM.DestroyDevice();
    mSwapChainM.DestroySurface();
    mInstanceM.DestroyInstance();
    glfwDestroyWindow(window);

    glfwTerminate();
}

/*

I guess for deferred rendering we would have a configuration with 4 attachement  like albedo, normal depthot create the Gbuffer
Then we read then
Part if this might be set in pipeleine

Read on render graph, that could get a pass object and read the info of a texture that will be added
*/