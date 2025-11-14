#include "Mesh.h"
#include "Material.h"
#include <tiny_obj_loader.h>

/*
void Mesh::loadModel(std::string filename)
{
    name = filename;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), "./ressources/models/"))
    {
        throw std::runtime_error(warn + err);
    }

    if (!warn.empty())
    {
        std::cout << "WARN: " << warn << std::endl;
    }

    if (!err.empty())
    {
        std::cerr << "ERR: " << err << std::endl;
    }

    positions.clear();
    normals.clear();
    uvs.clear();
    colors.clear();
    indices.clear();

    bool hasNormals = !attrib.normals.empty();
    bool hasTexcoords = !attrib.texcoords.empty();

    std::unordered_map<VertexUnique, uint32_t, VertexHash> uniqueVertices;

    for (const auto &shape : shapes)
    {
        for (const auto &index : shape.mesh.indices)
        {
            glm::vec3 pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]};

            glm::vec3 norm = glm::vec3(0.0f);
            if (hasNormals && index.normal_index >= 0)
            {
                norm = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]};
            }

            glm::vec2 uv = glm::vec2(0.0f);
            if (hasTexcoords && index.texcoord_index >= 0)
            {
                uv = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
            }

            VertexUnique vertex = {pos, norm, uv};

            if (uniqueVertices.count(vertex) == 0)
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(positions.size());
                positions.push_back(pos);
                normals.push_back(norm);
                uvs.push_back(uv);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }

    std::cout << "Model loaded successfully: " << filename << std::endl;
    std::cout << " - Total unique vertices: " << positions.size() << std::endl;
    std::cout << " - Total indices: " << indices.size() << std::endl;
    std::cout << " - Total shapes: " << shapes.size() << std::endl;
    std::cout << " - Total materials: " << materials.size() << std::endl;

    // Set flags based on what's filled
    inputFlag = Vertex_None;
    if (!positions.empty())
    {
        inputFlag = static_cast<VertexFlags>(inputFlag | Vertex_Pos);
    }
    if (!normals.empty())
    {
        inputFlag = static_cast<VertexFlags>(inputFlag | Vertex_Normal);
    }
    if (!uvs.empty())
    {
        inputFlag = static_cast<VertexFlags>(inputFlag | Vertex_UV);
    }
    if (!colors.empty())
    { // Color is so so in this ? Same as indices to be quite fair
        inputFlag = static_cast<VertexFlags>(inputFlag | Vertex_Color);
    }
    if (!indices.empty())
    {
        inputFlag = static_cast<VertexFlags>(inputFlag | Vertex_Indices);
    }
}
    */
