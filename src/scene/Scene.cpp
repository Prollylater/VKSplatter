#include "Scene.h"

Scene::Scene()
{
    sceneLayout.addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    sceneLayout.addDescriptor(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
    sceneLayout.addDescriptor(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);

    sceneLayout.addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SceneData));

    lights.addDirLight({glm::vec4(0.5, 1.0, 0.0, 0.0), glm::vec4(1.0, 1.0, 1.0, 0.0), 1.0});
    lights.addDirLight({glm::vec4(0.5, -1.0, 0.5, 0.0), glm::vec4(0.2, 0.0, 1.0, 0.0), 1.0});
    lights.addPointLight({glm::vec4(0.5, 1.0, 0.5, 0.0), glm::vec4(1.0, 1.0, 0.0, 0.0), 0.5, 0.5});
    lights.addPointLight({glm::vec4(-0.5, 0.0, 0.5, 0.0), glm::vec4(0.0, 1.0, 1.0, 0.0), 0.5, 0.5});
};

Scene::Scene(int compute)
{
    sceneLayout.addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
    sceneLayout.addDescriptor(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
    sceneLayout.addDescriptor(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);

    sceneLayout.addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SceneData));
};

//Todo: &&
void Scene::addNode(SceneNode node)
{
    /*
    InstanceFieldHandle transformField = node.getField("transform");
    if (transformField.valid == true)
    {
        for (uint32_t i = 0; i < node.instanceCount; i++)
        {
            Extents worldExtents = node.nodeExtents;

            glm::mat4 *transformPtr = node.getFieldPtr<glm::mat4>(i, "transform");
            glm::mat4 worldMatrix = *transformPtr;

            worldExtents.translate(worldMatrix[3]);

            sceneBB.expand(worldExtents);
        }
    }
    */

    for (uint32_t i = 0; i < node.instanceCount; i++)
    {
        Extents worldExtents = node.nodeExtents;
        glm::mat4 worldMatrix = node.getTransform(i).getWorldMatrix();
        worldExtents.translate(worldMatrix[3]);
        sceneBB.expand(worldExtents);
    }
    nodes.push_back(node);
}

const SceneNode &Scene::getNode(uint32_t index)
{
    return nodes[index];
};

void Scene::clearScene()
{
    nodes.clear();
}

// Temporary
Camera &Scene::getCamera() { return camera; };
SceneData Scene::getSceneData() const
{
    glm::mat4 proj = camera.getProjectionMatrix();
    proj[1][1] *= -1;

    return {proj * camera.getViewMatrix(), camera.getEye(), glm::vec4(0.3, 0.1, 0.4, 0.0)};
};

LightPacket Scene::getLightPacket() const
{

    return {lights.getDirLights(), lights.dirLightCount(), lights.getPointLights(), lights.pointLightCount()};
}

void RenderScene::destroy(VkDevice device, VmaAllocator alloc)
{
    for (auto &drawable : drawables)
    {
        // draw gable.materialGPU.destroy(device, alloc);
        // drawable.meshGPU.destroy(device, alloc);
    };
};

void RenderScene::initFromScene(const Scene &cpuScene,
                                const AssetRegistry &cpuRegistry,
                                GPUResourceRegistry &registry,
                                const GpuResourceUploader &builder)
{
    drawables.clear();
    drawables.reserve(cpuScene.nodes.size());

    for (auto &node : cpuScene.nodes)
    {
        addDrawable(node, cpuRegistry, registry, builder);
    }
}

// What is it that identify the instances ?
void RenderScene::addDrawable(
    const SceneNode &node,
    const AssetRegistry &cpuRegistry,
    GPUResourceRegistry &registry,
    const GpuResourceUploader &builder)
{
    const auto &mesh = cpuRegistry.get(node.mesh);

    const uint32_t firstDrawable = static_cast<uint32_t>(drawables.size());
    uint32_t drawableCount = 0;

    GPUHandle<MeshGPU> meshGPU = registry.add(node.mesh, std::function<MeshGPU()>([&]()
                                                                                  { return builder.uploadMeshGPU(node.mesh); }));
    constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

    std::vector<InstanceTransform> transforms(node.instanceNb());
    for (uint32_t i = 0; i < node.instanceNb(); ++i)
    {
        transforms[i] = node.buildGPUTransform(i);
    }
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

        // Todo:
        // Notes: Instance is pretty much mesh only dependant so it don't need to be nested here
        d.hotInstanceGPU = registry.addMultiFrame(node.mesh, mesh->materialIds[submesh.materialId], 0, MAX_FRAMES_IN_FLIGHT,
                                                  std::function<InstanceGPU()>([&]()
                                                                               { return builder.uploadInstanceGPU(reinterpret_cast<const std::vector<uint8_t> &>(transforms), layout, node.instanceNb(), true); }));

        // Todo: this don't need three Frame in flight
        d.coldInstanceGPU = registry.addMultiFrame(node.mesh, mesh->materialIds[submesh.materialId], 1, 1,
                                                   std::function<InstanceGPU()>([&]()
                                                                                { return builder.uploadInstanceGPU(node.instanceData, node.layout, node.instanceNb(), true); }));

        d.instanceCount = node.instanceCount;

        drawables.push_back(std::move(d));
        ++drawableCount;
    }
}

void RenderScene::syncFromScene(
    const Scene &cpuScene,
    const AssetRegistry &cpuRegistry,
    GPUResourceRegistry &registry,
    const GpuResourceUploader &uploader, uint32_t currFrame)
{
    size_t drawableIndex = 0;

    const auto sceneData = cpuScene.getSceneData();
    // Todo: Update Transform + Bounding box
    for (const SceneNode &node : cpuScene.nodes)
    {
        if (!node.visible)
        {
            // Skip all drawables belonging to this node
            // This does not remove the drawable however
            const auto &mesh = cpuRegistry.get(node.mesh);
            drawableIndex += mesh->submeshes.size();
            continue;
        }

        const auto &mesh = cpuRegistry.get(node.mesh);

        // CPU cull & compact transforms
        std::vector<InstanceTransform> visibleTransforms;
        visibleTransforms.reserve(node.instanceNb());

        for (uint32_t i = 0; i < node.instanceNb(); ++i)
        {

              if (!testAgainstFrustum(sceneData.viewproj, node.nodeExtents))
              {
                  continue;
              }
  
            visibleTransforms.push_back(
                node.buildGPUTransform(i));
        }
        
        int visible =   visibleTransforms.size() ;
        for (size_t submesh = 0; submesh < mesh->submeshes.size(); ++submesh)
        {
            // Notes:
            // Rely heavily on the scenenodes matchign the drawable
            Drawable &d = drawables[drawableIndex++];

            const auto &instanceGPU = registry.getInstances(d.hotInstanceGPU, currFrame);

            // Only upload hot instance
            uploader.updateInstanceGPU(
                *instanceGPU,
                visibleTransforms.data(),
                visibleTransforms.size());

            d.instanceCount = static_cast<uint32_t>(visibleTransforms.size());
        }
    }
}

// Should only use on a valid scene
void fitCameraToBoundingBox(Camera &camera, const Extents &box)
{
    glm::vec3 center = box.center();
    float radius = box.radius();

    float distance = radius / tan(camera.getFov() * 0.5f);
    distance *= 1.1f;

    glm::vec3 viewDir = glm::normalize(camera.getFront());

    glm::vec3 newPosition = center - viewDir * distance;

    camera.setPosition(newPosition);
}
