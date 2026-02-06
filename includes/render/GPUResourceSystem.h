#pragma once
#include "GPUResource.h"
#include "AssetTypes.h"
#include "Texture.h"
#include <functional>
#include <unordered_map>
#include <memory>

// Are that many forward declaration useful ?
class Mesh;
class VulkanContext;
class DescriptorManager;
class PipelineManager;
class AssetRegistry;
struct InstanceLayout;


// Todo: The refcounting

class GPUResourceRegistry
{
public:
    GPUResourceRegistry(VulkanContext &ctx)
        : mContext(ctx)
    {
    }
    //Todo:Can i make it more homogeneous
    BufferKey addBuffer(AssetID<void> cpuAsset, const BufferDesc &desc, VkDeviceSize explicitOffset = 0);
    GPUBufferRef getBuffer(const BufferKey &key, int allocation = 0);
    bool isEmpty(const BufferKey &key);
    void releaseBuffer(const BufferKey &key);

    GPUHandle<Texture> addTexture(AssetID<TextureCPU> cpuAsset, TextureCPU *cpuData = nullptr);
    Texture *getTexture(const GPUHandle<Texture> &handle);
    void releaseTexture(const GPUHandle<Texture> &handle);

    GPUHandle<MaterialGPU> addMaterial(AssetID<Material> cpuAsset, MaterialGPU gpuMaterial);
    MaterialGPU *getMaterial(const GPUHandle<MaterialGPU> &handle);
    void releaseMaterial(const GPUHandle<MaterialGPU> &handle);

    void clearAll();

private:
    struct BufferAllocation
    {
        VkDeviceSize offset;
        VkDeviceSize size;
        VkDeviceSize stride;
        //This used to be used as an Hash for very different allocation in the same Buffer
        //Currently we are supposed to know the data and supply the allocation index
        //uint64_t contentHash; 
    };

    struct BufferRecord
    {
        std::unique_ptr<Buffer> buffer;
        std::vector<BufferAllocation> allocations;
        VkDeviceSize usedSize = 0;
        uint32_t refCount = 1;
    };

    // Bring back templating
    template <typename T>
    struct ResourceRecord
    {
        std::unique_ptr<T> resource;
        uint32_t refCount = 1;
    };

private:
    VulkanContext &mContext;
    std::unordered_map<BufferKey, BufferRecord> mBufferRecords;
    std::unordered_map<uint64_t, ResourceRecord<Texture>> mTextureRecords;
    std::unordered_map<uint64_t, ResourceRecord<MaterialGPU>> mMaterialRecords;

    BufferRecord &getOrCreateBufferRecord(const BufferDesc &desc, const BufferKey &key);
    ResourceRecord<Texture> &getOrCreateTextureRecord(AssetID<TextureCPU> cpuAsset, TextureCPU *cpuData);
    ResourceRecord<MaterialGPU> &getOrCreateMaterialRecord(AssetID<Material> cpuAsset, const MaterialGPU &gpuMaterial);
};


MaterialGPU uploadMaterialGPU(
    const AssetID<Material>& matID,
    GPUResourceRegistry& gpuRegistry,
    VulkanContext& context
);
