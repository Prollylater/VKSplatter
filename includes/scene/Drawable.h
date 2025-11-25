#pragma once
#include "RessourcesGPU.h"
#include <glm/glm.hpp>
#include "AssetRegistry.h"

struct SceneNode
{
    AssetID<Mesh> mesh;
    AssetID<Material> material;
};
// https://giordi91.github.io/post/resourcesystem/

//Todo: Buffer registry structure + Handles ? Handles are good
struct Drawable
{
    MeshGPU meshGPU;
    MaterialGPU materialGPU;
    InstanceGPU instanceGPU;
    bool visible = true;
    // Add bounds i guess

  
    //Binding 

    bool bindInstanceBuffer(VkCommandBuffer cmd, const Drawable &drawable, uint32_t bindingIndex);

    // Todo: Handle delete

    /*void createMeshGPU(const Mesh &mesh, const LogicalDeviceManager &deviceM, VkPhysicalDevice physDevice, const uint32_t indice, bool SSBO = false);
    void createMaterialGPU(AssetRegistry &registry, const Material &material, const LogicalDeviceManager &deviceM, DescriptorManager &descriptor, VkPhysicalDevice physDevice, const uint32_t indice);
    void createInstanceGPU(const std::vector<InstanceData>& instances, const LogicalDeviceManager &deviceM, VkPhysicalDevice physDevice, const uint32_t indice);
    */
};


// Todo: Handling a submesh ? Since it would also affect visibility
