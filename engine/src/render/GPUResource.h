#pragma once
#include "AssetTypes.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include "Texture.h"
#include "ContextController.h"
struct Material;

inline uint64_t makeInstanceKey(
    AssetID<Mesh> meshID,
    AssetID<Material> materialID,
    uint32_t instanceBatchID)
{
    return (static_cast<uint64_t>(meshID.getID()) << 32) |
           (static_cast<uint64_t>(materialID.getID()) << 16) |
           static_cast<uint64_t>(instanceBatchID);
}

template <typename T>
class GPUHandle
{
public:
    uint64_t id = INVALID_ASSET_ID;
    GPUHandle() = default;
    explicit GPUHandle(uint64_t _id) : id(_id) {};

    template <typename T2>
    GPUHandle(AssetID<T2> _id) : id(_id.getID()){};

    uint64_t getID() const { return id; }
    bool valid() const { return id != 0; }

    bool operator==(const GPUHandle &other) const { return id == other.id; }
};

struct GPUBufferRef
{

    VkDescriptorBufferInfo getDescriptor() const
    {
        return {buffer->getBuffer(), offset, size};
    }

    VkDeviceAddress getDeviceAddress(VkDevice device) const
    {
        return 0;
    }

    //Range upload from the first element of the allocation
    void uploadData(VulkanContext &context, const void *src, VkDeviceSize dataSize)
    {
        if (!buffer || dataSize > size)
        {
            return;
        }
        context.updateBuffer(*buffer, src, dataSize, offset);
    }

    //Single element update
    //Assume the object correspon to stride
    //Notes:
    //Todo: Store stride by adding it in description then in allocation ?
    void updateElement(VulkanContext &ctx, const void *data, uint32_t index, VkDeviceSize stride)
    {
        /* Previously we passed dataSize and not size. DataSize was passed to control stride as the feature was halfbaked
        Still need to be reconsiderated
        if (!buffer || stride < size || stride == 0 || stride != dataSize)
        {
            return;
        }

        */
        if (!buffer || offset + stride > size || stride == 0)
        {
            return;
        }

        VkDeviceSize dstOffset = offset + index * stride;

        ctx.updateBuffer(*buffer, data, stride, dstOffset);
    }


    // Should be private
    Buffer *buffer = nullptr;
    VkDeviceSize offset = 0;
    VkDeviceSize size = 0;
    //VkDeviceSize stride = 0;

};

struct BufferKey
{
    uint64_t assetId = INVALID_ASSET_ID;         // AssetID of CPU object or anything else
// VkBufferUsageFlags usage = 0; // Optional further disambiguation

    bool operator==(const BufferKey &other) const
    {
        return assetId == other.assetId ;
    }
};

namespace std
{
    template <>
    struct hash<BufferKey>
    {
        std::size_t operator()(const BufferKey &k) const noexcept
        {
            size_t h1 = std::hash<uint64_t>{}(k.assetId);
            //size_t h2 = std::hash<uint64_t>{}(static_cast<uint64_t>(k.usage));
            return h1;// ^ (h2 << 1);
        }
    };
} //

struct MaterialGPU
{

    int descriptorIndex = -1;

    BufferKey uniformBuffer;
    GPUHandle<Texture> albedo;
    GPUHandle<Texture> normal;
    GPUHandle<Texture> metallic;
    GPUHandle<Texture> roughness;
    GPUHandle<Texture> emissive;

};
