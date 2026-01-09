#include "Scene.h"

Scene::Scene()
{
    sceneLayout.addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    sceneLayout.addDescriptor(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
    sceneLayout.addDescriptor(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);

    sceneLayout.addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SceneData));

    lights.addDirLight({glm::vec4(0.5, 1.0, 0.0, 0.0), glm::vec4(0.0, 0.0, 1.0, 0.0), 1.0});
    lights.addPointLight({glm::vec4(0.5, 0.5, 0.5, 0.0), glm::vec4(1.0, 0.0, 0.0, 0.0), 0.5, 0.5});
};

explicit Scene::Scene(int compute)
{
    sceneLayout.addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
    sceneLayout.addDescriptor(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
    sceneLayout.addDescriptor(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);

    sceneLayout.addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SceneData));
};

void Scene::addNode(SceneNode node)
{
    nodes.push_back(node);
}
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

    return {proj * camera.getViewMatrix(), camera.getEye()};
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

        for (auto &submesh : mesh->submeshes)
        {
            Drawable d;
            d.indexOffset = submesh.indexOffset;
            d.indexCount = submesh.indexCount;

            d.meshGPU = registry.add(meshHandle, std::function<MeshGPU()>([&]()
                                                                          { return builder.uploadMeshGPU(node.mesh); }));
            // Todo:
            // In practice, i could create a GpuHandle using the id but that's not a behavior i am clear on
            d.materialGPU = registry.add(mesh->materialIds[submesh.materialId],
                                         std::function<MaterialGPU()>([&]()
                                                                      { return MaterialGPU(); }));

            // d.instanceGPU = builder.buildInstanceGPU(d.inst);
            drawables.push_back(std::move(d));
        }
    }
}
