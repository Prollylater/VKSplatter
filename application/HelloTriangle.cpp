#include "HelloTriangle.h"
#include "LogicalDevice.h"
#include "Inputs.h"
#include "EventsSub.h"
#include "Renderer.h"
#include "WindowVk.h"

/*
 the general pattern that object creation function parameters in Vulkan follow is:

Pointer to struct with creation info
Pointer to custom allocator callbacks, always nullptr in this tutorial
Pointer to the variable that stores the handle to the new object
*/

void HelloTriangleApplication::setup()
{
    getRenderer().initAllGbuffers({}, true);
    constexpr bool useDynamic = true;

    VkFormat swapChainFormat = VK_FORMAT_B8G8R8A8_SRGB;
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

    // Descriptor definition
    PipelineSetLayoutBuilder frameLayout;
    // Notes: Could have a default
    frameLayout.addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
        .addDescriptor(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .addDescriptor(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .addDescriptor(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .addDescriptor(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
        .addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SceneData));

    // Init Frame Descriptor set of Scene instance Buffer
    PipelineSetLayoutBuilder instanceSSBOLayout;
    instanceSSBOLayout.addDescriptor(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

    // Backing buffer
    BufferDesc cameraDesc{sizeof(SceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, BufferUpdatePolicy::Dynamic, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "CameraUBO"};
    BufferDesc dLightBuffDesc{(10 * sizeof(DirectionalLight) + sizeof(uint32_t)), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, BufferUpdatePolicy::Dynamic, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "DirLghtUBO"};
    BufferDesc ptLightBuffDesc{(10 * sizeof(PointLight) + sizeof(uint32_t)), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, BufferUpdatePolicy::Dynamic, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "PtLghtSSBO"};
    BufferDesc shdwBuffDesc{(MAX_SHDW_CASCADES * sizeof(Cascade)), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, BufferUpdatePolicy::Dynamic, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, "ShdwSSBO"};

    std::vector<ResourceLink> globalResources = {{0, cameraDesc}, {1, dLightBuffDesc}, {2, ptLightBuffDesc}, {4, shdwBuffDesc}};

    // 4. Resolve and Allocate
    getRenderer().createFramesData(mEngineSpec.MAX_FRAMES_IN_FLIGHT);

    getRenderer().createDescriptorSet(DescriptorScope::Global, frameLayout.bindings);
    getRenderer().createDescriptorSet(DescriptorScope::Instances, instanceSSBOLayout.bindings);

    getRenderer().setDescriptorSet(DescriptorScope::Global, globalResources);
    // getRenderer().setDescriptorSet(DescriptorScope::Instances, );

    // Render Passes
    if (useDynamic)
    {
        // Depth only passes
        RenderPassConfig defShadowPass;
        defShadowPass.addAttachment(AttachmentSource::FrameLocal(0),
                                    depthFormat,
                                    VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                    VK_ATTACHMENT_LOAD_OP_CLEAR,
                                    VK_ATTACHMENT_STORE_OP_STORE,
                                    AttachmentConfig::Role::Depth)
            .addSubpass()
            .useDepthAttachment(0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

        RenderPassConfig defRenderPass;
        defRenderPass.addAttachment(AttachmentSource::Swapchain(),
                                    swapChainFormat,
                                    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                    VK_ATTACHMENT_LOAD_OP_CLEAR,
                                    VK_ATTACHMENT_STORE_OP_STORE,
                                    AttachmentConfig::Role::Present)
            .addAttachment(AttachmentSource::GBuffer(0),
                           depthFormat,
                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                           VK_ATTACHMENT_LOAD_OP_CLEAR,
                           VK_ATTACHMENT_STORE_OP_DONT_CARE,
                           AttachmentConfig::Role::Depth)
            .addSubpass()
            .useColorAttachment(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            .useDepthAttachment(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

        getRenderer().addPass(RenderPassType::Shadow, defShadowPass);
        getRenderer().addPass(RenderPassType::Forward, defRenderPass);
    }
    else
    {
        RenderPassConfig defConfigRenderPass = RenderPassConfig::defaultForward(
            swapChainFormat, depthFormat);
        defConfigRenderPass.setRenderingType(RenderConfigType::LegacyRenderPass);
        getRenderer().addPass(RenderPassType::Forward, defConfigRenderPass);
    }

    initScene();
    std::cout << "Init Scene" << std::endl;

    getRenderer().initRenderingRessources(getScene(), getAssetSystem().registry(), getMaterialSystem(), frameLayout.pushConstants);

    fitCameraToBoundingBox(getScene().getCamera(), getScene().sceneBB);
};

// const std::string MODEL_PATH = "hearthspring.obj";
const std::string MODEL_PATH = "sibenik.obj";
const std::string MODEL_PATH2 = "bmw.obj";

void HelloTriangleApplication::initScene()
{

    auto &assetSystem = getAssetSystem();
    auto assetMesh = assetSystem.loadMeshWithMaterials(cico::fs::meshes() / MODEL_PATH);
    const Mesh *meshAsset = assetSystem.registry().get(assetMesh);

    SceneNode node{
        .mesh = assetMesh,
        .nodeExtents = meshAsset->bndbox};

    InstanceLayout meshLayout;
    meshLayout.fields.push_back({"id", InstanceFieldType::Uint32, 0, sizeof(uint32_t)});
    meshLayout.stride = sizeof(uint32_t);
    node.layout = meshLayout;

    uint32_t i = node.addInstance();
    node.getTransform(i).setPosition(glm::vec3(0, 0, 0));
    getScene().addNode(node);

    SceneNode nodeb{
        .mesh = assetMesh,
        .nodeExtents = meshAsset->bndbox};
    i = nodeb.addInstance();
    nodeb.getTransform(i).setPosition(glm::vec3(0.0, 0.0, 16.0));
    getScene().addNode(nodeb);
}

void HelloTriangleApplication::render()
{
    auto &renderer = getRenderer();
    VisibilityFrame frameData = extractRenderFrame(getScene(), getAssetSystem().registry());
    renderer.updateRenderingScene(frameData, getAssetSystem().registry(), getMaterialSystem());

    std::cout << "Camera Position" << frameData.sceneData.eye[0] << " "
              << frameData.sceneData.eye[1] << " "
              << frameData.sceneData.eye[2] << " "
              << std::endl;

    renderer.beginFrame(frameData.sceneData, getWindow().getGLFWWindow());

    renderer.beginPass(RenderPassType::Shadow);
    renderer.drawFrame(RenderPassType::Shadow, frameData.sceneData, getMaterialSystem().materialDescriptor());
    renderer.endPass(RenderPassType::Shadow);

    renderer.beginPass(RenderPassType::Forward);
    renderer.drawFrame(RenderPassType::Forward, frameData.sceneData, getMaterialSystem().materialDescriptor());
    renderer.endPass(RenderPassType::Forward);

    renderer.endFrame(framebufferResized);
}

// Revise the separation between event dispatching and inputing
// Dragging at the very least should be event tied
struct MouseStateTemp
{
    std::array<float, 2> prevXY;
    std::array<float, 2> XY;

    bool dragging = false;
};

void HelloTriangleApplication::update(float dt)
{
    static MouseStateTemp mainLoopMouseState;
    Camera &cam = getScene().getCamera();

    if (cico::InputCode::isKeyPressed(getWindow().getGLFWWindow(), cico::InputCode::KeyCode::P))
    {
        cam.setMvmtSpd(cam.getMvmtSpd() * 1.1f);
    }

    if (cico::InputCode::isKeyPressed(getWindow().getGLFWWindow(), cico::InputCode::KeyCode::M))
    {
        cam.setMvmtSpd(cam.getMvmtSpd() * 0.9);
    }

    cam.processKeyboardMovement(dt,
                                cico::InputCode::isKeyPressed(getWindow().getGLFWWindow(), cico::InputCode::KeyCode::W),
                                cico::InputCode::isKeyPressed(getWindow().getGLFWWindow(), cico::InputCode::KeyCode::S),
                                cico::InputCode::isKeyPressed(getWindow().getGLFWWindow(), cico::InputCode::KeyCode::A),
                                cico::InputCode::isKeyPressed(getWindow().getGLFWWindow(), cico::InputCode::KeyCode::D),
                                cico::InputCode::isKeyPressed(getWindow().getGLFWWindow(), cico::InputCode::KeyCode::R),
                                cico::InputCode::isKeyPressed(getWindow().getGLFWWindow(), cico::InputCode::KeyCode::F));

    if (cico::InputCode::IsMouseButtonPressed(getWindow().getGLFWWindow(), cico::InputCode::MouseButton::BUTTON_LEFT))
    {
        // std::cout<<"Pressed"<< std::endl;
        if (!mainLoopMouseState.dragging)
        {
            mainLoopMouseState.prevXY = cico::InputCode::getMousePosition(getWindow().getGLFWWindow());
        }
        mainLoopMouseState.dragging = true;
        mainLoopMouseState.XY = cico::InputCode::getMousePosition(getWindow().getGLFWWindow());
    }
    else if (cico::InputCode::IsMouseButtonReleased(getWindow().getGLFWWindow(), cico::InputCode::MouseButton::BUTTON_LEFT))
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

    //Update descritpor buffer
    getScene().updateLights(dt);
    getScene().updateShadows();

    LightPacket lights = getScene().getLightPacket();
    ShadowPacket shadow = getScene().getShadowPacket();

    const auto sceneData = getScene().getSceneData();
    getRenderer().updateBuffer("CameraUBO", &sceneData, sizeof(SceneData));
    const size_t sizeDirLght = lights.directionalCount * lights.dirLigthSize + sizeof(lights.directionalCount);
    const size_t sizePtLght = lights.pointCount * lights.pointLightSize + sizeof(lights.pointCount);
    getRenderer().updateBuffer("DirLghtUBO", &lights, sizeDirLght);
    getRenderer().updateBuffer("PtLghtSSBO", &lights + sizeDirLght, sizePtLght);
    //getRenderer().updateBuffer("ShdwSSBO", );
}

void HelloTriangleApplication::onEvent(Event &event)
{

    if (event.type() == KeyPressedEvent::getStaticType())
    {
        KeyPressedEvent eventK = static_cast<KeyPressedEvent &>(event);
        if (cico::InputCode::isKeyPressed(getWindow().getGLFWWindow(), static_cast<cico::InputCode::KeyCode>(eventK.key)))
        {
            std::cout << "Input Key pressed " << cico::Input::keyToString(static_cast<cico::InputCode::KeyCode>(eventK.key)) << std::endl;
        }
    }
    if (event.type() == KeyReleasedEvent::getStaticType())
    {
        KeyReleasedEvent eventK = static_cast<KeyReleasedEvent &>(event);
        if (cico::InputCode::isKeyReleased(getWindow().getGLFWWindow(), static_cast<cico::InputCode::KeyCode>(eventK.key)))
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
