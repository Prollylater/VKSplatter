#pragma once
#include "VertexDescriptions.h"
#include "RessourcesGPU.h"

struct Mesh
{
    std::string name;
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> colors;
    std::vector<uint32_t> indices;
    //Todo: Alignment is slightly off but not bothering right now
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

    std::vector<AssetID<Material>> materialIds;
};

/*

    struct Submesh
    {
        uint32_t indexOffset;
        uint32_t indexCount;
        uint32_t vertexOffset;
        uint32_t vertexCount;
        AssetID<Material> materialIds;
    };
    std::vector<Submesh> submeshes;


*/

//Frame dependant too
// Not sure about more complex data nor where  to put them
struct InstanceData
{
    glm::mat4 transform = glm::mat4(1.0f);
    glm::vec4 color;
};