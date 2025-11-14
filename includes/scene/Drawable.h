#pragma once
#include "RessourcesGPU.h"
#include <glm/glm.hpp>
#include "AssetRegistry.h"


struct SceneNode
{
    AssetID<Mesh> mesh;
    AssetID<Material> material;
};

struct Drawable
{
    MeshGPU meshGPU;
    MaterialGPU materialGPU;

    // Not sure about more complex data
    struct InstanceData
    {
        glm::mat4 transform = glm::mat4(1.0f);
        // Could own proper transfrom hierarchy
        glm::vec4 color;
        // Additional data ?
    };
    std::vector<InstanceData> instances;

    bool visible = true;
    // BOunds i guess
    // Then subnodes

    // Todo: Create Drawables from a Mesh should be a fucntion in itself
    void createMeshGPU(const Mesh &mesh, const LogicalDeviceManager &deviceM, VkPhysicalDevice physDevice, const uint32_t indice, bool SSBO = false)
    {
        meshGPU = MeshGPU::createMeshGPU(mesh, deviceM, physDevice, indice, SSBO);
    };

    // Todo: If registry is passed might as well just pass an handle
    void createMaterialGPU(AssetRegistry &registry, const Material &material, const LogicalDeviceManager &deviceM, DescriptorManager &descriptor, VkPhysicalDevice physDevice, const uint32_t indice)
    {
        materialGPU = MaterialGPU::createMaterialGPU(registry, material, deviceM, descriptor, physDevice, indice);
    };
    // DeleteFLAG

    // Todo: Handle delete
    Drawable() = default;
    ~Drawable() = default;
};

// Todo: Handling a submesh ? Since it would also affect visibility
