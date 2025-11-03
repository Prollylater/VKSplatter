#pragma once
#include "BaseVk.h"
#include "Mesh.h"
#include "Drawable.h"
#include "Material.h"
#include "RessourcesGPU.h"

/*
class AssetRegistry {
public:
    //std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes;
    std::vector<Material> material;
    std::vector<Texture> material;

    std::unordered_map<std::string, std::shared_ptr<Material>> materials;
    std::unordered_map<std::string, std::shared_ptr<Texture>> textures;

    std::shared_ptr<Mesh> getMesh(const std::string& name);
    std::shared_ptr<Material> getMaterial(const std::string& name);
    std::shared_ptr<Texture> getTexture(const std::string& name);
}
*/

class Scene
{
public:
    Scene() = default;
    ~Scene() = default;

    void destroy(VkDevice device, VmaAllocator alloc)
    {
        for (auto &drawable : drawables)
        {
            drawable.materialGPU.destroy(device, alloc);
            drawable.meshGPU.destroy(device, alloc);
        };
        for (auto &texture : textures)
        {
            texture.destroyTexture(device, alloc);
        };
        for (auto &mesh : meshes)
        {
        }
    };

    // std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes;
    std::vector<Material> material;
    std::vector<Texture> textures;
    std::vector<Mesh> meshes;

    std::vector<Drawable> drawables;
    VertexFlags flag;
    Mesh getMesh(int index)
    {
        return meshes[index];
    };

    std::vector<Mesh> getMeshes()
    {
        return meshes;
    };

    void registerSceneFormat()
    {
        // Renderer.cpp
        meshes.emplace_back(Mesh());
        Mesh &mesh = meshes.back();
        mesh.loadModel(MODEL_PATH);
        // Todo: Properly awful
        flag = mesh.getflag();

        // Should clear
        VertexFormatRegistry::addFormat(mesh);
        sceneLayout.addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
        sceneLayout.addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UniformBufferObject));
    }

    // Camera object
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;

    // Light
    glm::vec4 ambientColor;
    glm::vec4 sunlightDirection;
    glm::vec4 sunlightColor;

    PipelineLayoutDescriptor sceneLayout;
};
