
#include "RessourcesGPU.h"
#include "Mesh.h"
#include "Buffer.h"
#include "Material.h"
#include "utils/PipelineHelper.h"
#include "Texture.h"
#include "TextureC.h"
#include "Descriptor.h"
#include "AssetRegistry.h"

#include "ResourceGPUManager.h"

// Todo:
// This is an awfully heavy method
MaterialGPU MaterialGPU::createMaterialGPU(
    // Todo: const
    const AssetRegistry &registry,
    GPUResourceRegistry &gpuRegistry,
    MaterialGPUCreateInfo info,
    const LogicalDeviceManager &deviceM,
    DescriptorManager &descriptor,
    VkPhysicalDevice physDevice,
    uint32_t queueindices)
{
         std::cout << "INMatGPu \n"
                  << std::endl;

    // Create or not a Pipeline
    const Material &material = *(registry.get(info.cpuMaterial));

    const auto &allocator = deviceM.getVmaAllocator();

    MaterialGPU gpuMat;
    gpuMat.pipelineEntryIndex = info.pipelineIndex;

    Buffer materialUniformBuffer;
    materialUniformBuffer.createBuffer(deviceM.getLogicalDevice(), physDevice, sizeof(Material::MaterialConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, allocator);
    materialUniformBuffer.uploadBuffer(&material.mConstants, sizeof(Material::MaterialConstants), 0, physDevice, deviceM, queueindices, allocator);
    // Better way to create this ?

    auto getOrDummy = [&](TextureCPU *tex, Texture *(*getter)(VkPhysicalDevice, const LogicalDeviceManager &, uint32_t, VmaAllocator)) -> Texture *
    {
        if (!tex)
        {
            return getter(physDevice, deviceM, queueindices, allocator);
        }
        Texture *textureGPU = new Texture();
        textureGPU->createTextureImage(physDevice, deviceM, *(tex), queueindices, allocator);
        textureGPU->createTextureImageView(deviceM.getLogicalDevice());
        textureGPU->createTextureSampler(deviceM.getLogicalDevice(), physDevice);

        return textureGPU;
    };

    auto acquireTex = [&](AssetID<TextureCPU> id, Texture *(*getter)(VkPhysicalDevice, const LogicalDeviceManager &, uint32_t, VmaAllocator)) -> Texture *
    {
        auto texPtr = gpuRegistry.get<Texture>(GPUHandle<Texture>(id.getID()));
        if (texPtr)
        {
            return texPtr;
        };
        //Else we create it
        return getOrDummy(registry.get(material.albedoMap), getter);
    };

     std::cout << "INMatGPu \n"
                  << std::endl;
    Texture *albedo = acquireTex(material.albedoMap, Texture::getDummyAlbedo);
    Texture *normal = acquireTex(material.normalMap, Texture::getDummyNormal);
    Texture *metallic = acquireTex(material.metallicMap, Texture::getDummyMetallic);
    Texture *roughness = acquireTex(material.roughnessMap, Texture::getDummyRoughness);
    // Todo
    // MetalRoughAO Check if can just do that
    // Also perhaps use this as a way to decide if a vecor would be easier to deal with (definitly)
     std::cout << "INMatGPu \n"  << albedo <<" " << material.albedoMap.id << " " <<  normal
                  << std::endl;
    VkDescriptorBufferInfo uboDescInfo = materialUniformBuffer.getDescriptor();
    VkDescriptorImageInfo albedoDescInfo = albedo->getImage().getDescriptor();
    VkDescriptorImageInfo normalDescInfo = normal->getImage().getDescriptor();
    VkDescriptorImageInfo metallicDescInfo = metallic->getImage().getDescriptor();
    VkDescriptorImageInfo roughnessDescInfo = roughness->getImage().getDescriptor();

    gpuMat.descriptorIndex = descriptor.allocateDescriptorSet(deviceM.getLogicalDevice(), info.descriptorLayoutIdx);
 std::cout << "INMatGPu \n"
                  << std::endl;
    auto &materialSet = descriptor.getSet(gpuMat.descriptorIndex);
    std::vector<VkWriteDescriptorSet> writes = {
        vkUtils::Descriptor::makeWriteDescriptor(materialSet, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uboDescInfo),
        vkUtils::Descriptor::makeWriteDescriptor(materialSet, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &albedoDescInfo),
        vkUtils::Descriptor::makeWriteDescriptor(materialSet, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &normalDescInfo),
        vkUtils::Descriptor::makeWriteDescriptor(materialSet, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &metallicDescInfo),
        vkUtils::Descriptor::makeWriteDescriptor(materialSet, 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &roughnessDescInfo),
    };
 std::cout << "INMatGPu \n"
                  << std::endl;
    descriptor.updateDescriptorSet(deviceM.getLogicalDevice(), writes);

    return gpuMat;
}

void MaterialGPU::destroy(VkDevice device, VmaAllocator allocator)
{
    if (allocator)
    {
        vmaDestroyBuffer(allocator, uniformBuffer, uniformBufferAlloc);
    }
    else
    {
        vkFreeMemory(device, uniformBufferMem, nullptr);
        vkDestroyBuffer(device, uniformBuffer, nullptr);
    }
    uniformBufferAlloc = VK_NULL_HANDLE;
    uniformBuffer = VK_NULL_HANDLE;
    uniformBufferMem = VK_NULL_HANDLE;
}

MeshGPU MeshGPU::createMeshGPU(const Mesh &mesh, const LogicalDeviceManager &deviceM, const VkPhysicalDevice &physDevice, const uint32_t indice, bool SSBO)
{
    MeshGPU gpu;
    const auto &device = deviceM.getLogicalDevice();
    const auto &allocator = deviceM.getVmaAllocator();
    gpu.vertexCount = static_cast<uint32_t>(mesh.positions.size());
    gpu.indexCount = static_cast<uint32_t>(mesh.indices.size());
    // Calculated using the vertex flag
    gpu.vertexStride = calculateVertexStride(mesh.getflag());

    // Create buffers via Buffer helper
    Buffer vertexBuffer;
    // TODO: TEst SSBO Shade

    vertexBuffer.createVertexBuffers(device, physDevice,
                                     mesh, deviceM, indice, allocator, SSBO);

    Buffer indexBuffer;

    indexBuffer.createIndexBuffers(device, physDevice,
                                   mesh, deviceM, indice, allocator);

    gpu.vertexBuffer = vertexBuffer.getBuffer();
    gpu.indexBuffer = indexBuffer.getBuffer();
    gpu.vertexAlloc = vertexBuffer.getVMAMemory();
    gpu.indexAlloc = indexBuffer.getVMAMemory();
    gpu.vertexMem = vertexBuffer.getMemory();
    gpu.indexMem = indexBuffer.getMemory();
    if (SSBO)
    {
        gpu.vertexAddress = vertexBuffer.getDeviceAdress(device);
    }

    return gpu;
}
// The who is allowed to destroy Mesh GPU Ressources is still a pending question
// Deletion QUEUE neeeded ?
void MeshGPU::destroy(VkDevice device, VmaAllocator allocator)
{
    if (allocator)
    {
        vmaDestroyBuffer(allocator, vertexBuffer, vertexAlloc);
        vmaDestroyBuffer(allocator, indexBuffer, indexAlloc);
    }
    else
    {
        vkFreeMemory(device, vertexMem, nullptr);
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, indexMem, nullptr);
        vkDestroyBuffer(device, indexBuffer, nullptr);
    }
    vertexBuffer = VK_NULL_HANDLE;
    indexBuffer = VK_NULL_HANDLE;
    vertexMem = VK_NULL_HANDLE;
    indexMem = VK_NULL_HANDLE;
    vertexAlloc = VK_NULL_HANDLE;
    indexAlloc = VK_NULL_HANDLE;
}

InstanceGPU InstanceGPU::createInstanceGPU(const std::vector<InstanceData> &data, const LogicalDeviceManager &deviceM, const VkPhysicalDevice &physDevice, const uint32_t indice)
{
    InstanceGPU gpu;
    const auto &device = deviceM.getLogicalDevice();
    const auto &allocator = deviceM.getVmaAllocator();
    gpu.instanceCount = static_cast<uint32_t>(data.size());
    gpu.instanceStride = sizeof(InstanceData);

    // Create buffers via Buffer helper
    Buffer buffer;
    buffer.createBuffer(device, physDevice, data.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, allocator);

    gpu.instanceBuffer = buffer.getBuffer();
    gpu.instanceAlloc = buffer.getVMAMemory();
    gpu.instanceMem = buffer.getMemory();
    // gpu.instanceAddr = buffer.getDeviceAdress(devicebuffer

    return gpu;
}

// The who is allowed to destroy Mesh GPU Ressources is still a pending question
// Deletion QUEUE neeeded ?
void InstanceGPU::destroy(VkDevice device, VmaAllocator allocator)
{
    if (allocator)
    {
        vmaDestroyBuffer(allocator, instanceBuffer, instanceAlloc);
    }
    else
    {
        vkFreeMemory(device, instanceMem, nullptr);
        vkDestroyBuffer(device, instanceBuffer, nullptr);
    }
    instanceBuffer = VK_NULL_HANDLE;
    instanceAlloc = VK_NULL_HANDLE;
    instanceMem = VK_NULL_HANDLE;
}
