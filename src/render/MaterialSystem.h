#pragma once

#pragma once

#include <unordered_map>
#include <functional>

#include "ContextController.h"
#include "AssetRegistry.h"
#include "GPUResourceSystem.h"
#include "Material.h"
#include "Descriptor.h"

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

    // Explicit update if CPU material changed
    void updateMaterial(
        AssetID<Material> materialId);

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

    void initializeDummyTextures();
};
