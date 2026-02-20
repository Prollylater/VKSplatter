
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
#include "VertexDescriptions.h"
#include "utils/RessourceHelper.h"

// Returns a handle to a GPU buffer, possibly a suballocation of an existing one
// @ cpuAsset:
// Todo: Template this
// Could and should be expanded a lot
// Tracking unused buffer and refillling it/Reallocation, explicit offset etc...

// Todo: Reverse the order, cpuAsset can be a variable
BufferKey GPUResourceRegistry::addBuffer(
    AssetID<void> cpuAsset,
    const BufferDesc &desc)
{
    if (!cpuAsset.isValid() && desc.name.empty())
    {
        return {};
    }

    BufferKey key{};
    if (!desc.name.empty())
    {
        // Use name hash
        key.assetId = std::hash<std::string>{}(desc.name);
    }
    else
    {
        key.assetId = cpuAsset.getID();
    }


    auto &record = acquireBufferRecord(desc, key);
    return BufferKey{key.assetId};
}

uint32_t GPUResourceRegistry::allocateInBuffer(BufferKey bufferKey, const GPUResourceRegistry::BufferAllocation &alloc)
{

    // Notes: Create a better function ?
    auto &record = acquireBufferRecord({}, bufferKey);

    VkDeviceSize allocOffset = alloc.offset ? alloc.offset : record.usedSize;

    // Notes: Technically, currently due to lack of fine tuning inside, it's better to avoid passing allocOffset != 0
    // and create hole within. Something to reconsider later
    if (allocOffset + alloc.size > record.buffer->getSize())
    {
        throw std::runtime_error("Offset + size exceeds buffer total size");
    }

    // Check for overlap with existing allocations
    for (const auto &allocation : record.allocations)
    {
        // Overlap if allocOffset is after
        bool isAfter = allocOffset >= allocation.offset + allocation.size;
        bool isBefore = allocOffset + alloc.size <= allocation.offset;
        bool overlap = !(isAfter || isBefore);
        if (overlap)
        {
            throw std::runtime_error("Explicit offset overlaps with existing allocation");
        }
    }

    record.usedSize = std::max(record.usedSize, allocOffset + alloc.size);
    record.usedSize = allocOffset + alloc.size;
    record.allocations.push_back({allocOffset, alloc.size});

    //Todo: Pretty sure i have no rason to return this
    return record.refCount++;
}

GPUBufferRef GPUResourceRegistry::getBuffer(const AssetID<void> cpuAsset, int allocation)
{
    BufferKey key{cpuAsset.getID()};
    return getBuffer(key, allocation);
};
GPUBufferRef GPUResourceRegistry::getBuffer(const std::string &name, int allocation)
{
    BufferKey key{std::hash<std::string>{}(name)};
    return getBuffer(key, allocation);
};

// Todo: Rename to getBufferAllocation
GPUBufferRef GPUResourceRegistry::getBuffer(const BufferKey &key, int allocation)
{
    auto it = mBufferRecords.find(key);
    if (it != mBufferRecords.end())
    {
        const auto &record = it->second;
        if (!record.allocations.empty() && allocation < record.allocations.size())
        {
            auto &alloc = record.allocations[allocation];
            return GPUBufferRef{record.buffer.get(), alloc.offset, alloc.size /*, alloc.stride*/};
        }
    }
    return {};
}

bool GPUResourceRegistry::hasBuffer(const BufferKey &key) const
{
    return mBufferRecords.find(key) != mBufferRecords.end();
}

void GPUResourceRegistry::releaseBuffer(const BufferKey &key)
{
    auto it = mBufferRecords.find(key);
    if (it != mBufferRecords.end())
    {
        auto &rec = it->second;
        if (--rec.refCount == 0)
        {
            auto device = mContext->getLDevice().getLogicalDevice();
            auto allocator = mContext->getLDevice().getVmaAllocator();
            rec.buffer->destroyBuffer(device, allocator);
            mBufferRecords.erase(it);
        }
    }
}

GPUHandle<Texture> GPUResourceRegistry::addTexture(AssetID<TextureCPU> cpuAsset, TextureCPU *cpuData)
{
    if (!cpuAsset.isValid())
    {
        return GPUHandle<Texture>{};
    }

    auto &rec = acquireTextureRecord(cpuAsset, cpuData);
    return GPUHandle<Texture>{cpuAsset.getID()};
}

Texture *GPUResourceRegistry::getTexture(const GPUHandle<Texture> &handle)
{
    auto it = mTextureRecords.find(handle.getID());
    return it != mTextureRecords.end() ? it->second.resource.get() : nullptr;
}

void GPUResourceRegistry::releaseTexture(const GPUHandle<Texture> &handle)
{
    auto it = mTextureRecords.find(handle.getID());
    if (it != mTextureRecords.end())
    {
        auto &rec = it->second;
        if (--rec.refCount == 0)
        {
            auto device = mContext->getLDevice().getLogicalDevice();
            auto allocator = mContext->getLDevice().getVmaAllocator();
            rec.resource->destroy(device, allocator);
            mTextureRecords.erase(it);
        }
    }
}

GPUHandle<MaterialGPU> GPUResourceRegistry::addMaterial(AssetID<Material> cpuAsset, MaterialGPU gpuMaterial)
{
    if (!cpuAsset.isValid())
    {
        return GPUHandle<MaterialGPU>{};
    }
    auto &rec = acquireMaterialRecord(cpuAsset, gpuMaterial);
    return GPUHandle<MaterialGPU>{cpuAsset.getID()};
}

MaterialGPU *GPUResourceRegistry::getMaterial(const GPUHandle<MaterialGPU> &handle)
{
    auto it = mMaterialRecords.find(handle.getID());
    return it != mMaterialRecords.end() ? it->second.resource.get() : nullptr;
}

void GPUResourceRegistry::releaseMaterial(const GPUHandle<MaterialGPU> &handle)
{
    auto it = mMaterialRecords.find(handle.getID());
    if (it != mMaterialRecords.end())
    {
        auto &rec = it->second;
        if (--rec.refCount == 0)
        {
            releaseBuffer(rec.resource->uniformBuffer);
            releaseTexture(rec.resource->albedo);
            releaseTexture(rec.resource->normal);
            releaseTexture(rec.resource->metallic);
            releaseTexture(rec.resource->roughness);
            releaseTexture(rec.resource->emissive);
            mMaterialRecords.erase(it);
        }
    }
}

void GPUResourceRegistry::clearAll()
{
    auto device = mContext->getLDevice().getLogicalDevice();
    auto allocator = mContext->getLDevice().getVmaAllocator();

    for (auto &record : mBufferRecords)
    {
        record.second.buffer->destroyBuffer(device, allocator);
    }
    mBufferRecords.clear();

    for (auto &record : mTextureRecords)
    {
        record.second.resource->destroy(device, allocator);
    }
    mTextureRecords.clear();
    mMaterialRecords.clear();
}

GPUResourceRegistry::BufferRecord &GPUResourceRegistry::acquireBufferRecord(const BufferDesc &desc, const BufferKey &key)
{
    auto it = mBufferRecords.find(key);
    if (it != mBufferRecords.end())
    {
        it->second.refCount++;
        return it->second;
    }

    auto buffer = std::make_unique<Buffer>(mContext->createBuffer(desc));
    auto [insertIt, inserted] = mBufferRecords.emplace(key, BufferRecord{std::move(buffer), {}, 0, 0});
    return insertIt->second;
}

GPUResourceRegistry::ResourceRecord<Texture> &GPUResourceRegistry::acquireTextureRecord(AssetID<TextureCPU> cpuAsset, TextureCPU *cpuData)
{
    auto it = mTextureRecords.find(cpuAsset.getID());
    if (it != mTextureRecords.end())
    {
        it->second.refCount++;
        return it->second;
    }

    if (!cpuData)
    {
        throw std::runtime_error("Cannot create GPU texture without CPU data");
    }
    auto tex = std::make_unique<Texture>(mContext->createTexture(*cpuData));
    auto [insertIt, inserted] = mTextureRecords.emplace(cpuAsset.getID(), ResourceRecord<Texture>{std::move(tex)});
    return insertIt->second;
}

GPUResourceRegistry::ResourceRecord<MaterialGPU> &GPUResourceRegistry::acquireMaterialRecord(AssetID<Material> cpuAsset, const MaterialGPU &gpuMaterial)
{
    auto it = mMaterialRecords.find(cpuAsset.getID());
    if (it != mMaterialRecords.end())
    {
        it->second.refCount++;
        return it->second;
    }

    auto mat = std::make_unique<MaterialGPU>(gpuMaterial);
    auto [insertIt, inserted] = mMaterialRecords.emplace(cpuAsset.getID(), ResourceRecord<MaterialGPU>{std::move(mat)});
    return insertIt->second;
}
