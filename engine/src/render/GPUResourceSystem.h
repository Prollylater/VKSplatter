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

//Notes: Currently there's no mechanism for repurposing buffer space or defragment allocations
class GPUResourceRegistry
{
public:
    struct BufferAllocation
    {
        VkDeviceSize offset = 0;
        VkDeviceSize size = 0;
        // VkDeviceSize stride;
        //  This used to be used as an Hash for very different allocation in the same Buffer
        //  Currently we are supposed to know the data and supply the allocation index
        //  uint64_t contentHash;
    };

    GPUResourceRegistry() = default;
    ~GPUResourceRegistry() = default;
    // Todo: Better and safer dependency injection
    void initDevice(VulkanContext &ctx)
    {
        mContext = &ctx;
    }
    // Todo:Can i make it more homogeneous
    BufferKey addBuffer(AssetID<void> cpuAsset, const BufferDesc &desc);
    uint32_t allocateInBuffer(BufferKey bufferKey, const BufferAllocation &alloc);

    GPUBufferRef getBuffer(const AssetID<void> cpuAsset, int allocation = 0);
    GPUBufferRef getBuffer(const std::string &name, int allocation = 0);
    GPUBufferRef getBuffer(const BufferKey &key, int allocation = 0);
    
    bool hasBuffer(const BufferKey& key) const;
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
    VulkanContext *mContext;
    std::unordered_map<BufferKey, BufferRecord> mBufferRecords;
    std::unordered_map<uint64_t, ResourceRecord<Texture>> mTextureRecords;
    std::unordered_map<uint64_t, ResourceRecord<MaterialGPU>> mMaterialRecords;

    BufferRecord &acquireBufferRecord(const BufferDesc &desc, const BufferKey &key);
    ResourceRecord<Texture> &acquireTextureRecord(AssetID<TextureCPU> cpuAsset, TextureCPU *cpuData);
    ResourceRecord<MaterialGPU> &acquireMaterialRecord(AssetID<Material> cpuAsset, const MaterialGPU &gpuMaterial);
};
