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

void HelloTriangleApplication::initVulkan()
{
    ContextCreateInfo info = ContextCreateInfo::Default();
    // Non essential line, just exist
    SwapChainConfig swapChain = SwapChainConfig::Default();
    info.getSwapChainConfig() = swapChain;

    context.initVulkanBase(window, info);
    renderer.initialize(context);
    renderer.registerSceneFormat();
    context.initRenderInfrastructure();

    PipelineLayoutDescriptor layout;
    layout.addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
    layout.addDescriptor(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    layout.addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UniformBufferObject));
    context.initPipelineAndDescriptors(layout, renderer.flag);
    
    renderer.initSceneRessources();
}

void HelloTriangleApplication::mainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        renderer.drawFrame(framebufferResized, window);
    }
    // Make sure the program exit properly once windows is closed
    context.mLogDeviceM.waitIdle();
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