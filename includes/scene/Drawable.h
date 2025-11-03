#pragma once
#include "RessourcesGPU.h"
#include <glm/glm.hpp>

struct Drawable
{
    /*
    Once Registry are set up we could move to this
    uint32_t meshIndex;
    uint32_t materialIndex;
    */
    Mesh *mesh;
    MeshGPU meshGPU;

    Material *material;
    MaterialGPU materialGPU;

    // Not sure about more complex data
    struct InstanceData
    {
        glm::mat4 transform = glm::mat4(1.0f);
        glm::vec4 color;
        // Additional data ?
    };
    std::vector<InstanceData> instances;

    bool visible = true;
    // BOunds i guess
    // Then subnodes

    void createMeshGPU(const LogicalDeviceManager &deviceM, VkPhysicalDevice physDevice, uint32_t indice, bool SSBO = false)
    {
        meshGPU = MeshGPU::createMeshGPU(*mesh, deviceM, physDevice, indice, SSBO);
    };

    void createMaterialGPU(const LogicalDeviceManager &deviceM, DescriptorManager& descriptor , VkPhysicalDevice physDevice, uint32_t indice)
    {
        materialGPU = MaterialGPU::createMaterialGPU(*material, deviceM, descriptor, physDevice, indice);
    };
    //DeleteFLAG

    //Todo: Handle delete
    Drawable() = default;
    ~Drawable() = default;
};

// Todo: Handling a submesh ? Since it would also affect visibility
