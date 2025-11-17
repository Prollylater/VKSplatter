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
    //Config
    ContextCreateInfo info = ContextCreateInfo::Default();
    info.selectionCriteria.requireGeometryShader = true;
    SwapChainConfig swapChain = SwapChainConfig::Default();
    info.getSwapChainConfig() = swapChain;
    //RenderStuff Info

    //Todo: Shouldn't be here but eh
    VertexFlags sceneflag = STANDARD_STATIC_FLAG;
    VertexFormatRegistry::addFormat(sceneflag);

    context.initVulkanBase(window, info);
    renderer.initialize(context, assetSystem.registry());

    // RenderTarget Info
    // RenderTargetInfo renderInfo;
    renderer.createFramesData(info.MAX_FRAMES_IN_FLIGHT,logicScene.sceneLayout.descriptorSetLayoutsBindings);
    renderer.initRenderInfrastructure();

    // Renderer
    initScene();
    renderer.initRenderingRessources(logicScene, assetSystem.registry());

    vkInitialized = true;
}

void HelloTriangleApplication::initScene()
{
    auto assetMesh = assetSystem.loadMeshWithMaterials(MODEL_PATH);
    const auto &materialIds = assetSystem.registry().get(assetMesh)->materialIds;
    SceneNode node{assetMesh, materialIds[0]};
    logicScene.addNode(node);

    // Also load Texture
    // Todo: SHould be able to load everything

    const LogicalDeviceManager &deviceM =context.getLogicalDeviceManager();
    const VmaAllocator &allocator =context.getLogicalDeviceManager().getVmaAllocator();
    const VkDevice &device =context.getLogicalDeviceManager().getLogicalDevice();
    const VkPhysicalDevice &physDevice =context.getPhysicalDeviceManager().getPhysicalDevice();
    const uint32_t indice =context.getPhysicalDeviceManager().getIndices().graphicsFamily.value();
    
    for (auto &material : materialIds)
    {
        auto mat = assetSystem.registry().get(material);
        mat->requestPipelineCreateInfo();
        
        auto text = assetSystem.registry().get(mat->albedoMap);

        //Load texture assets
        text->createTextureImage(physDevice, deviceM, TEXTURE_PATH, indice, allocator);
        text->createTextureImageView(device);
        text->createTextureSampler(device, physDevice);

    }
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