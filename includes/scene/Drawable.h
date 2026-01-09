#pragma once
#include <glm/glm.hpp>
#include "AssetTypes.h"
#include "Transform.h"

struct Mesh;
struct Material;


struct SceneNode
{
    AssetID<Mesh> mesh;
    Transform transform;
    
    // 
   // std::vector<AssetID<Material>> materialOverrides;
};

#include "GPUResourceSystem.h"

struct Drawable
{
    GPUHandle<MeshGPU> meshGPU;
    GPUHandle<MaterialGPU> materialGPU;
    GPUHandle<InstanceGPU> instanceGPU;
    //Todo: Implement 
    bool visible = true;
    uint32_t indexOffset = 0;
    uint32_t indexCount = 0;

    //uint32_t renderMask;
    // for filtering by pass directly here
    // BoundingBox worldBounds; and transform here

    // Binding

    bool bindInstanceBuffer(VkCommandBuffer cmd, const Drawable &drawable, uint32_t bindingIndex);
};

// Todo: Handling a submesh ? Since it would also affect visibility
// Add Pass requirement for automatic drawable disable
