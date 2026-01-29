#include "Scene.h"
#include <unordered_set>
Scene::Scene()
{
    sceneLayout.addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    sceneLayout.addDescriptor(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
    sceneLayout.addDescriptor(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);

    sceneLayout.addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SceneData));

    lights.addDirLight({glm::vec4(0.5, 1.0, 0.0, 0.0), glm::vec4(1.0, 1.0, 1.0, 0.0), 1.0});
    // lights.addDirLight({glm::vec4(0.5, -1.0, 0.0, 0.0), glm::vec4(0.2, 0.0, 1.0, 0.0), 1.0});
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

// Todo: &&
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

#include "Buffer.h"
// Todo: Move BufferDesc out of Buffer
void updateFrameSync(
    VulkanContext &ctx,
    RenderScene& scene,
    const VisibilityFrame &frame,
    GPUResourceRegistry &registry,
    const AssetRegistry &cpuRegistry,
    MaterialSystem matSystem,
    uint32_t currFrame)
{
    constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;
    constexpr uint32_t TRANSFORM_BUFFER_KEY = 0;

    // scene.ensureInstanceBuffer(registry);
    // One global instance Buffer. Might be divided
    // RenderScene might as well own this
    auto istBufferRef = registry.getBuffer(scene.instanceBuffer, currFrame);

    for (auto &[_, drawable] : scene.drawables)
    {
        drawable.instanceRanges.clear();
    }

    AssetID<Mesh> lastAsset(INVALID_ASSET_ID);
    for (const auto &rof : frame.objects)
    {
        const auto &mesh = cpuRegistry.get(rof.mesh);
        BufferKey vtxKey;
        BufferKey idxKey;

        if (rof.mesh.getID() != lastAsset.getID()) //Shouldn't really happen
        {
            AssetID<void> query = AssetID<void>(rof.mesh.getID());
            BufferDesc description;
            const VertexFormat &format = VertexFormatRegistry::getStandardFormat();
            description.updatePolicy = BufferUpdatePolicy::Immutable;
            description.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            description.size = mesh->positions.size() * sizeof(glm::vec3) +
                               mesh->normals.size() * sizeof(glm::vec3) + mesh->uvs.size() * sizeof(glm::vec2);
            description.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            description.ssbo = false;
            vtxKey = registry.addBuffer(query, description, currFrame);
            auto vtxBufferRef = registry.getBuffer(vtxKey);
            if (vtxBufferRef.buffer == nullptr)
            {
                VertexBufferData vbd = buildInterleavedVertexBuffer(*mesh, format);
                vtxBufferRef.uploadData(ctx, vbd.mBuffers[0].data(), description.size);
            }

            description.size = mesh->indices.size() * sizeof(uint32_t);
            description.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            idxKey = registry.addBuffer(query, description, currFrame);
            auto idxBufferRef = registry.getBuffer(idxKey);

            if (idxBufferRef.buffer == nullptr)
            {
                vtxBufferRef.uploadData(ctx, mesh->indices.data(), description.size);
            }
        }

        for (uint32_t submeshIdx = 0; submeshIdx < mesh->submeshes.size(); submeshIdx++)
        {
            uint64_t drawableKey = (uint64_t(rof.mesh.getID()) << 32 | submeshIdx);
            Drawable &drawable = scene.drawables[drawableKey];

            // Upload Material if it doesn't exist
            auto materialID = mesh->materialIds[mesh->submeshes[submeshIdx].materialId];
            drawable.vtxBuffer = vtxKey;
            drawable.idxBuffer = idxKey;
            drawable.materialGPU = matSystem.requestMaterial(materialID);
            drawable.indexOffset = mesh->submeshes[submeshIdx].indexOffset;
            drawable.indexCount = mesh->submeshes[submeshIdx].indexCount;
    
            uint32_t firstInstance = UINT32_MAX;
            uint32_t count = 0;

            std::vector<uint32_t> instanceIDs;
            instanceIDs.reserve(rof.visibleInstances.size());
            for (const auto &inst : rof.visibleInstances)
            {
                uint32_t instanceID = scene.getOrAssignInstance(inst.instanceKey);

                if (inst.transformDirty)
                {
                    istBufferRef.updateElement(ctx, &inst.transform, instanceID, sizeof(InstanceTransform));
                }

                instanceIDs.push_back(instanceID);
            }

            if (instanceIDs.empty())
            {
                continue;
            }

            // Sort & build ranges
            std::sort(instanceIDs.begin(), instanceIDs.end());

            uint32_t start = instanceIDs[0];
            uint32_t count = 1;

            for (size_t i = 1; i < instanceIDs.size(); ++i)
            {
                //Instance haven't contiguous id
                if (!(instanceIDs[i] == instanceIDs[i - 1] + 1))
                {
                    drawable.instanceRanges.push_back({start, count});
                    start = instanceIDs[i];
                    count = 1;
                    continue;
                }
                ++count;
            }

            drawable.instanceRanges.push_back({start, count});

            // Drawable are one instance of a submesh
            // This put it in instances [all submeshes] order
            // submeshe [all instances] would be pre grouped to some degree
        }
    }
    
}

// Allocator should be tied to SCene or be some sort of Ecs for handling removed stuff
VisibilityFrame extractRenderFrame(const Scene &scene,
                                   const AssetRegistry &registry)
{
    VisibilityFrame frame;
    frame.sceneData = scene.getSceneData();
    frame.lightData = scene.getLightPacket();

    // Assuming Scene new comer were resolved earlier
    //  Notes: We woudl flatten SceneNodes into RenderObjectFrames if we had hierarchy
    for (auto &node : scene.nodes)
    {
        if (!node.visible || node.instanceNb() == 0)
        {
            // We might still want to upload first timer actually
            // In hope to have more contiguous mesh
            continue;
        }

        std::vector<VisibleObject::VisibleInstance> visibleInstances;
        for (uint32_t instIdx = 0; instIdx < node.instanceNb(); ++instIdx)
        {
            glm::vec3 worldPos = node.getTransform(instIdx).getPosition();
            Extents worldExtents = node.nodeExtents;
            worldExtents.translate(worldPos);

            if (!testAgainstFrustum(frame.sceneData.viewproj, worldExtents))
            {
                continue;
            }

            uint64_t key = (uint64_t(&node) << 32) | instIdx;

            VisibleObject::VisibleInstance data{};
            data.transform = node.buildGPUTransform(instIdx);
            data.instanceKey = key;
            // Todo: Always dirty until i handle scene update
            data.transformDirty = true;
            visibleInstances.push_back(data);
        }

        const auto &mesh = registry.get(node.mesh);

        // Create one RenderObjectFrame per submesh, sharing visibleInstances
        // Notes: Ideally visibile instance array wouldn'( be duplicated)
        // Pointer with Linear Allocator
        VisibleObject rof{};
        rof.mesh = node.mesh;
        // rof.indexOffset = 0;
        // rof.vertexOffset = 0;
        rof.visibleInstances = visibleInstances;
        frame.objects.push_back(std::move(rof));
        // rof.instanceData = node.getGenericData(instIdx);
        //  rof.worldExtents = node.nodeExtents;
        // rof.layout = node.layout;
    }

    return frame;
};

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




void buildPassFrame(
    RenderPassFrame &outPass,
    RenderScene &scene,
    PipelineManager &mPipeline, int pipelineINdex)
{
    outPass.queue.drawCalls.clear();
    outPass.queue.pipelineIndex.clear();

    for (auto &[key, drawable] : scene.drawables)
    {
        if (drawable.instanceRanges.empty())
        {
            continue;
        }

        // Pass-level filtering happens
        // Reintroduce Passrequirement and what not
        // if (!materialCompatible(drawable.materialGPU, outPass.type))
        //    continue;
        // Also we still haven't really culled thing only visible by certain passes

        /*
        PipelineBuilder pkey;
        pkey.pass = outPass.type;
        pkey.material = drawable.materialGPU;
        pkey.vertexFormat = drawable.vertexFormat;
        pkey.depthTest = true;
        pkey.depthWrite = (outPass.type != RenderPassType::Transparent);
        */

        outPass.queue.drawCalls.push_back(&drawable);
    }
    outPass.queue.pipelineIndex.push_back(pipelineINdex);
}
