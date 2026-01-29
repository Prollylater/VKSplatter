#include "MaterialSystem.h"
#include "PipelineHelper.h"

MaterialSystem::MaterialSystem(
    VulkanContext &context,
    AssetRegistry &assetRegistry,
    GPUResourceRegistry &gpuRegistry)
    : mContext(context), mAssets(assetRegistry), mGPURegistry(gpuRegistry)
{
    initializeDummyTextures();
    mDescriptorManager.createDescriptorPool(context.getLDevice().getLogicalDevice(), 50, {});
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
        GPUHandle<Texture>(dummy.id);
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

    // Step 3: fallback to dummy
    return GPUHandle<Texture>(dummy.id);
}

void MaterialSystem::initializeDummyTextures()
{
    auto createDummy = [&](AssetID<TextureCPU> &id, TextureCPU *(*getter)() )
    {
        static uint64_t dummyCounter = INVALID_ASSET_ID;
        id = AssetID<TextureCPU>{--dummyCounter};
        auto* ptrTex = getter();
        mGPURegistry.addTexture(
            id, ptrTex);
    };

    createDummy(mDummyAlbedoID, TextureCPU::getDummyAlbedoImage);
    createDummy(mDummyNormalID, TextureCPU::getDummyNormalImage);
    createDummy(mDummyMetallicID, TextureCPU::getDummyMetallicImage);
    createDummy(mDummyRoughnessID, TextureCPU::getDummyRoughnessImage);
    createDummy(mDummyEmissiveID, TextureCPU::getDummyMetallicImage);
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
    auto layout = MaterialLayoutRegistry::Get(mat.mType);
    const auto device = mContext.getLDevice().getLogicalDevice();

    MaterialGPU gpu{};
    // gpu.pipelineEntryIndex = pipelineEntryIndex;

    // Uniform buffer stuff
    BufferDesc descriptions;
    descriptions.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    descriptions.ssbo = false;
    descriptions.size = sizeof(Material::MaterialConstants);
    // Todo:
    // Could be immutable and be a big buffer, actually should
    descriptions.updatePolicy = BufferUpdatePolicy::StagingOnly;
    descriptions.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    gpu.uniformBuffer = mGPURegistry.addBuffer(AssetID<void>(materialId.getID()), descriptions);
    auto bufferRef = mGPURegistry.getBuffer(gpu.uniformBuffer);
    bufferRef.uploadData(mContext, &mat.mConstants, sizeof(Material::MaterialConstants));

    // Textures stuff
    gpu.albedo = resolveTexture(mat.albedoMap, mDummyAlbedoID);
    gpu.normal = resolveTexture(mat.normalMap, mDummyRoughnessID);
    gpu.metallic = resolveTexture(mat.metallicMap, mDummyMetallicID);
    gpu.roughness = resolveTexture(mat.roughnessMap, mDummyRoughnessID);
    gpu.emissive = resolveTexture(mat.emissiveMap, mDummyMetallicID);

    // ---- Descriptor
    int matLayoutIdx = mDescriptorManager.getOrCreateSetLayout(mContext.getLDevice().getLogicalDevice(), layout.descriptorSetLayoutsBindings);
    gpu.descriptorIndex =
        mDescriptorManager.allocateDescriptorSet(device, matLayoutIdx);

    VkDescriptorSet set =
        mDescriptorManager.getSet(gpu.descriptorIndex);
    std::vector<VkWriteDescriptorSet> writes{
        vkUtils::Descriptor::makeWriteDescriptor(
            set, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            &mGPURegistry.getBuffer(gpu.uniformBuffer).getDescriptor()),
        vkUtils::Descriptor::makeWriteDescriptor(
            set, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            nullptr, &mGPURegistry.getTexture(gpu.albedo)->getImage().getDescriptor()),
        vkUtils::Descriptor::makeWriteDescriptor(
            set, 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            nullptr, &mGPURegistry.getTexture(gpu.normal)->getImage().getDescriptor()),
        vkUtils::Descriptor::makeWriteDescriptor(
            set, 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            nullptr, &mGPURegistry.getTexture(gpu.metallic)->getImage().getDescriptor()),
        vkUtils::Descriptor::makeWriteDescriptor(
            set, 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            nullptr, &mGPURegistry.getTexture(gpu.roughness)->getImage().getDescriptor()),
        vkUtils::Descriptor::makeWriteDescriptor(
            set, 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            nullptr, &mGPURegistry.getTexture(gpu.emissive)->getImage().getDescriptor())};

    mDescriptorManager.updateDescriptorSet(device, writes);

    return gpu;
}
