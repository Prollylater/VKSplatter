#include "Application.h"
#include "Events.h"

#include "BaseVk.h"
#include "Clock.h"
#include "logging/Logger.h"
#include "filesystem/Filesystem.h"
#include "ContextController.h"
#include "Renderer.h"
#include "ResourceSystem.h"
#include "MaterialSystem.h"
#include "WindowVk.h"

Application::Application()
{
    // Setup default paths
    cico::fs::setRoot(std::filesystem::current_path());
    cico::fs::setShaders(cico::fs::root() / "ressources/shaders");
    cico::fs::setTextures(cico::fs::root() / "ressources/textures");
    cico::fs::setMeshes(cico::fs::root() / "ressources/models");
}

void Application::setAssetRoot(const std::filesystem::path &root)
{
    cico::fs::setRoot(root);
}

Application::~Application()
{
    // Cleanup in reverse order
}

int Application::run()
{
    try
    {
        cico::logging::initialize("logs.txt");

        // Framework initialization
        initFramework();
        setup();
        mainLoop();
        shutdown();
        cleanup();

        return EXIT_SUCCESS;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}

void Application::initFramework()
{
    // Initialize window
    mWindow = std::make_unique<VulkanWindow>();
    mWindow->init(mWindowTitle, mWindowWidth, mWindowHeight);
    mWindow->setEventCallback([this](Event &e)
                              { onEvent(e); });

    _CINFO("Window initialized");

    // TODO: This should come from configuration
    ContextCreateInfo info = ContextCreateInfo::Default();
    info.selectionCriteria.requireGeometryShader = true;

    // Initialize Vulkan context
    initVulkan(info);

    // Initialize default systems
    mScene = std::make_unique<Scene>();//Scene in Application ?
    mAssetSystem = std::make_unique<AssetSystem>();

    mRenderer = std::make_unique<Renderer>();
    mRenderer->initialize(*mContext, mAssetSystem->registry());
 
    //Also passes definition is not much "Frameworkey"

   
    // TODO: This should come from configuration or user setup
    mRenderer->initAllGbuffers({}, true);
    mRenderer->createFramesData(info.MAX_FRAMES_IN_FLIGHT,
                                   mScene->sceneLayout.descriptorSetLayoutsBindings);
    // Setup default render pass (TODO: make configurable)

    constexpr bool useDynamic = true;
    // TODO: This should come from configuration simplified for the user
    if (useDynamic)
    {
        RenderTargetConfig defRenderPass;
        defRenderPass.addAttachment(AttachmentSource::Swapchain(),
                         mContext->mSwapChainM.getSwapChainImageFormat().format,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                         VK_ATTACHMENT_LOAD_OP_CLEAR,
                         VK_ATTACHMENT_STORE_OP_STORE,
                         AttachmentConfig::Role::Present)
            .addAttachment(AttachmentSource::GBuffer(0), 
                mContext->mPhysDeviceM.findDepthFormat(),
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_ATTACHMENT_LOAD_OP_CLEAR,
                VK_ATTACHMENT_STORE_OP_DONT_CARE,
                AttachmentConfig::Role::Depth);
        mRenderer->addPass(RenderPassType::Forward, defRenderPass);

        //Depth only passes
        RenderTargetConfig defShadowPass;
        /*defShadowPass.addAttachment(AttachmentSource::FrameLocal(0),
                         mContext->mSwapChainM.getSwapChainImageFormat().format,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                         VK_ATTACHMENT_LOAD_OP_CLEAR,
                         VK_ATTACHMENT_STORE_OP_STORE,
                         AttachmentConfig::Role::Present)*/
        defShadowPass.addAttachment(AttachmentSource::FrameLocal(0), 
                mContext->mPhysDeviceM.findDepthFormat(),
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_ATTACHMENT_LOAD_OP_CLEAR,
                VK_ATTACHMENT_STORE_OP_DONT_CARE,
                AttachmentConfig::Role::Depth);
        mRenderer->addPass(RenderPassType::Shadow, defShadowPass);
    }
    else
    {
        RenderPassConfig defConfigRenderPass = RenderPassConfig::defaultForward(
            mContext->mSwapChainM.getSwapChainImageFormat().format,
            mContext->mPhysDeviceM.findDepthFormat());
        mRenderer->addPass(RenderPassType::Forward, defConfigRenderPass);

        //Shadow pass not done here
    }

    // Todo: This pattern is off
    auto& ctx = *mContext.get();
    mMaterialSystem = std::make_unique<MaterialSystem>(ctx, mAssetSystem->registry(), mRenderer->getGPURegistry());

    _CINFO("Framework initialized");

    mClock.reset();
}

void Application::initVulkan(ContextCreateInfo& info)
{
    SwapChainConfig swapChain = SwapChainConfig::Default();
    info.getSwapChainConfig() = swapChain;

    // Vertex format registration (TODO: should be part of Scene or Renderer)
    VertexFlags sceneflag = STANDARD_STATIC_FLAG;
    VertexFormatRegistry::addFormat(sceneflag);

    mContext = std::make_unique<VulkanContext>();
    mContext->initVulkanBase(mWindow->getGLFWWindow(), info);

    _CINFO("Vulkan initialized");
}

// Revise the separation between event dispatching and inputing
// Dragging at the very least should be event tied

void Application::mainLoop()
{
    // Moved from HelloTriangleApplication::mainLoop()
    // Todo: Stand is for a proper AppIsRunning
    // while (!mWindow->shouldClose())

    float appLastTime = mClock.elapsed();

    while (mWindow->isOpen())
    {
        // mWindow->pollEvents();

        float currentTime = mClock.elapsed();
        float deltaTime = currentTime - appLastTime;
        // mAppLastTime = currentTime;

        // Todo:
        // Point of this in #13 was to use a Frame Target and not hog ressources once it is reached
        // With the current stuff, it really just
        // float frameTime = clock.elapsedMs() - appLastTime;
        // Calculate the remaining millisceond to reach targetFPS (float)
        appLastTime = currentTime;

        // Temp
        glfwPollEvents();

        // User update
        update(deltaTime);

        // Default rendering
        // if (!shouldSkipRender())
        //{
        // User render function (passes/graph currently)
        render(); // User can override
        //}

        // Handle framebuffer resize
        if (mFramebufferResized)
        {
            // TODO: Handle resize
            mFramebufferResized = false;
        }
        mContext->getLDevice().waitIdle();
    }
}

void Application::cleanup()
{
    // Todo: Name should be changed across the board
    if(mMaterialSystem){
        mMaterialSystem->materialDescriptor().destroyDescriptorLayout(mContext.get()->getLDevice().getLogicalDevice());
        mMaterialSystem->materialDescriptor().destroyDescriptorPool(mContext.get()->getLDevice().getLogicalDevice());

    }

    if (mRenderer)
    {
        mRenderer->deinitSceneRessources();
    }

    if (mContext)
    {
        mContext->destroyAll();
    }

    if (mWindow)
    {
        mWindow->close();
    }
    cico::logging::shutdown();

    _CINFO("Framework cleaned up");
}

void Application::framebufferResizeCallback(GLFWwindow *window, int width, int height)
{
    auto *app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
    if (app)
    {
        app->mFramebufferResized = true;
    }
}
