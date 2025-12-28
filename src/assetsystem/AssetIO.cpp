#include "AssetIO.h"

#include <tiny_obj_loader.h>

//Todo: Proper handling  of cpu destruction 


Material loadMaterial(const tinyobj::material_t &objMaterial)
{
    Material material;
    material.hashedKey = std::hash<std::string>{}(objMaterial.name);
    material.name =  objMaterial.name;

    material.mType = MaterialType::PBR;

    // Material constants (colors, PBR factors)
    material.mConstants.albedoColor = glm::vec4(objMaterial.diffuse[0], objMaterial.diffuse[1], objMaterial.diffuse[2], 1.0f);
    material.mConstants.emissive = glm::vec4(objMaterial.emission[0], objMaterial.emission[1], objMaterial.emission[2], 1.0f);

    material.mConstants.metallic = objMaterial.metallic;
    material.mConstants.roughness = objMaterial.roughness;

    return material;
}

// Todo: Decoyple Asset Registry from t
// Ex: Multi steps with first step sending back Mesh + Material names
Mesh loadMesh(std::string filename)
{
    Mesh mesh;
    mesh.name = filename;
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
        std::cout << "Warning: " << warn << std::endl;
    }

    if (!err.empty())
    {
        std::cerr << "Error: " << err << std::endl;
    }

    mesh.positions.clear();
    mesh.normals.clear();
    mesh.uvs.clear();
    mesh.colors.clear();
    mesh.indices.clear();

    bool hasNormals = !attrib.normals.empty();
    bool hasTexcoords = !attrib.texcoords.empty();

    std::unordered_map<VertexUnique, uint32_t, VertexHash> uniqueVertices;

    // Technichally building submeshes here
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
                uniqueVertices[vertex] = static_cast<uint32_t>(mesh.positions.size());
                mesh.positions.push_back(pos);
                mesh.normals.push_back(norm);
                mesh.uvs.push_back(uv);
            }

            mesh.indices.push_back(uniqueVertices[vertex]);
        }
    }

    std::cout << "Model loaded successfully: " << filename << std::endl;
    std::cout << " - Total unique vertices: " << mesh.positions.size() << std::endl;
    std::cout << " - Total indices: " << mesh.indices.size() << std::endl;
    std::cout << " - Total shapes: " << shapes.size() << std::endl;
    std::cout << " - Total materials: " << materials.size() << std::endl;

    // Set flags based on what's filled
    mesh.inputFlag = Vertex_None;
    if (!mesh.positions.empty())
    {
        mesh.inputFlag = static_cast<VertexFlags>(mesh.inputFlag | Vertex_Pos);
    }
    if (!mesh.normals.empty())
    {
        mesh.inputFlag = static_cast<VertexFlags>(mesh.inputFlag | Vertex_Normal);
    }
    if (!mesh.uvs.empty())
    {
        mesh.inputFlag = static_cast<VertexFlags>(mesh.inputFlag | Vertex_UV);
    }
    if (!mesh.colors.empty())
    { // Color is so so in this ? Same as indices to be quite fair
       // mesh.inputFlag = static_cast<VertexFlags>(mesh.inputFlag | Vertex_Color);
    }
    if (!mesh.indices.empty())
    {
       // mesh.inputFlag = static_cast<VertexFlags>(mesh.inputFlag | Vertex_Indices);
    }

    return mesh;
}


std::unique_ptr<Mesh> AssetIO::loadMeshFromFile(const std::string &path)
{
    return std::make_unique<Mesh>(::loadMesh(path));
};

std::unique_ptr<TextureCPU> AssetIO::loadTextureFromFile(const std::string &path)
{
    //Too generalist
    return std::make_unique<TextureCPU>(::LoadImageTemplate<stbi_uc>(path, STBI_rgb_alpha));
};

std::unique_ptr<Material> AssetIO::loadMaterialFromFile(const std::string &path)
{
    return std::make_unique<Material>(Material{});
};

 
