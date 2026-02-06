#pragma once

#pragma once

#include <unordered_map>
#include <functional>

#include "ContextController.h"
#include "AssetRegistry.h"
#include "GPUResourceSystem.h"
#include "Material.h"
#include "Descriptor.h"

struct MaterialInstance
{
    int pipelineIndex;
    RenderPassType pass;
};

struct MaterialInstanceKey
{
    GPUHandle<MaterialGPU> material;
    RenderPassType pass;
    uint32_t geometryFeatures;

    bool operator==(const MaterialInstanceKey &o) const
    {
        return material == o.material &&
               pass == o.pass &&
               geometryFeatures == o.geometryFeatures;
    }

    struct KeyHash
    {
        size_t operator()(const MaterialInstanceKey &k) const
        {
            auto hashCombine = [](size_t currhash, size_t value)
            {
                return currhash ^ (value + (currhash << 6));
            };
            size_t h = 0;
            hashCombine(h, k.material.getID());
            hashCombine(h, static_cast<uint32_t>(k.pass));
            hashCombine(h, k.geometryFeatures);
            return h;
        }
    };
};

class MaterialSystem
{
public:
    MaterialSystem(
        VulkanContext &context,
        AssetRegistry &assetRegistry,
        GPUResourceRegistry &gpuRegistry);

    ~MaterialSystem();

    // Called by Renderer / Scene extraction
    GPUHandle<MaterialGPU> requestMaterial(
        AssetID<Material> materialId);

    MaterialInstance requestMaterialInstance(
        GPUHandle<MaterialGPU> gpuMat,
        RenderPassType pass,
        uint32_t geometryFeatures);

    MaterialInstance addMaterialInstance(
        GPUHandle<MaterialGPU> gpuMat,
        RenderPassType pass,
        uint32_t geometryFeatures,
        int pipelineIndex);

    // Explicit update if CPU material changed
    void updateMaterial(
        AssetID<Material> materialId);

    DescriptorManager &materialDescriptor()
    {
        return mDescriptorManager;
    }

private:
    MaterialGPU buildMaterialGPU(
        AssetID<Material> materialId);

    GPUHandle<Texture> resolveTexture(AssetID<TextureCPU> id,
                                      AssetID<TextureCPU> dummy);

private:
    VulkanContext &mContext;
    AssetRegistry &mAssets; // Should be const
    GPUResourceRegistry &mGPURegistry;
    DescriptorManager mDescriptorManager;

    // This exist onnly to deduplicate Dummy
    AssetID<TextureCPU> mDummyAlbedoID;
    AssetID<TextureCPU> mDummyNormalID;
    AssetID<TextureCPU> mDummyMetallicID;
    AssetID<TextureCPU> mDummyRoughnessID;
    AssetID<TextureCPU> mDummyEmissiveID;

    std::vector<MaterialInstance> mInstances;
    std::unordered_map<MaterialInstanceKey, uint32_t, MaterialInstanceKey::KeyHash> mInstancesIdx;

    void initializeDummyTextures();
};
