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
    initScene();
    std::cout << "Init Scene" << std::endl;

    getRenderer().initRenderingRessources(getScene(), getAssetSystem().registry(), getMaterialSystem());

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
    node.getTransform(i).setPosition(glm::vec3(1, 0, 0));
    setFieldU32(node, i, "id", 0);

    /*
    i = node.addInstance();
    node.getTransform(i).setPosition(glm::vec3(-1, 0, 0));
    setFieldU32(node, i, "id", 1);*/

    getScene().addNode(node);

    SceneNode nodeb{
        .mesh = assetMesh,
        .nodeExtents = meshAsset->bndbox};

     i = nodeb.addInstance();
    nodeb.getTransform(i).setPosition(glm::vec3(0.0, 0.0, 17.0));
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
