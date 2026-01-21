#pragma once
#include <glm/glm.hpp>
#include "AssetTypes.h"
#include "Transform.h"
#include "VertexDescriptions.h"
#include "geometry/Extents.h"

struct Mesh;
struct Material;

// Instance Data structure created on the fly
// Instance buffer read only assume to not change

struct InstanceTransform
{
    glm::vec4 position_scale;
    glm::quat rotation;
};

//Notes: The generic interface is very messy and not that much useful
//currently nor might ever be
//A simpler approach might be better
struct SceneNode
{
    AssetID<Mesh> mesh;
    Extents nodeExtents;

    // Hot field: transforms
    // This could also be an instance Data equivalent
    // for element that are often changed
    std::vector<Transform> transforms;
    // Generic instance data (cold)
    InstanceLayout layout;
    std::vector<uint8_t> instanceData;
    uint32_t instanceCount = 0;
    bool visible = true;

    uint32_t addInstance()
    {
        uint32_t index = instanceCount++;
        // Replace by other allocator + introduce instance deletion
        instanceData.resize(instanceData.size() + layout.stride);
        memset(&instanceData[index * layout.stride], 0, layout.stride);

        transforms.push_back(Transform{});
        return index;
    }

    uint32_t instanceNb() const
    {
        return instanceCount;
    }

    uint8_t *instancePtr(uint32_t instanceIndex)
    {
        return &instanceData[instanceIndex * layout.stride];
    }

    InstanceFieldHandle getField(std::string name) const
    {
        for (const auto &f : layout.fields)
        {
            if (f.name == name)
            {
                return {f.offset, f.type, true};
            }
        }
        return {0, InstanceFieldType::Float, false};
    }

    template <typename T>
    T *getFieldPtr(uint32_t instanceIndex, const std::string &name)
    {
        auto h = getField(name);
        if (!h.valid)
            return nullptr;
        return reinterpret_cast<T *>(instancePtr(instanceIndex) + h.offset);
    }

    std::vector<uint8_t> getGenericData(uint32_t instanceIndex)  const
    {
        std::vector<uint8_t> data;

        if (instanceIndex * layout.stride >= instanceData.size()){
            return data;}

        const uint8_t *src = &instanceData[instanceIndex * layout.stride];

        data.resize(layout.stride);
        std::memcpy(data.data(), src, layout.stride);

        return data;
    }

    // Transform accessors
    Transform &getTransform(uint32_t instanceIndex)
    {
        return transforms[instanceIndex];
    }

    const Transform &getTransform(uint32_t instanceIndex) const
    {
        return transforms[instanceIndex];
    }

    void setTransform(uint32_t instanceIndex, const Transform &t)
    {
        transforms[instanceIndex] = t;
    }

    InstanceTransform buildGPUTransform(uint32_t instanceIndex) const
    {
        InstanceTransform t{};

        const Transform &tr = transforms[instanceIndex];

        const auto pos = tr.getPosition();
        t.position_scale = glm::vec4(pos[0], pos[1], pos[2], tr.getScale()[0]);
        t.rotation = tr.getRotation();

        return t;
    }
};

// Todo: This don't check that Fiel exist
inline void setFieldF(SceneNode &node, uint32_t idx, std::string name, float val)
{
    memcpy(node.instancePtr(idx) + node.getField(name).offset, &val, sizeof(float));
}

inline void setFieldU32(SceneNode &node, uint32_t idx, std::string name, uint32_t val)
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

inline void setFieldM4(SceneNode &node, uint32_t idx, std::string name, const glm::mat4 &val)
{
    memcpy(node.instancePtr(idx) + node.getField(name).offset, &val, sizeof(glm::mat4));
}

// uint

#include "GPUResourceSystem.h"

struct DrawableKey
{
    uint32_t nodeIndex;
    uint32_t submeshIndex;

    bool operator==(const DrawableKey &other) const
    {
        return nodeIndex == other.nodeIndex &&
               submeshIndex == other.submeshIndex;
    }
};

// Todo:
// Take the time to compare
struct Drawable
{
    GPUHandle<MeshGPU> meshGPU;
    GPUHandle<MaterialGPU> materialGPU;
    GPUHandle<InstanceGPU> coldInstanceGPU;
    GPUHandle<InstanceGPU> hotInstanceGPU;

    // Extents worldExtent;

    // Todo: Implement
    // bool visible = true; //If drawable are visible the isntance make less sense
    uint32_t indexOffset = 0;
    uint32_t indexCount = 0;
    uint32_t instanceCount = 0;
    int pipelineEntryIndex;
    // RenderFlags flags;   // shadow, transparency, etc.

    bool bindInstanceBuffer(VkCommandBuffer cmd, const Drawable &drawable, uint32_t bindingIndex);
};

// Todo: Handling a submesh ? Since it would also affect visibility
// Add Pass requirement for automatic drawable disable