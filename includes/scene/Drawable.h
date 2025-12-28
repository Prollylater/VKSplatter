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
    GPUHandle<MeshGPU> meshGPU;
    GPUHandle<MaterialGPU> materialGPU;
    GPUHandle<InstanceGPU> instanceGPU;
    bool visible = true;
    //  uint32_t parentIndex;
    //    uint32_t renderMask;
    // for filtering by pass directly here
    // Add bounds i guess
    //    BoundingBox worldBounds;

    // Binding

    bool bindInstanceBuffer(VkCommandBuffer cmd, const Drawable &drawable, uint32_t bindingIndex);
};

// Todo: Handling a submesh ? Since it would also affect visibility
// Add Pass requirement for automatic drawable disable
