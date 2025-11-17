#include "Scene.h"

void RenderScene::destroy(VkDevice device, VmaAllocator alloc)
{
    for (auto &drawable : drawables)
    {
        drawable.materialGPU.destroy(device, alloc);
        drawable.meshGPU.destroy(device, alloc);
    };
};

void RenderScene::syncFromScene(const Scene &cpuScene,
                                const GpuResourceUploader &builder)
{
    drawables.clear();
    drawables.reserve(cpuScene.nodes.size());

    for (auto &node : cpuScene.nodes)
    {
        Drawable d;
        std::cout<<"Mesh \n";
        d.meshGPU = builder.buildMeshGPU(node.mesh);
        std::cout<<"Material \n" << std::endl;

        d.materialGPU = builder.buildMaterialGPU(node.material);
        //d.instanceGPU = builder.buildInstanceGPU(d.inst);

        drawables.push_back(std::move(d));
    }
}

GpuResourceUploader::GpuResourceUploader(const VulkanContext &ctx,
                                         const AssetRegistry &assets,
                                         DescriptorManager &descriptors,
                                         PipelineManager &pipelines) : context(ctx), assetRegistry(assets),
                                                                       materialDescriptors(descriptors),
                                                                       pipelineManager(pipelines) {
                                                                       };

// Todo: Additional reminder on removing such calls
MeshGPU GpuResourceUploader::buildMeshGPU(const AssetID<Mesh> meshId, bool useSSBO) const
{
    return MeshGPU::createMeshGPU(*assetRegistry.get(meshId), context.getLogicalDeviceManager(), context.getPhysicalDeviceManager().getPhysicalDevice(), context.getPhysicalDeviceManager().getIndices().graphicsFamily.value(), useSSBO);
};

MaterialGPU GpuResourceUploader::buildMaterialGPU(const AssetID<Material> matID) const
{
    return MaterialGPU::createMaterialGPU(assetRegistry, *assetRegistry.get(matID), context.getLogicalDeviceManager(), materialDescriptors, context.getPhysicalDeviceManager().getPhysicalDevice(), context.getPhysicalDeviceManager().getIndices().graphicsFamily.value());
};
InstanceGPU GpuResourceUploader::buildInstanceGPU(const std::vector<InstanceData> &instance) const
{
    return InstanceGPU::createInstanceGPU(instance, context.getLogicalDeviceManager(), context.getPhysicalDeviceManager().getPhysicalDevice(), context.getPhysicalDeviceManager().getIndices().graphicsFamily.value());
};
