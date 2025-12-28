#include "ResourceSystem.h"
#include <functional>

// Todo: Asset System should not be owning that logic
// FileSystem
AssetID<Mesh> AssetSystem::loadMeshWithMaterials(const std::string &filename)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), "./ressources/models/"))
    {
        throw std::runtime_error(warn + err);
    }

    std::hash<std::string> hasher{};
    auto mesh = std::make_unique<Mesh>();
    mesh->hashedKey = hasher(filename);
    mesh->name = filename;

    mesh->positions.clear();
    mesh->normals.clear();
    mesh->uvs.clear();
    mesh->colors.clear();
    mesh->indices.clear();

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
                uniqueVertices[vertex] = static_cast<uint32_t>(mesh->positions.size());
                mesh->positions.push_back(pos);
                mesh->normals.push_back(norm);
                mesh->uvs.push_back(uv);
            }

            mesh->indices.push_back(uniqueVertices[vertex]);
        }
    }

    for (const auto &mtl : materials)
    {
        auto material = std::make_unique<Material>();
        material->hashedKey = hasher(mtl.name);
        material->name = mtl.name;
        material->mType = MaterialType::PBR;

        // Material constants (colors, PBR factors)
        material->mConstants.albedoColor = glm::vec4(mtl.diffuse[0], mtl.diffuse[1], mtl.diffuse[2], 1.0f);
        material->mConstants.emissive = glm::vec4(mtl.emission[0], mtl.emission[1], mtl.emission[2], 1.0f);

        material->mConstants.metallic = mtl.metallic;
        material->mConstants.roughness = mtl.roughness;

        if (!mtl.diffuse_texname.empty())
        {
            auto texture = std::make_unique<TextureCPU>(mtl.diffuse_texname);
            texture->hashedKey = hasher(mtl.diffuse_texname);
            texture->name = mtl.diffuse_texname;
            AssetID textureID = mRegistry.add<TextureCPU>(mtl.diffuse_texname, std::move(texture));
            material->albedoMap = textureID;
        }

        if (!mtl.bump_texname.empty())
        {
            auto texture = std::make_unique<TextureCPU>(mtl.bump_texname);
            texture->hashedKey = hasher(mtl.normal_texname);
            texture->name = mtl.normal_texname;
            AssetID textureID = mRegistry.add<TextureCPU>(mtl.bump_texname, std::move(texture));
            material->normalMap = textureID;
        }
        if (!mtl.metallic_texname.empty())
        {
            auto texture = std::make_unique<TextureCPU>(mtl.metallic_texname);
            texture->hashedKey = hasher(mtl.metallic_texname);
            texture->name = mtl.metallic_texname;
            AssetID textureID = mRegistry.add<TextureCPU>(mtl.metallic_texname, std::move(texture));
            material->metallicMap = textureID;
        }

        if (!mtl.roughness_texname.empty())
        {
            auto texture = std::make_unique<TextureCPU>(mtl.roughness_texname);
            texture->hashedKey = hasher(mtl.roughness_texname);
            texture->name = mtl.roughness_texname;
            AssetID textureID = mRegistry.add<TextureCPU>(mtl.roughness_texname, std::move(texture));
            material->roughnessMap = textureID;
        }

        AssetID materialID = mRegistry.add<Material>(mtl.name, std::move(material));
        mesh->materialIds.push_back(materialID);
    }

    std::cout << "Model loaded successfully: " << filename << std::endl;
    std::cout << " - Total unique vertices: " << mesh->positions.size() << std::endl;
    std::cout << " - Total indices: " << mesh->indices.size() << std::endl;
    std::cout << " - Total shapes: " << shapes.size() << std::endl;
    std::cout << " - Total materials: " << materials.size() << std::endl;

    AssetID meshID = mRegistry.add<Mesh>(filename, std::move(mesh));
    return meshID;
}

// Loads a mesh from file and returns an AssetID<Mesh>
AssetID<Mesh> AssetSystem::loadMesh(const std::string &path)
{
    // Name based lookup called in registry once Mesh derivate from : asset
    auto meshPtr = mLoader.loadMeshFromFile(path);
    if (!meshPtr)
    {
        return AssetID<Mesh>{};
    }
    return mRegistry.add<Mesh>(path, std::move(meshPtr));
}

AssetID<TextureCPU> AssetSystem::loadTexture(const std::string &path)
{
    auto texPtr = mLoader.loadTextureFromFile(path);
    if (!texPtr)
    {
        return AssetID<TextureCPU>{};
    };
    return mRegistry.add<TextureCPU>(path, std::move(texPtr));
}

AssetID<Material> AssetSystem::loadMaterial(const std::string &path)
{
    auto matPtr = mLoader.loadMaterialFromFile(path);
    if (!matPtr)
    {
        return AssetID<Material>{};
    };
    return mRegistry.add<Material>(path, std::move(matPtr));
}

// Provide direct access to registry and loader if needed
AssetRegistry &AssetSystem::registry() { return mRegistry; }

AssetIO &AssetSystem::loader() { return mLoader; }