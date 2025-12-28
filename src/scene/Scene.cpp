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
                                const GpuResourceUploader &builder,
                                GPUResourceRegistry &registry,
                                const std::vector<MaterialGPU::MaterialGPUCreateInfo> &matCaches)
{
    drawables.clear();
    drawables.reserve(cpuScene.nodes.size());
    int index = 0;
    for (auto &node : cpuScene.nodes)
    {
        Drawable d;
        std::cout << "Mesh \n";
        d.meshGPU = registry.add(node.mesh, builder.uploadMeshGPU(node.mesh));

        std::cout << "Material \n"
                  << std::endl;

        const auto &cache = matCaches[index];
        d.materialGPU = registry.add(cache.cpuMaterial, builder.uploadMaterialGPU(cache.cpuMaterial, registry, cache.descriptorLayoutIdx, cache.pipelineIndex););

        // d.instanceGPU = builder.buildInstanceGPU(d.inst);

        drawables.push_back(std::move(d));
    }
}
