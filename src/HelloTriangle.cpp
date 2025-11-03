#include "HelloTriangle.h"

#include "LogicalDevice.h"

/*
 the general pattern that object creation function parameters in Vulkan follow is:

Pointer to struct with creation info
Pointer to custom allocator callbacks, always nullptr in this tutorial
Pointer to the variable that stores the handle to the new object
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
    wdwInitialized = true;
}

void HelloTriangleApplication::initVulkan()
{
    ContextCreateInfo info = ContextCreateInfo::Default();
    // Non essential line, just exist
    SwapChainConfig swapChain = SwapChainConfig::Default();
    info.getSwapChainConfig() = swapChain;

    renderer.initialize(context);

    // Context set up
    context.initVulkanBase(window, info);
    context.initMaterialPool();
    context.initRenderInfrastructure();
    renderer.initSceneRessources();
    // TODO:
    // Render Pass Buildder/ Pipelien Builder might stay in Renderer if pre existing Rnderer are made
    //  Todo better mangament of this. Like creating here and passing it to context etc...
    //  Decide if Config shoudl all belong to an unique file
   // RenderPassConfig renderPassConfig = context.getRenderPassManager().getConfiguration();
    // In the case of compute shader we should get InputAttachement Descriptor from configuration above.

   // Then i could reflect the InputAttachment from render pass Config after this
   // Second Problem is the number of RenderPass probably
   //contefxt.initPipelineAndDescriptors(layout, renderer.flag);

 
    vkInitialized = true;
}

void HelloTriangleApplication::mainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        // Input and stuff
        glfwPollEvents();
        renderer.drawFrame(framebufferResized, window);
    }

    // Make sure the program exit properly once windows is closed
    context.mLogDeviceM.waitIdle();
}

void HelloTriangleApplication::cleanup()
{
    renderer.deinitSceneRessources();
    context.destroyAll();
    glfwDestroyWindow(window);
    glfwTerminate();
}
