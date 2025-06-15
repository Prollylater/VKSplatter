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
}

// Very long call so fare as i have yet to decide how to handle to pass so elements
void HelloTriangleApplication::initVulkan()
{
    context.initAll(window);
    renderer.associateContext(context);
}


void HelloTriangleApplication::mainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        renderer.drawFrame(framebufferResized, window);
    }
    // Make sure the program exit properly once windows is closed
    vkDeviceWaitIdle(context.mLogDeviceM.getLogicalDevice());
}

void HelloTriangleApplication::cleanup()
{
    context.destroyAll();
      glfwDestroyWindow(window);

        glfwTerminate();
}

/*

I guess for deferred rendering we would have a configuration with 4 attachement  like albedo, normal depthot create the Gbuffer
Then we read then
Part if this might be set in pipeleine

Read on render graph, that could get a pass object and read the info of a texture that will be added
*/