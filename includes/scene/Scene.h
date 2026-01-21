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

// Todo: Move it somewhere else close to Vertex maybe in a common Tpes
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

struct RenderObjectFrame
{
    AssetID<Mesh> mesh;
    AssetID<Material> material;

    uint32_t indexOffset = 0;
    uint32_t indexCount = 0;

    std::vector<InstanceTransform> transforms;
    InstanceLayout layout;
    std::vector<uint8_t> instanceData;

    // Gpu Culling
    // Extents worldExtents;

    // For sorting
    // uint32_t pipelineIndex = 0;
};

struct RenderPassFrame
{
    RenderPassType type;
    std::vector<size_t> sortedObjectIndices;
};

struct RenderFrame
{
    std::vector<RenderObjectFrame> objects;
    std::vector<RenderPassFrame> passes;

    // Notes: Could be per pass
    SceneData sceneData;
    LightPacket lightData;
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

RenderFrame extractRenderFrame(const Scene &scene,
                                      const AssetRegistry &registry);
                                      
class RenderScene
{
public:
    RenderScene() = default;
    ~RenderScene() = default;

    std::vector<Drawable> drawables;

    struct PassRequirements
    {
        bool needsMaterial = false;
        bool needsMesh = true;
        bool needsTransform = true;
    };
    /*
enum RenderBits {
    RENDER_BIT_MAIN = 1 << 0,
    RENDER_BIT_SHADOW = 1 << 1,
    RENDER_BIT_REFLECTION = 1 << 2,
    RENDER_BIT_DEBUG = 1 << 3,
};
*/
};

/*
// updateFrameSync():
// - may allocate GPU resources
// - may reallocate instance buffers
// - will uploads per-frame instance data (everytime when instancing exist or with dynamic meshes)
// - must be called before rendering
*/
RenderScene* updateFrameSync(
    const RenderFrame &frame,
    GPUResourceRegistry &registry,
    const AssetRegistry &cpuRegistry,
    const GpuResourceUploader &uploader,
    uint32_t currFrame);

/*
RenderScene updateFrame(const RenderFrame &frame,
                        GPUResourceRegistry &registry,
                        const AssetRegistry &cpuRegistry,
                        const GpuResourceUploader &builder, uint32_t currFrame)
{
    RenderScene scene;
    for (const RenderObjectFrame &rof : frame.objects)
    {
        // Todo:
        // GPUHandle could be removed
        const auto &mesh = cpuRegistry.get(rof.mesh);
        uint32_t drawableCount = 0;

        GPUHandle<MeshGPU> meshGPU = registry.add(rof.mesh, std::function<MeshGPU()>([&]()
                                                                                     { return builder.uploadMeshGPU(rof.mesh); }));
        constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;
        InstanceLayout layout{};
        layout.stride = sizeof(InstanceTransform);

        for (const auto &submesh : mesh->submeshes)
        {
            Drawable d{};
            d.meshGPU = meshGPU;
            d.indexOffset = submesh.indexOffset;
            d.indexCount = submesh.indexCount;

            // Todo:
            // No upload here because Material are uploaded separately currently
            // Hence the lambda is never used
            // In practice, i could create a GpuHandle using the id but that's not a behavior i am clear on using.

            d.materialGPU = registry.add(mesh->materialIds[submesh.materialId],
                                         std::function<MaterialGPU()>([&]()
                                                                      { return MaterialGPU(); }));

            // Only if already exist but idk how to do it et
            //  Todo:
            //  Notes: Instance is pretty much mesh only dependant so it don't need to be nested here
            d.hotInstanceGPU = registry.addMultiFrame(rof.mesh, mesh->materialIds[submesh.materialId], 0, MAX_FRAMES_IN_FLIGHT,
                                                      std::function<InstanceGPU()>([&]()
                                                                                   { return builder.uploadInstanceGPU(reinterpret_cast<const std::vector<uint8_t> &>(rof.transforms), layout, 10, true); }));

            /*
              const auto &instanceGPU = registry.getInstances(d.hotInstanceGPU, currFrame);

// Only upload hot instance
uploader.updateInstanceGPU(
*instanceGPU,
visibleTransforms.data(),
visibleTransforms.size());
* /

// Todo: this don't need three Frame in flight
d.coldInstanceGPU = registry.addMultiFrame(rof.mesh, mesh->materialIds[submesh.materialId], 1, 1,
                                           std::function<InstanceGPU()>([&]()
                                                                        { return builder.uploadInstanceGPU(rof.instanceData, rof.layout, 10, true); }));

d.instanceCount = rof.transforms.size();
d.pipelineEntryIndex =
    scene.drawables.push_back(std::move(d));
}
}
}
// Handling null ptr
RenderScene updateFrame(
    const RenderFrame &frame,
    GPUResourceRegistry &registry,
    const AssetRegistry &cpuRegistry,
    const GpuResourceUploader &uploader,
    uint32_t currFrame)
{
    RenderScene scene;

    for (const RenderObjectFrame &rof : frame.objects)
    {
        const auto &mesh = cpuRegistry.get(rof.mesh);

        auto meshGPU = registry.add(
            rof.mesh,
            std::function<MeshGPU()>([&]
                                     { return uploader.uploadMeshGPU(rof.mesh); }));

        constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;
        InstanceLayout layout{};
        layout.stride = sizeof(InstanceTransform);
        for (const auto &submesh : mesh->submeshes)
        {
            Drawable d{};
            d.meshGPU = meshGPU;
            d.indexOffset = submesh.indexOffset;
            d.indexCount = submesh.indexCount;

            // Todo:
            // No upload here because Material are uploaded separately currently
            // Hence the lambda is never used
            d.materialGPU = registry.add(mesh->materialIds[submesh.materialId],
                                         std::function<MaterialGPU()>([&]()
                                                                      { return MaterialGPU(); }));

            d.hotInstanceGPU = registry.addMultiFrame(
                rof.mesh,
                mesh->materialIds[submesh.materialId],
                0,
                MAX_FRAMES_IN_FLIGHT,
                std::function<InstanceGPU()>([&]
                                             { return uploader.uploadInstanceGPU(
                                                   {},
                                                   layout,
                                                   rof.transforms.size()); }));

            d.coldInstanceGPU = registry.addMultiFrame(rof.mesh, mesh->materialIds[submesh.materialId], 1, 1,
                                                       std::function<InstanceGPU()>([&]()
                                                                                    { return uploader.uploadInstanceGPU(rof.instanceData, rof.layout, 10, true); }));

            InstanceGPU *gpu =
                registry.getInstances(d.hotInstanceGPU, currFrame);

            uint32_t required = rof.transforms.size();

            if (required > gpu.capacity)
            {
                uint32_t newCapacity =
                    std::max(required, gpu.capacity * 2);

                registry.reallocateInstance(
                    d.hotInstanceGPU,
                    currFrame,
                    uploader,
                    newCapacity,
                    rof.layout,
                    true);

                gpu = registry.getInstance(d.hotInstanceGPU, currFrame);
            }

            uploader.updateInstanceGPU(
                gpu,
                rof.transforms.data(),
                required);

            gpu.count = required;
            d.instanceCount = required;

            scene.drawables.push_back(std::move(d));
        }
    }

    return scene;
}
*/