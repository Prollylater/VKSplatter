#pragma once
#include "GPUResource.h"
#include "AssetTypes.h"
#include "Texture.h"
#include <functional>
#include <unordered_map>

template <typename T>
class GPUHandle
{
public:
    uint64_t id = INVALID_ASSET_ID;

    GPUHandle() = default;
    explicit GPUHandle(uint64_t _id) : id(_id) {}

    uint64_t getID() const { return id; }
    bool valid() const { return id != 0; }

    bool operator==(const GPUHandle & other)  const { return id == other.id; }

};

//Are that many forward declaration useful ?
class Mesh;
class VulkanContext;
class DescriptorManager;
class PipelineManager;
class GPUResourceRegistry;
class AssetRegistry;

class GpuResourceUploader
{
public:
    GpuResourceUploader(const VulkanContext &ctx,
                        const AssetRegistry &assets,
                        DescriptorManager &descriptors,
                        PipelineManager &pipelines) : context(ctx), assetRegistry(assets), materialDescriptors(descriptors),
                                                      pipelineManager(pipelines) {};

    MeshGPU uploadMeshGPU(const AssetID<Mesh>, bool useSSBO = false) const;
    MaterialGPU uploadMaterialGPU(const AssetID<Material> matID, GPUResourceRegistry &gpuRegistry, int descriptorIdx, int pipelineIndex) const;
    InstanceGPU uploadInstanceGPU(const std::vector<InstanceData> &) const;
    Texture uploadTexture(const AssetID<TextureCPU>) const;

private:
    const VulkanContext &context;
    const AssetRegistry &assetRegistry;
    DescriptorManager &materialDescriptors;
    PipelineManager &pipelineManager;
};


class GPUResourceRegistry
{
public:
     GPUResourceRegistry() = default;

    //The use of function here is a bit rigid
    template <typename CpuT, typename GpuT>
    GPUHandle<GpuT> add(const AssetID<CpuT> asset, std::function<GpuT()> uploader)
    {
        auto &map = getMap<GpuT>();
        // same as AssetID for CPU-based resources
        uint64_t key = asset.getID();

        auto it = map.find(key);
        if (it != map.end())
        {
            it->second.refCount++;
            return GPUHandle<GpuT>{key};
        }

        map.emplace(key, GPURessRecord<GpuT>{
                                  .resource = std::make_unique<GpuT>(uploader()),
                                  .refCount = 1});
        return GPUHandle<GpuT>{key};
    }

    template <typename GpuT>
    GpuT *get(const GPUHandle<GpuT> &handle)
    {
        auto &map = getMap<GpuT>();
        auto it = map.find(handle.getID());

        if (it != map.end())
        {
            return it->second.resource.get();
        }
        return nullptr;
    }

    template <typename GpuT>
    void release(GPUHandle<GpuT> handle, VkDevice device, VmaAllocator allocator = nullptr)
    {
        auto &map = getMap<GpuT>();
        auto it = map.find(handle.getID());
        if (it != map.end())
        {
            if (--it->second.refCount == 0)
            {
                it->second.resource->destroy(device, allocator);
                map.erase(it);
            }
        }
    }
    

    void clearAll(VkDevice device, VmaAllocator allocator = nullptr);
private:
    template <typename GpuT>
    struct GPURessRecord
    {
        std::unique_ptr<GpuT> resource;
        uint32_t refCount = 0;
    };

    std::unordered_map<uint64_t, GPURessRecord<MeshGPU>> meshes;
    std::unordered_map<uint64_t, GPURessRecord<MaterialGPU>> materials;
    std::unordered_map<uint64_t, GPURessRecord<Texture>> textures;
    std::unordered_map<uint64_t, GPURessRecord<InstanceGPU>> instances;

    template <typename GpuT>
    std::unordered_map<uint64_t, GPURessRecord<GpuT>> &getMap();

    template <typename GpuT>
    const std::unordered_map<uint64_t, GPURessRecord<GpuT>> &getMap() const;
};


template void GPUResourceRegistry::release(GPUHandle<MeshGPU> handle, VkDevice device, VmaAllocator allocator = nullptr);
template void GPUResourceRegistry::release(GPUHandle<MaterialGPU> handle, VkDevice device, VmaAllocator allocator = nullptr);
template void GPUResourceRegistry::release(GPUHandle<Texture> handle, VkDevice device, VmaAllocator allocator = nullptr);
//template void GPUResourceRegistry::release(GPUHandle<InstanceGPU> handle, VkDevice device, VmaAllocator allocator = nullptr)
