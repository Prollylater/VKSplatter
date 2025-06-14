#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


#include "BaseVk.h"

#include "QueueFam.h"
#include <iostream>

#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <vector>
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

//Our Application only use one physical device and one logical device


/*
As you'll see, the general pattern that object creation function parameters in Vulkan follow is:

Pointer to struct with creation info
Pointer to custom allocator callbacks, always nullptr in this tutorial
Pointer to the variable that stores the handle to the new object
*/
class HelloTriangleApplication
{
public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

    void recordCommandBuffer(uint32_t currentFrame, uint32_t imageIndex);
    void drawFrame();

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}


private:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();

    Mesh mesh;

    void recreateSwapChain(VkDevice device);

    ///////////////////////////

    //std::vector<Buffer> vertexBuffers;
    //std::vector<Buffer> indexBuffers;
    //std::vector<Image> textures;

    bool framebufferResized;
    GLFWwindow *window;
    // Help setting up the Vulkan APi
    // Describe App info, Extension and Validation Layer
    VulkanInstanceManager mInstanceM;
    SwapChainManager mSwapChainM;
    SwapChainResources mSwapChainRess;
    DepthRessources mDepthRessources;

    PhysicalDeviceManager mPhysDeviceM;
    LogicalDeviceManager mLogDeviceM;
    PipelineManager mPipelineM;
    RenderPassManager mRenderPassM;
    CommandPoolManager mCommandPoolM;
    CommandBuffer mCommandBuffer;
    SyncObjects mSyncObjM;
    Buffer mBufferM;
    DescriptorManager mDescriptorM;
    TextureManager mTextureM;
};





/*
#pragma once

#include "Buffer.h"
#include "Image.h"
#include "VulkanUtils.h"
#include "DeviceExtensions.h"

*/