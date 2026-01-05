
#include "GPUResourceSystem.h"
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

    MeshGPU gpu;
    auto &deviceM = context.getLogicalDeviceManager();

    const auto device = context.getLogicalDeviceManager().getLogicalDevice();
    const auto allocator = context.getLogicalDeviceManager().getVmaAllocator();
    const auto indice = context.getPhysicalDeviceManager().getIndices().graphicsFamily.value();
    const auto physDevice = context.getPhysicalDeviceManager().getPhysicalDevice();
    // Todo: There's no guarantee  id exist so i should usse pointer
    auto &mesh = *assetRegistry.get(meshId);
    //gpu.vertexCount = static_cast<uint32_t>(mesh.positions.size());
    //gpu.indexCount = static_cast<uint32_t>(mesh.indices.size());

    // Create buffers via Buffer helper
    Buffer vertexBuffer;
    // TODO: TEst SSBO Shade

    vertexBuffer.createVertexBuffers(device, physDevice,
                                     mesh, deviceM, indice, allocator, useSSBO);

    Buffer indexBuffer;

    indexBuffer.createIndexBuffers(device, physDevice,
                                   mesh, deviceM, indice, allocator);

    gpu.vertexBuffer = vertexBuffer.getBuffer();
    gpu.indexBuffer = indexBuffer.getBuffer();
    gpu.vertexAlloc = vertexBuffer.getVMAMemory();
    gpu.indexAlloc = indexBuffer.getVMAMemory();
    gpu.vertexMem = vertexBuffer.getMemory();
    gpu.indexMem = indexBuffer.getMemory();
    if (useSSBO)
    {
        gpu.vertexAddress = vertexBuffer.getDeviceAdress(device);
    }

    return gpu;
};

// Todo: Rewrite this method 
// Descriptor writing is too much responsibility here
MaterialGPU GpuResourceUploader::uploadMaterialGPU(const AssetID<Material> matID, GPUResourceRegistry &gpuRegistry, int descriptorIdx, int pipelineIndex) const
{
    MaterialGPU::MaterialGPUCreateInfo info{matID, descriptorIdx, pipelineIndex};
    auto &deviceM = context.getLogicalDeviceManager();
    const auto device = context.getLogicalDeviceManager().getLogicalDevice();
    const auto allocator = context.getLogicalDeviceManager().getVmaAllocator();
    const auto indice = context.getPhysicalDeviceManager().getIndices().graphicsFamily.value();
    const auto physDevice = context.getPhysicalDeviceManager().getPhysicalDevice();

    // Create or not a Pipeline
    const Material &material = *(assetRegistry.get(info.cpuMaterial));

    MaterialGPU gpuMat;
    gpuMat.pipelineEntryIndex = info.pipelineIndex;

    Buffer materialUniformBuffer;
    materialUniformBuffer.createBuffer(deviceM.getLogicalDevice(), physDevice, sizeof(Material::MaterialConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, allocator);
    materialUniformBuffer.uploadBuffer(&material.mConstants, sizeof(Material::MaterialConstants), 0, physDevice, deviceM, indice, allocator);
    // Better way to create this ?

    auto getOrDummy = [&](AssetID<TextureCPU> id, Texture *(*getter)(VkPhysicalDevice, const LogicalDeviceManager &, uint32_t, VmaAllocator)) -> Texture *
    {
        TextureCPU *tex = assetRegistry.get(id);
        if (!tex)
        { // This can currently be uploaded multiples time
            return getter(physDevice, deviceM, indice, allocator);
        }

        return gpuRegistry.get(gpuRegistry.add(id,
                                               std::function<Texture()>([&]()
                                                                        { return uploadTexture(id); })));
    };

    auto acquireTex = [&](AssetID<TextureCPU> id, Texture *(*getter)(VkPhysicalDevice, const LogicalDeviceManager &, uint32_t, VmaAllocator)) -> Texture *
    {
        auto texPtr = gpuRegistry.get<Texture>(GPUHandle<Texture>(id.getID()));
        if (texPtr)
        {
            return texPtr;
        };
        // Else we create it
        return getOrDummy(id, getter);
    };

    Texture *albedo = acquireTex(material.albedoMap, Texture::getDummyAlbedo);
    Texture *normal = acquireTex(material.normalMap, Texture::getDummyNormal);
    Texture *metallic = acquireTex(material.metallicMap, Texture::getDummyMetallic);
    Texture *roughness = acquireTex(material.roughnessMap, Texture::getDummyRoughness);
    Texture *emissive = acquireTex(material.emissiveMap, Texture::getDummyMetallic);

    // Todo
    // MetalRoughAO Check if can just do that
    // Also perhaps use this as a way to decide if a vecor would be easier to deal with (definitly)
    VkDescriptorBufferInfo uboDescInfo = materialUniformBuffer.getDescriptor();
    VkDescriptorImageInfo albedoDescInfo = albedo->getImage().getDescriptor();
    VkDescriptorImageInfo normalDescInfo = normal->getImage().getDescriptor();
    VkDescriptorImageInfo metallicDescInfo = metallic->getImage().getDescriptor();
    VkDescriptorImageInfo roughnessDescInfo = roughness->getImage().getDescriptor();
    VkDescriptorImageInfo emissiveDesc = emissive->getImage().getDescriptor();


    gpuMat.descriptorIndex = materialDescriptors.allocateDescriptorSet(deviceM.getLogicalDevice(), info.descriptorLayoutIdx);
    auto &materialSet = materialDescriptors.getSet(gpuMat.descriptorIndex);
    std::vector<VkWriteDescriptorSet> writes = {
        vkUtils::Descriptor::makeWriteDescriptor(materialSet, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uboDescInfo),
        vkUtils::Descriptor::makeWriteDescriptor(materialSet, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &albedoDescInfo),
        vkUtils::Descriptor::makeWriteDescriptor(materialSet, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &normalDescInfo),
        vkUtils::Descriptor::makeWriteDescriptor(materialSet, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &metallicDescInfo),
        vkUtils::Descriptor::makeWriteDescriptor(materialSet, 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &roughnessDescInfo),
        vkUtils::Descriptor::makeWriteDescriptor(materialSet, 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &emissiveDesc)
    };
    materialDescriptors.updateDescriptorSet(deviceM.getLogicalDevice(), writes);

    return gpuMat;
}

InstanceGPU GpuResourceUploader::uploadInstanceGPU(const std::vector<InstanceData> &instance) const
{

    auto &deviceM = context.getLogicalDeviceManager();
    const auto device = context.getLogicalDeviceManager().getLogicalDevice();
    const auto allocator = context.getLogicalDeviceManager().getVmaAllocator();
    const auto indice = context.getPhysicalDeviceManager().getIndices().graphicsFamily.value();
    const auto physDevice = context.getPhysicalDeviceManager().getPhysicalDevice();

    InstanceGPU gpu;
    gpu.instanceCount = static_cast<uint32_t>(instance.size());
    gpu.instanceStride = sizeof(InstanceData);

    // Create buffers via Buffer helper
    Buffer buffer;
    buffer.createBuffer(device, physDevice, instance.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, allocator);

    gpu.instanceBuffer = buffer.getBuffer();
    gpu.instanceAlloc = buffer.getVMAMemory();
    gpu.instanceMem = buffer.getMemory();
    // gpu.instanceAddr = buffer.getDeviceAdress(devicebuffer

    return gpu;
}

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
    for (auto &pair : meshes)
    {
        if (pair.second.resource)
        {
            pair.second.resource->destroy(device, allocator);
        }
    }
    meshes.clear();

    for (auto &pair : materials)
    {
        if (pair.second.resource)
        {
            pair.second.resource->destroy(device, allocator);
        }
    }
    materials.clear();

    for (auto &pair : textures)
    {
        if (pair.second.resource)
        {
            pair.second.resource->destroy(device, allocator);
        }
    }
    textures.clear();

    for (auto &pair : instances)
    {
        if (pair.second.resource)
        {
            pair.second.resource->destroy(device, allocator);
        }
    }
    instances.clear();
}
