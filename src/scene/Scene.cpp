#include "Scene.h"

Scene::Scene()
{
    sceneLayout.addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    sceneLayout.addDescriptor(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
    sceneLayout.addDescriptor(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);

    sceneLayout.addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SceneData));

    lights.addDirLight({glm::vec4(0.5, 1.0, 0.0, 0.0), glm::vec4(1.0, 1.0, 1.0, 0.0), 1.0});
    lights.addDirLight({glm::vec4(0.5, -1.0, 0.5,0.0), glm::vec4(0.2, 0.0, 1.0, 0.0), 1.0});
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

void Scene::addNode(SceneNode node)
{
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
SceneData Scene::getSceneData()
{
    glm::mat4 proj = camera.getProjectionMatrix();
    proj[1][1] *= -1;

    return {proj * camera.getViewMatrix(), camera.getEye(), glm::vec4(0.3,0.1,0.4, 0.0)};
};

LightPacket Scene::getLightPacket()
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

void RenderScene::syncFromScene(const Scene &cpuScene,
                                const AssetRegistry &cpuRegistry,
                                GPUResourceRegistry &registry,
                                const GpuResourceUploader &builder)
{
    drawables.clear();
    drawables.reserve(cpuScene.nodes.size());
    int index = 0;

    for (auto &node : cpuScene.nodes)
    {
        // Is there a way to not add more add registry more than once
        const auto &meshHandle = node.mesh;
        const auto &mesh = cpuRegistry.get(meshHandle);

        auto meshGPU = registry.add(meshHandle, std::function<MeshGPU()>([&]()
                                                                         { return builder.uploadMeshGPU(node.mesh); }));
        for (auto &submesh : mesh->submeshes)
        {
            Drawable d;
            d.indexOffset = submesh.indexOffset;
            d.indexCount = submesh.indexCount;

            d.meshGPU = meshGPU;

            // Todo:
            // In practice, i could create a GpuHandle using the id but that's not a behavior i am clear on using
            d.materialGPU = registry.add(mesh->materialIds[submesh.materialId],
                                         std::function<MaterialGPU()>([&]()
                                                                      { return MaterialGPU(); }));

            uint32_t MAX_FRAMES_IN_FLIGHT = 3;

            // Todo:
            // Notes: Instance is pretty much mesh only dependant so it don't need to be nested here
            d.instanceGPU = registry.addMultiFrame(meshHandle, mesh->materialIds[submesh.materialId], MAX_FRAMES_IN_FLIGHT,
                                                   std::function<InstanceGPU()>([&]()
                                                                                { return builder.uploadInstanceGPU(node.instanceData, node.layout, node.instanceCount, true); }));

            drawables.push_back(std::move(d));
        }
    }
}

//Should only use on a valid scene
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
