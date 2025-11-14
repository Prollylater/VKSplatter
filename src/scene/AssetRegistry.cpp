#include "AssetRegistry.h"

#include <tiny_obj_loader.h>


// Todo:
// Create a version of loadMaterial directly using our "Texture Class"
Material loadMaterial(AssetRegistry &assets, const tinyobj::material_t &objMaterial)
{
    Material material;

    material.mType = MaterialType::PBR;

    // Material constants (colors, PBR factors)
    material.mConstants.albedoColor = glm::vec4(objMaterial.diffuse[0], objMaterial.diffuse[1], objMaterial.diffuse[2], 1.0f);
    material.mConstants.emissive = glm::vec4(objMaterial.emission[0], objMaterial.emission[1], objMaterial.emission[2], 1.0f);

    material.mConstants.metallic = objMaterial.metallic;
    material.mConstants.roughness = objMaterial.roughness;

    // Load texture maps names
    if (!objMaterial.diffuse_texname.empty())
    {
        Texture texture(objMaterial.diffuse_texname);
        AssetID textureID = assets.add<Texture>(objMaterial.diffuse_texname, std::make_unique<Texture>(texture));
        material.albedoMap = textureID;
    }

    if (!objMaterial.bump_texname.empty())
    {
        Texture texture(objMaterial.bump_texname);
        AssetID textureID = assets.add<Texture>(objMaterial.diffuse_texname, std::make_unique<Texture>(texture));
        material.normalMap = textureID;
    }
    if (!objMaterial.roughness_texname.empty())
    {
        Texture texture(objMaterial.metallic_texname);
        AssetID textureID = assets.add<Texture>(objMaterial.diffuse_texname, std::make_unique<Texture>(texture));
        material.metallicMap = textureID;
    }

    if (!objMaterial.roughness_texname.empty())
    {
        Texture texture(objMaterial.roughness_texname);
        AssetID textureID = assets.add<Texture>(objMaterial.diffuse_texname, std::make_unique<Texture>(texture));
        material.roughnessMap = textureID;
    }

    return material;
}

// Todo: Decoyple Asset Registry from t
// Ex: Multi steps with first step sending back Mesh + Material names
Mesh loadMesh(AssetRegistry &assets, std::string filename)
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

    // Load materials through the registry
    for (const auto &mtl : materials)
    {
        Material material = loadMaterial(assets, mtl);
        AssetID materialID = assets.add<Material>(mtl.name, std::make_unique<Material>(material));
        mesh.materialIds.push_back(materialID);
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
//Todo:
std::unique_ptr<Mesh> AssetRegistry::loadMesh(const std::string &path)
{
    return std::make_unique<Mesh>(::loadMesh(*this,path));
};

std::unique_ptr<Texture> AssetRegistry::loadTexture(const std::string &path)
{
    return std::make_unique<Texture>(Texture{});
};

std::unique_ptr<Material>  AssetRegistry::loadMaterial(const std::string &path)
{
    return std::make_unique<Material>(Material{});
};


template <>
inline std::unordered_map<uint32_t, AssetRegistry::AssetRecord<Mesh>> &AssetRegistry::getAssetMap<Mesh>()
{
    return meshes;
}

template <>
inline std::unordered_map<uint32_t, AssetRegistry::AssetRecord<Texture>> &AssetRegistry::getAssetMap<Texture>()
{
    return textures;
}

template <>
inline std::unordered_map<uint32_t, AssetRegistry::AssetRecord<Material>> &AssetRegistry::getAssetMap<Material>()
{
    return materials;
}

template <>
std::unique_ptr<Mesh> AssetRegistry::loadAsset(const std::string &name)
{
    return (loadMesh(name));
}

template <>
std::unique_ptr<Material> AssetRegistry::loadAsset(const std::string &name)
{
    return (loadMaterial(name));
}

template <>
std::unique_ptr<Texture> AssetRegistry::loadAsset(const std::string &name)
{
    return (loadTexture(name));
}

//

std::vector<Material *> AssetResolver::resolveMaterials(AssetID<Mesh> meshHandle)
{
    std::vector<Material *> result;
    Mesh *mesh = registry.get(meshHandle);
    if (!mesh)
        return result;

    for (auto matHandle : mesh->materialIds)
    {
        Material *mat = registry.get(matHandle);
        if (mat)
            result.push_back(mat);
    }
    return result;
}

std::vector<Texture *> AssetResolver::resolveTextures(AssetID<Mesh> meshHandle)
{
    std::vector<Texture *> result;
    Mesh *mesh = registry.get(meshHandle);
    if (!mesh)
        return result;

    for (auto matHandle : mesh->materialIds)
    {
        Material *mat = registry.get(matHandle);
        if (!mat){continue;}
        if (mat->albedoMap.isValid())
        {
            result.push_back(registry.get(mat->albedoMap));
        };
        if (mat->normalMap.isValid())
        {
            result.push_back(registry.get(mat->normalMap));
        };
        if (mat->metallicMap.isValid())
        {
            result.push_back(registry.get(mat->metallicMap));
        };
        if (mat->roughnessMap.isValid())
        {
            result.push_back(registry.get(mat->roughnessMap));
        };
    }
    return result;
}
