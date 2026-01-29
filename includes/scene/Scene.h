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

uint32_t getInstanceID(InstanceIDAllocator &allocator, uint64_t key)
{
    auto it = allocator.idMapping.find(key);
    if (it != allocator.idMapping.end())
    {
        return it->second;
    }

    uint32_t id = allocator.nextID++;
    allocator.idMapping[key] = id;
    return id;
}

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

struct PassQueue
{
    std::vector<Drawable *> drawCalls;
    std::vector<uint32_t> pipelineIndex;
};

struct RenderPassFrame
{
    RenderPassType type;
    PassQueue queue;
};

// Temporary
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

    // Todo: Something about rebuild when an Asset become invalid
    std::vector<SceneNode> nodes;
    Extents sceneBB;
    // Camera object
    Camera camera;
    LightSystem lights;
    PipelineSetLayoutBuilder sceneLayout; // This should either become Api agnostic or be removed
};

void fitCameraToBoundingBox(Camera &camera, const Extents &box);

VisibilityFrame extractRenderFrame(const Scene &scene,
                                   const AssetRegistry &registry);

#include "MaterialSystem.h"

class RenderScene
{
public:
    RenderScene() = default;
    ~RenderScene() = default;

    void initInstanceBuffer(
        GPUResourceRegistry &registry)
    {
        constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;
        constexpr uint32_t TRANSFORM_BUFFER_KEY = 0;

        BufferDesc description;
        description.updatePolicy = BufferUpdatePolicy::Dynamic;
        description.memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        description.size = MAX_INSTANCES * MAX_FRAMES_IN_FLIGHT;
        description.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        description.ssbo = true;
        description.stride = sizeof(InstanceTransform);
        AssetID<void> instanceBufferId(TRANSFORM_BUFFER_KEY);
        // RenderScene might as well own this
        instanceBuffer = registry.addBuffer(instanceBufferId, description, 0);
        // Handle the allocations
        instanceBuffer = registry.addBuffer(instanceBufferId, description, MAX_INSTANCES);
        instanceBuffer = registry.addBuffer(instanceBufferId, description, MAX_INSTANCES * 2);
    }
    BufferKey instanceBuffer;
    std::unordered_map<uint64_t, Drawable> drawables;
    InstanceIDAllocator mInstanceAllocator;

    uint32_t getOrAssignInstance(uint64_t instanceKey)
    {
        getInstanceID(mInstanceAllocator, instanceKey);
    };

    static constexpr uint32_t MAX_INSTANCES = 1024;
};

/*
// updateFrameSync():
// - may allocate GPU resources
// - may reallocate instance buffers (not currently)
// - will uploads per-frame instance data (everytime when instancing exist or with dynamic meshes)
// - must be called before rendering
*/

void updateFrameSync(
    VulkanContext &ctx,
    RenderScene &scene,
    const VisibilityFrame &frame,
    GPUResourceRegistry &registry,
    const AssetRegistry &cpuRegistry,
    MaterialSystem matSystem, uint32_t currFrame);
    
void buildPassFrame(
    RenderPassFrame &outPass,
    RenderScene &scene,
    PipelineManager &mPipeline, int pipelineINdex);