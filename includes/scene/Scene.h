#pragma once
#include "BaseVk.h"
#include "Mesh.h"
#include "Drawable.h"
#include "Material.h"
#include "RessourcesGPU.h"

class Scene
{
public:
    Scene()
    {
        // Todo: Temporary position
        sceneLayout.addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
        sceneLayout.addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UniformBufferObject));
    };

    explicit Scene(int compute)
    {
        sceneLayout.addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
        sceneLayout.addDescriptor(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
        sceneLayout.addDescriptor(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);

        sceneLayout.addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UniformBufferObject));
    };
    ~Scene() = default;

    // Todo: Something about rebuild when an Asset become invalid
    std::vector<SceneNode> nodes;
    void addNode(SceneNode node)
    {
        nodes.push_back(node);
    }
    void clearScene()
    {
        nodes.clear();
    }

    // Camera object
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;

    // Light
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection;
    glm::vec4 sunlightColor;

    PipelineSetLayoutBuilder sceneLayout;
};

class GpuResourceUploader
{
public:
    GpuResourceUploader(const VulkanContext &ctx,
                        const AssetRegistry &assets,
                        DescriptorManager &descriptors,
                        PipelineManager &pipelines);

    MeshGPU buildMeshGPU(const AssetID<Mesh>, bool useSSBO = false) const;
    MaterialGPU buildMaterialGPU(const AssetID<Material> matID, uint32_t descriptorIdx, uint32_t pipelineIndex) const;
    InstanceGPU buildInstanceGPU(const std::vector<InstanceData> &) const;

private:
    const VulkanContext &context;
    const AssetRegistry &assetRegistry;
    DescriptorManager &materialDescriptors;
    PipelineManager &pipelineManager;
};

class RenderScene
{
public:
    RenderScene() = default;
    ~RenderScene() = default;
    void destroy(VkDevice device, VmaAllocator alloc);

    std::vector<Drawable> drawables;

    void syncFromScene(const Scene &cpuScene,
                       const GpuResourceUploader &builder,
                       const std::vector<MaterialGPU::MaterialGPUCreateInfo> &matCaches);
};
