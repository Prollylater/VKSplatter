#include "HelloTriangle.h"

#include "LogicalDevice.h"
#include "Inputs.h"
#include "EventsSub.h"

/*
 the general pattern that object creation function parameters in Vulkan follow is:

Pointer to struct with creation info
Pointer to custom allocator callbacks, always nullptr in this tutorial
Pointer to the variable that stores the handle to the new object
*/

// GLFW Functions
void HelloTriangleApplication::initWindow()
{
    window.init("Vulkan Cico", WIDTH, HEIGHT);
    window.setEventCallback([&](Event &e)
                            { onEvent(e); });
}

void HelloTriangleApplication::initVulkan()
{
    // Config
    ContextCreateInfo info = ContextCreateInfo::Default();
    info.selectionCriteria.requireGeometryShader = true;
    SwapChainConfig swapChain = SwapChainConfig::Default();
    info.getSwapChainConfig() = swapChain;
    // RenderStuff Info

    // Todo: Shouldn't be here but eh
    VertexFlags sceneflag = STANDARD_STATIC_FLAG;
    VertexFormatRegistry::addFormat(sceneflag);

    context.initVulkanBase(window.getGLFWWindow(), info);

    // Renderer
    renderer.initialize(context, assetSystem.registry());

    // RenderTarget Info
    // RenderTargetInfo renderInfo;
    renderer.createFramesData(info.MAX_FRAMES_IN_FLIGHT, logicScene.sceneLayout.descriptorSetLayoutsBindings);
    renderer.initRenderInfrastructure();

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

    const LogicalDeviceManager &deviceM = context.getLogicalDeviceManager();
    const VmaAllocator &allocator = context.getLogicalDeviceManager().getVmaAllocator();
    const VkDevice &device = context.getLogicalDeviceManager().getLogicalDevice();
    const VkPhysicalDevice &physDevice = context.getPhysicalDeviceManager().getPhysicalDevice();
    const uint32_t indice = context.getPhysicalDeviceManager().getIndices().graphicsFamily.value();

    for (auto &material : materialIds)
    {
        auto mat = assetSystem.registry().get(material);
        auto text = assetSystem.registry().get(mat->albedoMap);

        // Load texture assets
        text->createTextureImage(physDevice, deviceM, TEXTURE_PATH, indice, allocator);
        text->createTextureImageView(device);
        text->createTextureSampler(device, physDevice);
    }
}

void HelloTriangleApplication::mainLoop()
{
    while (window.isOpen())
    {
        // Input and stuff
        glfwPollEvents();
        renderer.drawFrame(framebufferResized, window.getGLFWWindow());
    }

    // Make sure the program exit properly once windows is closed
    context.mLogDeviceM.waitIdle();
}

void HelloTriangleApplication::onEvent(Event &event)
{
    if (event.type() == KeyPressedEvent::getStaticType())
    {
        KeyPressedEvent eventK = static_cast<KeyPressedEvent &>(event);
        if (cico::InputCode::IsKeyPressed(window.getGLFWWindow(), static_cast<cico::InputCode::KeyCode>(eventK.key)))
        {
            std::cout << "Input Key pressed " << cico::Input::keyToString(static_cast<cico::InputCode::KeyCode>(eventK.key)) << std::endl;
        }
    }
    if (event.type() == KeyReleasedEvent::getStaticType())
    {
        KeyReleasedEvent eventK = static_cast<KeyReleasedEvent &>(event);
        if (cico::InputCode::IsKeyReleased(window.getGLFWWindow(), static_cast<cico::InputCode::KeyCode>(eventK.key)))
        {
            std::cout << "Input Key released " << cico::Input::keyToString(static_cast<cico::InputCode::KeyCode>(eventK.key)) << std::endl;
        }
    }

    EventDispatcherTemp dispatcher(event);

    dispatcher.dispatch<KeyPressedEvent>([](Event &e)
                                         {
        KeyPressedEvent& keyEvent = static_cast<KeyPressedEvent&>(e);
        std::cout << "Event Key Pressed: " <<  static_cast<char>(keyEvent.key) << std::endl; });

    dispatcher.dispatch<KeyReleasedEvent>([](Event &e)
                                          {
        KeyReleasedEvent& keyEvent = static_cast<KeyReleasedEvent&>(e);
        std::cout << "Event  Key Released: " << static_cast<char>(keyEvent.key) << std::endl; });

};

void HelloTriangleApplication::cleanup()
{
    renderer.deinitSceneRessources(logicScene);
    context.destroyAll();
    window.close();
}