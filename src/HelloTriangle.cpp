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
    window.init("Vk Cico", WIDTH, HEIGHT);
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
    constexpr bool dynamic = true;

    renderer.initialize(context, assetSystem.registry());

    //Todo: Generalist
    //Logging and "check", throw, error handling
    renderer.initAllGbuffers({}, true);

    renderer.createFramesData(info.MAX_FRAMES_IN_FLIGHT, logicScene.sceneLayout.descriptorSetLayoutsBindings);

    if (dynamic)
    {
        RenderTargetConfig defRenderPass;
        defRenderPass.addAttachment(context.mSwapChainM.getSwapChainImageFormat().format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, AttachmentConfig::Role::Present)
            .addAttachment(context.mPhysDeviceM.findDepthFormat(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, AttachmentConfig::Role::Depth);
        renderer.addPass(RenderPassType::Forward,defRenderPass);
    }
    else
    {
        RenderPassConfig defConfigRenderPass = RenderPassConfig::defaultForward(context.mSwapChainM.getSwapChainImageFormat().format, context.mPhysDeviceM.findDepthFormat());        
        renderer.addPass(RenderPassType::Forward,defConfigRenderPass);
    }
   
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
    /*
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
    }*/
}

// Revise the separation between event dispatching and inputing
// Dragging at the very least should be event tied
struct MouseStateTemp
{
    std::array<float, 2> prevXY;
    std::array<float, 2> XY;

    bool dragging = false;
};

void HelloTriangleApplication::mainLoop()
{
    float appLastTime = clock.elapsed();
    float targetFps = 0;
    MouseStateTemp mainLoopMouseState;

    while (window.isOpen()) // Stand in for app is running for now
    {
        // Bit barbaric profiling
        const float currentTime = clock.elapsed();
        float delta = currentTime - appLastTime;
        appLastTime = currentTime;

        glfwPollEvents();
        // Temp
        Camera &cam = logicScene.getCamera();
        cam.processKeyboardMovement(delta,
                                    cico::InputCode::isKeyPressed(window.getGLFWWindow(), cico::InputCode::KeyCode::W),
                                    cico::InputCode::isKeyPressed(window.getGLFWWindow(), cico::InputCode::KeyCode::S),
                                    cico::InputCode::isKeyPressed(window.getGLFWWindow(), cico::InputCode::KeyCode::A),
                                    cico::InputCode::isKeyPressed(window.getGLFWWindow(), cico::InputCode::KeyCode::D),
                                    cico::InputCode::isKeyPressed(window.getGLFWWindow(), cico::InputCode::KeyCode::R),
                                    cico::InputCode::isKeyPressed(window.getGLFWWindow(), cico::InputCode::KeyCode::F));

        if (cico::InputCode::IsMouseButtonPressed(window.getGLFWWindow(), cico::InputCode::MouseButton::BUTTON_LEFT))
        {
            // std::cout<<"Pressed"<< std::endl;
            if (!mainLoopMouseState.dragging)
            {
                mainLoopMouseState.prevXY = cico::InputCode::getMousePosition(window.getGLFWWindow());
            }
            mainLoopMouseState.dragging = true;
            mainLoopMouseState.XY = cico::InputCode::getMousePosition(window.getGLFWWindow());
        }
        else if (cico::InputCode::IsMouseButtonReleased(window.getGLFWWindow(), cico::InputCode::MouseButton::BUTTON_LEFT))
        {
            mainLoopMouseState.dragging = false;
        }

        if (mainLoopMouseState.dragging)
        {
            const auto &currentXY = mainLoopMouseState.XY;
            const auto &prevXY = mainLoopMouseState.prevXY;

            cam.processMouseMovement(currentXY[0] - prevXY[0], currentXY[1] - prevXY[1]);
            mainLoopMouseState.prevXY = mainLoopMouseState.XY;
        }

        // Todo: Camera Mouse Movement definitly feel off
        std::cout << "Curr" << mainLoopMouseState.XY[0] << " " << mainLoopMouseState.XY[1] << " Prev "
                  << mainLoopMouseState.prevXY[0] << " " << mainLoopMouseState.prevXY[1] << "Bool " << mainLoopMouseState.dragging << std::endl;

        // Input and stuff
        // I want this pattern
        auto sceneData = logicScene.getSceneData();
        
        renderer.beginFrame(sceneData, window.getGLFWWindow());
        renderer.beginPass(RenderPassType::Forward);
        renderer.drawFrame(sceneData);
        renderer.endPass(RenderPassType::Forward);
        renderer.endFrame(framebufferResized);

        // Todo:
        // Point of this in #13 was to use a Frame Target and not hog ressources once it is reached
        // WIth the current stuff, it really just
        // float frameTime = clock.elapsedMs() - appLastTime;
        // Calcilate the remainnig millisceond to reach targetFPS (float)
        // We idle

        cico::logging::flushBuffer();
    }

    // Make sure the program exit properly once windows is closed
    context.mLogDeviceM.waitIdle();
}

void HelloTriangleApplication::onEvent(Event &event)
{

    if (event.type() == KeyPressedEvent::getStaticType())
    {
        KeyPressedEvent eventK = static_cast<KeyPressedEvent &>(event);
        if (cico::InputCode::isKeyPressed(window.getGLFWWindow(), static_cast<cico::InputCode::KeyCode>(eventK.key)))
        {
            std::cout << "Input Key pressed " << cico::Input::keyToString(static_cast<cico::InputCode::KeyCode>(eventK.key)) << std::endl;
        }
    }
    if (event.type() == KeyReleasedEvent::getStaticType())
    {
        KeyReleasedEvent eventK = static_cast<KeyReleasedEvent &>(event);
        if (cico::InputCode::isKeyReleased(window.getGLFWWindow(), static_cast<cico::InputCode::KeyCode>(eventK.key)))
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
    // Todo:
    // Due to asset Registry still holding texture GPU data Textures fail to be freed,
    // Best way to solve this is to properly separate Textures GPU and
    // Perhas removing the whole Texture class has it is more or less an helper at this point

    cico::logging::shutdown();
   
    renderer.deinitSceneRessources(logicScene);
    context.destroyAll();
    window.close();
}