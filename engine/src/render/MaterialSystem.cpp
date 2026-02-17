#include "MaterialSystem.h"
#include "utils/PipelineHelper.h"

MaterialSystem::MaterialSystem(
    VulkanContext &context,
    AssetRegistry &assetRegistry,
    GPUResourceRegistry &gpuRegistry)
    : mContext(context), mAssets(assetRegistry), mGPURegistry(gpuRegistry)
{
    initializeDummyTextures();
    mDescriptorManager.createDescriptorPool(mContext.getLDevice().getLogicalDevice(), 50, {});
}

MaterialSystem::~MaterialSystem()
{
    auto device = mContext.getLDevice().getLogicalDevice();
    mDescriptorManager.destroyDescriptorLayout(device);
    mDescriptorManager.destroyDescriptorPool(device);
}
// Todo: This is a bit messy, should introduce a get Handle to check the existence of a resource
GPUHandle<Texture> MaterialSystem::resolveTexture(
    AssetID<TextureCPU> id,
    AssetID<TextureCPU> dummy)
{
    if (!id.isValid())
    {
        return GPUHandle<Texture>(dummy.id);
    }

    auto handle = GPUHandle<Texture>{id.getID()};
    if (mGPURegistry.getTexture(handle) != nullptr)
    {
        return handle;
    }

    if (auto cpuTex = mAssets.get(id))
    {
        return mGPURegistry.addTexture(id, cpuTex);
    }

    return GPUHandle<Texture>(dummy.id);
}

namespace
{
    constexpr AssetID<TextureCPU> DummyAlbedoID{UINT64_MAX - 1};
    constexpr AssetID<TextureCPU> DummyNormalID{UINT64_MAX - 2};
    constexpr AssetID<TextureCPU> DummyMetallicID{UINT64_MAX - 3};
    constexpr AssetID<TextureCPU> DummyRoughnessID{UINT64_MAX - 4};
    constexpr AssetID<TextureCPU> DummyEmissiveID{UINT64_MAX - 5};
}

void MaterialSystem::initializeDummyTextures()
{
    auto createDummy = [&](AssetID<TextureCPU> id, TextureCPU (*getter)())
    {
        auto ptrTex = getter();
        // Notes: Might want to use a pointer
        mGPURegistry.addTexture(id, &ptrTex);
    };

    createDummy(DummyAlbedoID, TextureCPU::getDummyAlbedoImage);
    createDummy(DummyNormalID, TextureCPU::getDummyNormalImage);
    createDummy(DummyMetallicID, TextureCPU::getDummyMetallicImage);
    createDummy(DummyRoughnessID, TextureCPU::getDummyRoughnessImage);
    createDummy(DummyEmissiveID, TextureCPU::getDummyMetallicImage);
}

GPUHandle<MaterialGPU> MaterialSystem::requestMaterial(AssetID<Material> materialId)
{
    // AssetID == GPUHandle
    auto mat = mGPURegistry.getMaterial(GPUHandle<MaterialGPU>(materialId));
    if (mat)
    {
        return GPUHandle<MaterialGPU>(materialId.getID());
    }
    return mGPURegistry.addMaterial(
        materialId, buildMaterialGPU(
                        materialId));
}

MaterialInstance MaterialSystem::requestMaterialInstance(
    GPUHandle<MaterialGPU> gpuMat,
    RenderPassType pass,
    uint32_t geometryFeatures)
{
    MaterialInstanceKey key{gpuMat, pass, geometryFeatures};

    auto it = mInstancesIdx.find(key);
    if (it != mInstancesIdx.end())
    {
        return mInstances[it->second];
    }

    return MaterialInstance{-1, RenderPassType::Count};
}

MaterialInstance MaterialSystem::addMaterialInstance(
    GPUHandle<MaterialGPU> gpuMat,
    RenderPassType pass,
    uint32_t geometryFeatures,
    int pipelineIndex)
{
    MaterialInstanceKey key{gpuMat, pass, geometryFeatures};

    auto it = mInstancesIdx.find(key);
    if (it != mInstancesIdx.end())
    {
        return mInstances[it->second];
    }

    // Create new variant
    MaterialInstance instance;
    instance.pipelineIndex = pipelineIndex;
    instance.pass = pass;

    uint32_t idx = static_cast<uint32_t>(mInstances.size());
    mInstances.push_back(instance);
    mInstancesIdx[key] = idx;

    return instance;
}

void MaterialSystem::updateMaterial(AssetID<Material> materialId)
{
    auto material = mGPURegistry.getMaterial(materialId);
    if (material == nullptr)
    {
        return;
    }

    auto UBO = mGPURegistry.getBuffer(material->uniformBuffer);
    const Material &mat = *mAssets.get(materialId);

    // Update UBO only
    UBO.uploadData(mContext, &mat.mConstants, sizeof(Material::MaterialConstants));

    // Update Texture and confirm if texture exist
}

/*
void MaterialSystem::updateMaterialContent(AssetID<Material> materialId)
{
    auto material = mGPURegistry.getMaterial(materialId);
    if (material == nullptr)
    {
        return;
    }

    auto UBO = mGPURegistry.getBuffer(material->uniformBuffer);
    const Material &mat = *mAssets.get(materialId);

    // Update UBO only
    UBO.uploadData(mContext, &mat.mConstants, sizeof(Material::MaterialConstants));

    // Confirm if texture exist, and update or if texture id match
    // I should have two
}
*/

MaterialGPU
MaterialSystem::buildMaterialGPU(
    AssetID<Material> materialId)
{
    const Material &mat = *mAssets.get(materialId);
    const auto device = mContext.getLDevice().getLogicalDevice();

    MaterialGPU gpu{};
    // gpu.pipelineEntryIndex = pipelineEntryIndex;

    // Uniform buffer stuff
    BufferDesc descriptions;
    descriptions.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    descriptions.size = sizeof(Material::MaterialConstants);
    // Todo:
    // Could be immutable and be a big buffer, actually should
    descriptions.updatePolicy = BufferUpdatePolicy::StagingOnly;
    descriptions.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    gpu.uniformBuffer = mGPURegistry.addBuffer(AssetID<void>(materialId.getID()), descriptions);
    mGPURegistry.allocateInBuffer(gpu.uniformBuffer, {.offset = 0, .size = descriptions.size});
    auto bufferRef = mGPURegistry.getBuffer(gpu.uniformBuffer);
    bufferRef.uploadData(mContext, &mat.mConstants, sizeof(Material::MaterialConstants));

    // Textures stuff
    gpu.albedo = resolveTexture(mat.albedoMap, ::DummyAlbedoID);
    gpu.normal = resolveTexture(mat.normalMap, ::DummyRoughnessID);
    gpu.metallic = resolveTexture(mat.metallicMap, ::DummyMetallicID);
    gpu.roughness = resolveTexture(mat.roughnessMap, ::DummyRoughnessID);
    gpu.emissive = resolveTexture(mat.emissiveMap, ::DummyMetallicID);

    // ---- Descriptor
    auto layout = MaterialLayoutRegistry::Get(MaterialType::PBR);
    int matLayoutIdx = mDescriptorManager.getOrCreateSetLayout(device, layout.descriptorSetLayoutsBindings);
    gpu.descriptorIndex =
        mDescriptorManager.allocateDescriptorSet(device, matLayoutIdx);

    VkDescriptorBufferInfo uboInfo =
        mGPURegistry.getBuffer(gpu.uniformBuffer).getDescriptor();

    VkDescriptorImageInfo albedoInfo =
        mGPURegistry.getTexture(gpu.albedo)->getImage().getDescriptor();

    VkDescriptorImageInfo normalInfo =
        mGPURegistry.getTexture(gpu.normal)->getImage().getDescriptor();

    VkDescriptorImageInfo metallicInfo =
        mGPURegistry.getTexture(gpu.metallic)->getImage().getDescriptor();

    VkDescriptorImageInfo roughnessInfo =
        mGPURegistry.getTexture(gpu.roughness)->getImage().getDescriptor();

    VkDescriptorImageInfo emissiveInfo =
        mGPURegistry.getTexture(gpu.emissive)->getImage().getDescriptor();

    VkDescriptorSet set =
        mDescriptorManager.getSet(gpu.descriptorIndex);
    std::vector<VkWriteDescriptorSet> writes{
        vkUtils::Descriptor::makeWriteDescriptor(
            set, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uboInfo),
        vkUtils::Descriptor::makeWriteDescriptor(
            set, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &albedoInfo),
        vkUtils::Descriptor::makeWriteDescriptor(
            set, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &normalInfo),
        vkUtils::Descriptor::makeWriteDescriptor(
            set, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &metallicInfo),
        vkUtils::Descriptor::makeWriteDescriptor(
            set, 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &roughnessInfo),
        vkUtils::Descriptor::makeWriteDescriptor(
            set, 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &emissiveInfo)};

    mDescriptorManager.updateDescriptorSet(device, writes);

    return gpu;
}
