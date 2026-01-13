#pragma once
// #include "AssetTypes.h"
#include "VertexDescriptions.h"
#include "Material.h"
#include "geometry/Extents.h"

//Helper for plane geometry,
//Geometry config concept to abstract away tinyloader

struct Mesh : AssetBase
{

    // static AssetType getStaticType() { return AssetType::Mesh; }
    // AssetType type() const override { return getStaticType(); }

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> colors; //Remove
    std::vector<uint32_t> indices;
    Extents bndbox;
    // Todo: Alignment is slightly off but not bothering right now
    VertexFlags inputFlag;

    size_t vertexCount() const
    {
        return positions.size(); // assume others match or are empty
    }

    const VertexFormat &getFormat() const
    {
        return VertexFormatRegistry::getFormat(inputFlag);
    }

    const VertexFlags &getflag() const
    {
        return inputFlag;
    }

    bool validateMesh(VertexFlags flags)
    {
        size_t count = positions.size();
        if ((flags & Vertex_Normal) && normals.size() != count)
            return false;
        if ((flags & Vertex_Color) && colors.size() != count)
            return false;
        if ((flags & Vertex_UV) && uvs.size() != count)
            return false;
        return true;
    }

    
    struct Submesh
    {
        uint32_t indexOffset;
        uint32_t indexCount;
        uint32_t vertexOffset;
        //AssetID<Material>
        uint32_t materialId;
    };
    std::vector<Submesh> submeshes;

    std::vector<AssetID<Material>> materialIds;
};

