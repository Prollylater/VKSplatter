#pragma once
#include <glm/glm.hpp>
#include "AssetTypes.h"
#include "Transform.h"
#include "VertexDescriptions.h"
struct Mesh;
struct Material;

// Instance Data structure created on the fly
// Instance buffer read only assume to not change
struct SceneNode
{
    AssetID<Mesh> mesh;
    Transform transform;

    // Notes that despite Instance being generic, shader definition is not
    InstanceLayout layout;
    std::vector<uint8_t> instanceData;
    uint32_t instanceCount = 0;

    uint32_t addInstance()
    {
        uint32_t index = instanceCount++;
        // Replace by other allocator + introduce instance deletion
        instanceData.resize(instanceData.size() + layout.stride);
        memset(&instanceData[index * layout.stride], 0, layout.stride);
        return index;
    }

    uint32_t instanceNb()
    {
        return instanceCount;
    }

    uint8_t *instancePtr(uint32_t index)
    {
        return &instanceData[index * layout.stride];
    }

    InstanceFieldHandle getField(std::string name) const
    {
        for (const auto &f : layout.fields)
        {
            if (f.name == name)
            {
                return {f.offset, f.type};
            }
        }
        return {0, InstanceFieldType::Float};
    }
};

// Todo: This don't check that Fiel exist
inline void setFieldF(SceneNode &node, uint32_t idx, std::string name, float val)
{
    memcpy(node.instancePtr(idx) + node.getField(name).offset, &val, sizeof(float));
}

inline void setFieldM4(SceneNode &node, uint32_t idx, std::string name, uint32_t val)
{
    memcpy(node.instancePtr(idx) + node.getField(name).offset, &val, sizeof(uint32_t));
}

inline void setFieldV3(SceneNode &node, uint32_t idx, std::string name, const glm::vec3 &val)
{
    memcpy(node.instancePtr(idx) + node.getField(name).offset, &val, sizeof(glm::vec3));
}

inline void setFieldV4(SceneNode &node, uint32_t idx, std::string name, const glm::vec4 &val)
{
    memcpy(node.instancePtr(idx) + node.getField(name).offset, &val, sizeof(glm::vec4));
}

inline void setFieldU32(SceneNode &node, uint32_t idx, std::string name, const glm::mat4 &val)
{
    memcpy(node.instancePtr(idx) + node.getField(name).offset, &val, sizeof(glm::mat4));
}

// uint

#include "GPUResourceSystem.h"

struct Drawable
{
    GPUHandle<MeshGPU> meshGPU;
    GPUHandle<MaterialGPU> materialGPU;
    GPUHandle<InstanceGPU> instanceGPU;

    // Todo: Implement 
    bool visible = true; //If drawable are visible the isntance make less sense
    uint32_t indexOffset = 0;
    uint32_t indexCount = 0;

    // uint32_t renderMask;
    //  for filtering by pass directly here
    //  BoundingBox worldBounds; and transform here

    // Binding

    bool bindInstanceBuffer(VkCommandBuffer cmd, const Drawable &drawable, uint32_t bindingIndex);
};

// Todo: Handling a submesh ? Since it would also affect visibility
// Add Pass requirement for automatic drawable disable
