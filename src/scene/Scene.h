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
#include "geometry/Frustum.h"

// Notes: This is a cleaner replacement
// But both method need to be rethought.
// A proper ECS Would remove this need
struct InstanceIDAllocator
{
    uint32_t nextID = 0;
    std::unordered_map<uint64_t, uint32_t> idMapping;
};

uint32_t getInstanceID(InstanceIDAllocator &allocator, uint64_t key);

// Todo: Move it somewhere else close to Vertex maybe in a common Tpes
// Todo: Handle more visibility, typically light visible object
// Something
struct SceneData
{
    glm::mat4 viewproj;
    glm::vec4 eye;
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

struct ShadowPacket
{
    std::vector<std::array<Cascade, LightSystem::MAX_CASCADES>> cascades;
    uint32_t shadowCount;

    static constexpr uint32_t shadowSize = sizeof(Cascade);
};

// Skip if GPU culling ?
struct VisibleObject
{
    AssetID<Mesh> mesh;
    struct VisibleInstance
    {
        uint64_t instanceKey;
        InstanceTransform transform; // only filled if needsUpload
        bool transformDirty;
    };

    std::vector<VisibleInstance> visibleInstances;

    /*
    InstanceLayout layout;
    std::vector<uint8_t> instanceData;
*/
    // Gpu Culling
    // Extents worldExtents;

    // For sorting
    // uint32_t pipelineIndex = 0;
};

// Todo:  Better name
struct VisibilityFrame
{
    std::vector<VisibleObject> objects;
    // Visibility perqueue here too maybe
    //  Notes: Could be per pass
    SceneData sceneData;
    LightPacket lightData;
};

// Notes:
//  Scene do need to be more complete and perhaps the main vector of change
//  Due to pipeline Layout
//  I am considering making Scene a base class interface
//
class Scene
{
public:
    Scene();
    explicit Scene(int compute);

    ~Scene() = default;

    void addNode(SceneNode node);
    const SceneNode &getNode(uint32_t index);

    void clearScene();
    Camera &getCamera();
    SceneData getSceneData() const;
    LightPacket getLightPacket() const;
    ShadowPacket getShadowPacket() const;

    void updateLights(float deltaTime);
    void updateShadows();

    // Todo: Something about rebuild when an Asset become invalid
    std::vector<SceneNode> nodes;
    Extents sceneBB;
    Camera camera;
    LightSystem lights;
    PipelineSetLayoutBuilder sceneLayout; // This should either become Api agnostic or be removed
};

void fitCameraToBoundingBox(Camera &camera, const Extents &box);

VisibilityFrame extractRenderFrame(const Scene &scene,
                                   const AssetRegistry &registry);
