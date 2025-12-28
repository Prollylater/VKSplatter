
#include "ResourceGPUManager.h"
#include "Mesh.h"
#include "Buffer.h"
#include "Material.h"
#include "utils/PipelineHelper.h"
#include "Texture.h"
#include "TextureC.h"
#include "Descriptor.h"
#include "AssetRegistry.h"
#include "ContextController.h"

/////////////////////////////////////////////////////////////////

MeshGPU GpuResourceUploader::uploadMeshGPU(const AssetID<Mesh> meshId, bool useSSBO) const
{
    return MeshGPU::createMeshGPU(*assetRegistry.get(meshId), context.getLogicalDeviceManager(), context.getPhysicalDeviceManager().getPhysicalDevice(), context.getPhysicalDeviceManager().getIndices().graphicsFamily.value(), useSSBO);
};

MaterialGPU GpuResourceUploader::uploadMaterialGPU(const AssetID<Material> matID, GPUResourceRegistry &gpuRegistry, int descriptorIdx, int pipelineIndex) const
{
    return MaterialGPU::createMaterialGPU(assetRegistry, gpuRegistry, {matID, descriptorIdx, pipelineIndex}, context.getLogicalDeviceManager(), materialDescriptors, context.getPhysicalDeviceManager().getPhysicalDevice(), context.getPhysicalDeviceManager().getIndices().graphicsFamily.value());
};
InstanceGPU GpuResourceUploader::uploadInstanceGPU(const std::vector<InstanceData> &instance) const
{
    return InstanceGPU::createInstanceGPU(instance, context.getLogicalDeviceManager(), context.getPhysicalDeviceManager().getPhysicalDevice(), context.getPhysicalDeviceManager().getIndices().graphicsFamily.value());
};

Texture GpuResourceUploader::uploadTexture(const AssetID<TextureCPU> textureId) const
{
    Texture textureGPU;
    auto &deviceM = context.getLogicalDeviceManager();
    auto &physDevice = context.getPhysicalDeviceManager();

    // Check on CPU might be needed here or above
    textureGPU.createTextureImage(physDevice.getPhysicalDevice(),
                                  deviceM, *(assetRegistry.get(textureId)), physDevice.getIndices().graphicsFamily.value(), deviceM.getVmaAllocator());
    textureGPU.createTextureImageView(deviceM.getLogicalDevice());
    textureGPU.createTextureSampler(deviceM.getLogicalDevice(), physDevice.getPhysicalDevice());

    return textureGPU;
};

///////////////////////////////////////////


template <>
std::unordered_map<uint64_t, GPUResourceRegistry::GPURessRecord<MeshGPU>> &
GPUResourceRegistry::getMap<MeshGPU>()
{
    return meshes;
}

template <>
std::unordered_map<uint64_t, GPUResourceRegistry::GPURessRecord<MaterialGPU>> &
GPUResourceRegistry::getMap<MaterialGPU>()
{
    return materials;
}

template <>
std::unordered_map<uint64_t, GPUResourceRegistry::GPURessRecord<Texture>> &
GPUResourceRegistry::getMap<Texture>()
{
    return textures;
}

template <>
const std::unordered_map<uint64_t, GPUResourceRegistry::GPURessRecord<MeshGPU>> &
GPUResourceRegistry::getMap<MeshGPU>() const
{
    return meshes;
}

template <>
const std::unordered_map<uint64_t, GPUResourceRegistry::GPURessRecord<MaterialGPU>> &
GPUResourceRegistry::getMap<MaterialGPU>() const
{
    return materials;
}

template <>
const std::unordered_map<uint64_t, GPUResourceRegistry::GPURessRecord<Texture>> &
GPUResourceRegistry::getMap<Texture>() const
{
    return textures;
}



void GPUResourceRegistry::clearAll(VkDevice device, VmaAllocator allocator)
{
    // Destroy all meshes
    for (auto &pair : meshes)
    {
        if (pair.second.resource)
            pair.second.resource->destroy(device, allocator);
    }
    meshes.clear();

    // Destroy all materials
    for (auto &pair : materials)
    {
        if (pair.second.resource)
            pair.second.resource->destroy(device, allocator);
    }
    materials.clear();

    // Destroy all textures
    for (auto &pair : textures)
    {
        if (pair.second.resource)
            pair.second.resource->destroy(device, allocator);
    }
    textures.clear();

    // Destroy all instances
    for (auto &pair : instances)
    {
        if (pair.second.resource)
            pair.second.resource->destroy(device, allocator);
    }
    instances.clear();
}

