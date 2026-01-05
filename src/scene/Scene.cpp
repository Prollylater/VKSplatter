#include "Scene.h"

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
            //Todo:
            //In practice, i could create a GpuHandle using the id but that's not a behavior i am clear on
            d.materialGPU = registry.add(mesh->materialIds[submesh.materialId],
                            std::function<MaterialGPU()>([&](){return MaterialGPU();}));

            // d.instanceGPU = builder.buildInstanceGPU(d.inst);
            drawables.push_back(std::move(d));
        }
    }
}
