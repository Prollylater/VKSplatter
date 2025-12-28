#pragma once
#include "BaseVk.h"
#include "Mesh.h"
#include "Drawable.h"
#include "Material.h"
#include "RessourcesGPU.h"
#include "Camera.h"

// Todo: Move it somewhere else close to Vertex maybe in a common Tpes
struct SceneData
{
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection;
    glm::vec4 sunlightColor;
    glm::mat4 viewproj;
    // Some card may require UBo to be padded
};

class Scene
{
public:
    Scene()
    {
        sceneLayout.addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
        sceneLayout.addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SceneData));
    };

    explicit Scene(int compute)
    {
        sceneLayout.addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
        sceneLayout.addDescriptor(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);
        sceneLayout.addDescriptor(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);

        sceneLayout.addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SceneData));
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
    Camera camera;

    // Light
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection;
    glm::vec4 sunlightColor;

    PipelineSetLayoutBuilder sceneLayout;

    // Temporary
    Camera &getCamera() { return camera; };
    SceneData getSceneData()
    {
        glm::mat4 proj = camera.getProjectionMatrix();
        proj[1][1] *= -1;

        return {ambientColor, sunlightDirection, sunlightColor, proj * camera.getViewMatrix()};
    };
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
                        GPUResourceRegistry& registry,
                       const std::vector<MaterialGPU::MaterialGPUCreateInfo> &matCaches);
    struct PassRequirements
    {
        bool needsMaterial = false;
        bool needsMesh = true;
        bool needsTransform = true;
    };
    /*


enum RenderBits {
    RENDER_BIT_MAIN = 1 << 0,
    RENDER_BIT_SHADOW = 1 << 1,
    RENDER_BIT_REFLECTION = 1 << 2,
    RENDER_BIT_DEBUG = 1 << 3,
};
*/
};
