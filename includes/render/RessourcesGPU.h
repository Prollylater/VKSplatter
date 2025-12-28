#pragma once
#include "BaseVk.h"

struct Mesh;
struct Material;
class LogicalDeviceManager;
class DescriptorManager;
class AssetRegistry;
class GPUResourceRegistry;

// Todo: Can this also be shared ? Depending on howw other color data are handled
struct MaterialGPU
{
    int pipelineEntryIndex = -1;
    int descriptorIndex = -1;

    // Not allocated
    VkBuffer uniformBuffer = VK_NULL_HANDLE;
    VmaAllocation uniformBufferAlloc = VK_NULL_HANDLE; // Memory handle
    VkDeviceMemory uniformBufferMem = VK_NULL_HANDLE;

    void destroy(VkDevice device, VmaAllocator alloc = VK_NULL_HANDLE);

    struct MaterialGPUCreateInfo
    {
        AssetID<Material> cpuMaterial;
        // Todo: uint32_t
        int descriptorLayoutIdx;
        int pipelineIndex;
    };

    static MaterialGPU createMaterialGPU(const AssetRegistry &registry, GPUResourceRegistry &gpuRegistry,
                                         MaterialGPUCreateInfo info, const LogicalDeviceManager &deviceM, DescriptorManager &descriptor, VkPhysicalDevice physDevice, uint32_t indice);
};

struct MeshGPU
{
    // TOdo: SHould i derectly use my Buffer class ?
    //  Cleanup object in case the mesh buffer is not kept
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VmaAllocation vertexAlloc = VK_NULL_HANDLE;
    VmaAllocation indexAlloc = VK_NULL_HANDLE;
    VkDeviceMemory vertexMem = VK_NULL_HANDLE;
    VkDeviceMemory indexMem = VK_NULL_HANDLE;

    VkDeviceAddress vertexAddress = 0;

    uint32_t vertexCount = 0; // Not too sure if this would be useful
    uint32_t indexCount = 0;
    uint32_t vertexStride = 0;

    static MeshGPU createMeshGPU(const Mesh &mesh, const LogicalDeviceManager &deviceM, const VkPhysicalDevice &physDevice, uint32_t indice, bool SSBO = false);
    // void bind()
    void destroy(VkDevice device, VmaAllocator alloc = VK_NULL_HANDLE);
};

class Texture;

struct InstanceData;
struct InstanceGPU
{
    VkBuffer instanceBuffer = VK_NULL_HANDLE;
    VmaAllocation instanceAlloc = VK_NULL_HANDLE;
    VkDeviceMemory instanceMem = VK_NULL_HANDLE;
    // VkDeviceAddress indexAddress = 0;

    uint32_t instanceCount;
    uint32_t instanceStride;

    static InstanceGPU createInstanceGPU(const std::vector<InstanceData> &mesh, const LogicalDeviceManager &deviceM, const VkPhysicalDevice &physDevice, uint32_t indice);
    // void bind()
    void destroy(VkDevice device, VmaAllocator alloc = VK_NULL_HANDLE);
};

struct GPUBufferView
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceAddress mVertexBufferAddress = 0;
};

/*

struct GPUBufferView
{
    VkBuffer mBuffer = VK_NULL_HANDLE;
    VkDeviceAddress mVertexBufferAddress = 0;
    // Goal would be to pass this to an Uniform Buffer
    VkDeviceSize mOffset = 0;
    VkDeviceSize mSize = 0;
};

struct AttributeStream // An elements ?
{
    GPUBufferView mView;
    uint32_t mStride = 0;
    // Todo:
    // Decide how to handle it in regard to the actual buffer
    // Specifally with the build interleaved and so on.
    // This is only really fine due to how we construct our stuff but it render the whole MeshGPuRessources abstraction useless
    // Since the descriptor handle everything well enough
    // Using Attributes just assume everything is neatly deferred to the descriptor
    // Main use so far is for non interleaved and to allow us to drop the Buffer once created
    enum class Type
    {
        Attributes,
        /*Pos, Normal, UV, Color,* / Index
    } mType;
};

class MeshGPUResources
{
public:
    void addStream(GPUBufferView view, uint32_t stride, AttributeStream::Type type)
    {
        mStreams.push_back({view, stride, type});
    };

    AttributeStream getStream(uint32_t index)
    {
        return mStreams[index];
    }

    VkBuffer getStreamBuffer(uint32_t index)
    {
        return mStreams[index].mView.mBuffer;
    }

private:
    std::vector<AttributeStream> mStreams;
};

*/

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
class VulkanContext;
class DescriptorManager;
class PipelineManager;


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
                                  .ressource = std::make_unique(uploader()),
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
            return it->second.asset.get();
        }
        return nullptr;
    }

    template <typename GpuT>
    void release(GPUHandle<GpuT> handle, VkDevice device, VmaAllocator allocator = nullptr)
    {
        auto &map = getMap<GpuT>();
        auto it = map.find(handle);
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

// Todo: Additional reminder on removing such calls

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



template void GPUResourceRegistry::release(GPUHandle<MeshGPU> handle, VkDevice device, VmaAllocator allocator = nullptr);
template void GPUResourceRegistry::release(GPUHandle<MaterialGPU> handle, VkDevice device, VmaAllocator allocator = nullptr);
template void GPUResourceRegistry::release(GPUHandle<Texture> handle, VkDevice device, VmaAllocator allocator = nullptr);
//template void GPUResourceRegistry::release(GPUHandle<InstanceGPU> handle, VkDevice device, VmaAllocator allocator = nullptr)
