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

    renderer.initialize(context, registry);

    // Context set up
    context.initVulkanBase(window, info);

    renderer.createFramesData(info.MAX_FRAMES_IN_FLIGHT);
    renderer.initRenderInfrastructure();

    //Renderer
    renderer.initSceneRessources(logicScene);
 
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
    renderer.deinitSceneRessources(logicScene);
    context.destroyAll();
    glfwDestroyWindow(window);
    glfwTerminate();
}