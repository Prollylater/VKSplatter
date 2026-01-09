#pragma once
#include "BaseVk.h"
#include "Mesh.h"
#include "Drawable.h"
#include "Material.h"
#include "GPUResource.h"
#include "AssetRegistry.h"
#include "Light.h"
#include "Camera.h"
#include "config/PipelineConfigs.h"

// Todo: Move it somewhere else close to Vertex maybe in a common Tpes
struct SceneData
{
    glm::mat4 viewproj;
    glm::vec4 view;
    glm::vec4 ambientColor;
};

// Todo: Alignment
struct LightPacket
{
    std::vector<DirectionalLight> directionalLights;
    uint32_t directionalCount;
    std::vector<PointLight> pointLights;
    uint32_t pointCount;

    static constexpr uint32_t dirLigthSize = sizeof(DirectionalLight);
    static constexpr uint32_t pointLightSize = sizeof(PointLight);
};

// Temporary
class Scene
{
public:
    Scene();
    explicit Scene(int compute);

    ~Scene() = default;

    void addNode(SceneNode node);
    void clearScene();

    Camera &getCamera();
    SceneData getSceneData();
    LightPacket getLightPacket();

    // Todo: Something about rebuild when an Asset become invalid
    std::vector<SceneNode> nodes;
    // Camera object
    Camera camera;
    LightSystem lights;
    PipelineSetLayoutBuilder sceneLayout; // This should either become Api agnostic or be removed
};

class RenderScene
{
public:
    RenderScene() = default;
    ~RenderScene() = default;
    void destroy(VkDevice device, VmaAllocator alloc);

    std::vector<Drawable> drawables;

    void syncFromScene(const Scene &cpuScene,
                       const AssetRegistry &cpuRegistry,
                       GPUResourceRegistry &registry,
                       const GpuResourceUploader &builder);

    struct PassRequirements
    {
        bool needsMaterial = false;
        bool needsMesh = true;
        bool needsTransform = true;
    };

    // Todo:
    // Would light do better here than in Frame handler ?
    /*


enum RenderBits {
    RENDER_BIT_MAIN = 1 << 0,
    RENDER_BIT_SHADOW = 1 << 1,
    RENDER_BIT_REFLECTION = 1 << 2,
    RENDER_BIT_DEBUG = 1 << 3,
};
*/
};
