#include "Scene.h"

Scene::Scene()
{
    sceneLayout.addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    sceneLayout.addDescriptor(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
    sceneLayout.addDescriptor(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);

    sceneLayout.addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SceneData));

    lights.addDirLight({glm::vec4(0.5, 1.0, 0.0, 0.0), glm::vec4(1.0, 1.0, 1.0, 0.0), 1.0});
    //lights.addDirLight({glm::vec4(0.5, -1.0, 0.0, 0.0), glm::vec4(0.2, 0.0, 1.0, 0.0), 1.0});
    lights.addPointLight({glm::vec4(0.0, 1.0, 0.5, 0.0), glm::vec4(1.0, 1.0, 0.0, 0.0), 1.5, 1.5});
    lights.addPointLight({glm::vec4(-0.0, 0.0, 0.5, 0.0), glm::vec4(0.0, 1.0, 1.0, 0.0), 0.5, 0.5});
    lights.addPointLight({glm::vec4(-9.5, -14, 0.5, 0.0), glm::vec4(1.0, 1.0, 0.0, 0.0), 2.5, 5.5});
    lights.addPointLight({glm::vec4(-5.5, -13.0, 0.5, -0.5), glm::vec4(0.0, 1.0, 1.0, 0.0), 5.5, 1.5});
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


RenderScene* updateFrameSync(
    const RenderFrame &frame,
    GPUResourceRegistry &registry,
    const AssetRegistry &cpuRegistry,
    const GpuResourceUploader &uploader,
    uint32_t currFrame)
{
    RenderScene* scenePtr = new RenderScene();
    RenderScene& scene = *scenePtr;
    constexpr uint32_t MAX_INSTANCES = 10;

    for (const RenderObjectFrame &rof : frame.objects)
    {
        const auto &mesh = cpuRegistry.get(rof.mesh);

        // Mesh: lazy, once
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

            // Instance: lazy creation
            d.hotInstanceGPU = registry.addMultiFrame(
                rof.mesh,
                mesh->materialIds[submesh.materialId],
                0,
                MAX_FRAMES_IN_FLIGHT,
                std::function<InstanceGPU()>([&]
                                             { return InstanceGPU{}; }));

            d.coldInstanceGPU = registry.addMultiFrame(rof.mesh, mesh->materialIds[submesh.materialId], 1, MAX_INSTANCES,
                                                       std::function<InstanceGPU()>([&]()
                                                                                    { return uploader.uploadInstanceGPU(rof.instanceData, rof.layout, 10, true); }));

            // Per-frame buffer
            InstanceGPU *gpu = registry.getInstances(d.hotInstanceGPU, currFrame);

            if (!gpu)
            {
                //Failure state for now
                //We should always have an instance
                return nullptr;
            };
            // Unified sync (allocate / reallocate / update)
            uploader.syncInstanceGPU(
                *gpu,
                rof.transforms.data(),
                static_cast<uint32_t>(rof.transforms.size()),
                rof.layout,
                MAX_INSTANCES);

            d.instanceCount = gpu->count;

            scene.drawables.push_back(std::move(d));
        }
    }

    return scenePtr;
}


RenderFrame extractRenderFrame(const Scene &scene,
                                      const AssetRegistry &registry)
{
    RenderFrame frame;
    frame.sceneData = scene.getSceneData();
    frame.lightData = scene.getLightPacket();

    // Notes: We woudl flatten SceneNodes into RenderObjectFrames if we had hierarchy
    for (size_t nodeIdx = 0; nodeIdx < scene.nodes.size(); ++nodeIdx)
    {
        const SceneNode &node = scene.nodes[nodeIdx];
        if (!node.visible || node.instanceNb() == 0)
        {
            continue;
        }

        const auto &mesh = registry.get(node.mesh);

        for (uint32_t submeshIdx = 0; submeshIdx < mesh->submeshes.size(); ++submeshIdx)
        {
            RenderObjectFrame rof{};
            const auto &submesh = mesh->submeshes[submeshIdx];
            rof.mesh = node.mesh;
            rof.material = mesh->materialIds[submesh.materialId];

            rof.indexOffset = submesh.indexOffset;
            rof.indexCount = submesh.indexCount;

            rof.transforms.reserve(node.instanceNb());
            rof.instanceData.reserve(node.instanceNb());

            for (uint32_t instIdx = 0; instIdx < node.instanceNb(); ++instIdx)
            {

                if (!testAgainstFrustum(frame.sceneData.viewproj, node.nodeExtents))
                {
                    continue;
                }
                rof.transforms.push_back(node.buildGPUTransform(instIdx));
                rof.instanceData = node.getGenericData(instIdx);
            }

            // rof.worldExtents = node.nodeExtents;
            rof.layout = node.layout;
            frame.objects.push_back(std::move(rof));
        }
    }

    // Handle passes
    //  Note:
    //   Currently only Forward pass exists, but expandable
    RenderPassFrame forwardPass{};
    forwardPass.type = RenderPassType::Forward;
    forwardPass.sortedObjectIndices.reserve(frame.objects.size());
    for (size_t i = 0; i < frame.objects.size(); ++i)
    {
        forwardPass.sortedObjectIndices.push_back(i);
    }

    /*
    // Sort by pipeline first, then material if we separate material and piepline
    std::sort(frame.sortedIndices.begin(), frame.sortedIndices.end(),
    [&](size_t a, size_t b)
    {
        const auto &ra = frame.objects[a];
        const auto &rb = frame.objects[b];
        if (ra.pipelineIndex != rb.pipelineIndex)
            return ra.pipelineIndex < rb.pipelineIndex;
        return ra.materialHash < rb.materialHash;
    });
    */
    std::sort(forwardPass.sortedObjectIndices.begin(), forwardPass.sortedObjectIndices.end(),
              [&](size_t a, size_t b)
              {
                  const auto &rofa = frame.objects[a];
                  const auto &rofb = frame.objects[b];
                  return rofa.material.getID() < rofb.material.getID();
              });

    frame.passes.push_back(std::move(forwardPass));

    return frame;
};