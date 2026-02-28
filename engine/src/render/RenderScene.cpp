#include "RenderScene.h"
#include "Buffer.h"
#include "Texture.h"
#include "MaterialSystem.h"

// Todo: Trim down
void RenderScene::updateFrameSync(
    VulkanContext &ctx,
    const VisibilityFrame &frame,
    GPUResourceRegistry &registry,
    const AssetRegistry &cpuRegistry,
    MaterialSystem &matSystem,
    uint32_t currFrame)
{
    constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;
    constexpr uint32_t TRANSFORM_BUFFER_KEY = 0;

    // scene.ensureInstanceBuffer(registry);
    // One global instance Buffer. Might be divided
    // RenderScene might as well own this
    auto istBufferRef = registry.getBuffer(instanceBuffer, currFrame);

    for (auto &[_, drawable] : drawables)
    {
        drawable.instanceRanges.clear();
    }

    AssetID<Mesh> lastAsset(INVALID_ASSET_ID);
    for (const auto &rof : frame.objects)
    {
        const auto &mesh = cpuRegistry.get(rof.mesh);
        BufferKey vtxKey;
        BufferKey idxKey;

        if (rof.mesh.getID() != lastAsset.getID()) // Shouldn't really happen
        {
            AssetID<void> query = AssetID<void>(rof.mesh.getID());
            BufferDesc description;
            const VertexFormat &format = VertexFormatRegistry::getStandardFormat();
            description.updatePolicy = BufferUpdatePolicy::StagingOnly;
            description.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            description.size = mesh->positions.size() * sizeof(glm::vec3) +
                               mesh->normals.size() * sizeof(glm::vec3) + mesh->uvs.size() * sizeof(glm::vec2);
            description.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

            BufferKey vtxKeyTmp{rof.mesh.getID()};

            // This is temporary hopefully
            if (!registry.hasBuffer(vtxKeyTmp))
            {
                vtxKey = registry.addBuffer(query, description);
                GPUResourceRegistry::BufferAllocation allocation{.offset = 0, .size = description.size};
                registry.allocateInBuffer(vtxKey, allocation);

                auto vtxBufferRef = registry.getBuffer(vtxKey);
                VertexBufferData vbd = buildInterleavedVertexBuffer(*mesh, format);
                vtxBufferRef.uploadData(ctx, vbd.mBuffers[0].data(), description.size);

                description.size = mesh->indices.size() * sizeof(uint32_t);
                description.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

                //"Cheat", now that usage doesn't define the key buy key remain widely used
                // Le
                query.id++;
                idxKey = registry.addBuffer(query, description);
                allocation.size = description.size;
                registry.allocateInBuffer(idxKey, allocation);
                auto idxBufferRef = registry.getBuffer(idxKey);
                idxBufferRef.uploadData(ctx, mesh->indices.data(), description.size);
            }
            else
            {

                vtxKey = vtxKeyTmp;
                idxKey = BufferKey{rof.mesh.getID() + 1};
            }
        }

        std::vector<std::vector<uint32_t>> buckets;
        buckets.resize(mesh->submeshes.size());

        for (uint32_t submeshIdx = 0;
             submeshIdx < mesh->submeshes.size();
             ++submeshIdx)
        {
            uint64_t drawableKey =
                (uint64_t(rof.mesh.getID()) << 32 | submeshIdx);

            Drawable &drawable = drawables[drawableKey];

            auto materialID =
                mesh->materialIds[mesh->submeshes[submeshIdx].materialId];

            drawable.vtxBuffer = vtxKey;
            drawable.idxBuffer = idxKey;
            drawable.materialGPU = matSystem.requestMaterial(materialID);
            drawable.indexOffset = mesh->submeshes[submeshIdx].indexOffset;
            drawable.indexCount = mesh->submeshes[submeshIdx].indexCount;

            drawable.instanceRanges.clear();
            buckets.reserve(rof.visibleInstances.size());
        }

        for (const auto &inst : rof.visibleInstances)
        {
            uint32_t instanceID = getOrAssignInstance(inst.instanceKey);

            if (inst.transformDirty)
            {
                istBufferRef.updateElement(
                    ctx,
                    &inst.transform,
                    instanceID,
                    sizeof(InstanceTransform));
            }

            for (const auto submeshIdx : inst.visibleSubmesh)
            {
                buckets[submeshIdx].push_back(instanceID);
            }
        }

        for (uint32_t submeshIdx = 0;
             submeshIdx < mesh->submeshes.size();
             ++submeshIdx)
        {
            auto &instanceIDs = buckets[submeshIdx];

            if (instanceIDs.empty())
                continue;

            uint64_t drawableKey =
                (uint64_t(rof.mesh.getID()) << 32 | submeshIdx);

            Drawable &drawable = drawables[drawableKey];

            // Sort & build ranges
            std::sort(instanceIDs.begin(), instanceIDs.end());

            uint32_t start = instanceIDs[0];
            uint32_t count = 1;

            for (size_t i = 1; i < instanceIDs.size(); ++i)
            {
                // Instance haven't contiguous id
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
        }
    }
}

void buildPassFrame(
    RenderPassFrame &outFrame,
    RenderScene &scene,
    MaterialSystem &materialSystem)
{
    outFrame.queues.clear();

    std::unordered_map<uint32_t, size_t> queueMap;

    for (auto &[key, drawable] : scene.drawables)
    {
        // Skip drawables with no instances
        if (drawable.instanceRanges.empty())
        {
            continue;
        }

        // Resolve material variant (guaranteed to return a valid MaterialInstance, dummy if necessary)
        const MaterialInstance &matInst = materialSystem.requestMaterialInstance(drawable.materialGPU, outFrame.type, 0);

        uint32_t pipeline = matInst.pipelineIndex;

        auto it = queueMap.find(pipeline);
        if (it == queueMap.end())
        {
            size_t queueIdx = outFrame.queues.size();
            outFrame.queues.push_back({pipeline, {}});
            queueMap[pipeline] = queueIdx;
            it = queueMap.find(pipeline);
        }

        outFrame.queues[it->second].drawCalls.push_back(&drawable);
    }
}
