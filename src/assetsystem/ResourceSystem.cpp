#include "ResourceSystem.h"
#include <functional>
#include <tiny_obj_loader.h>

// Todo: Asset System should not be owning that logic or not in that way
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

    // Todo::Hasher might be kept in the Asset structure
    std::hash<std::string> hasher{};
    auto mesh = std::make_unique<Mesh>();
    mesh->hashedKey = hasher(filename);
    mesh->name = filename;

    mesh->positions.clear();
    mesh->normals.clear();
    mesh->uvs.clear();
    mesh->colors.clear();
    mesh->indices.clear();
    mesh->submeshes.clear();

    bool hasNormals = !attrib.normals.empty();
    bool hasTexcoords = !attrib.texcoords.empty();

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
            // Todo: TexturePath
            auto texture = std::make_unique<TextureCPU>(
                LoadImageTemplate<stbi_uc>(TEXTURE_PATH, STBI_rgb_alpha));
            texture->hashedKey = hasher(mtl.diffuse_texname);
            texture->name = mtl.diffuse_texname;
            AssetID textureID = mRegistry.add<TextureCPU>(std::move(texture));
            material->albedoMap = textureID;
            material->mConstants.albedoColor = glm::vec4{mtl.diffuse[0],
                                                         mtl.diffuse[1], mtl.diffuse[2], 1.0};
        }

        if (!mtl.bump_texname.empty())
        {
            auto texture = std::make_unique<TextureCPU>(
                LoadImageTemplate<stbi_uc>(TEXTURE_PATH, STBI_rgb_alpha));
            texture->hashedKey = hasher(mtl.normal_texname);
            texture->name = mtl.normal_texname;
            AssetID textureID = mRegistry.add<TextureCPU>(std::move(texture));
            material->normalMap = textureID;
        }
        if (!mtl.metallic_texname.empty())
        {
            auto texture = std::make_unique<TextureCPU>(
                LoadImageTemplate<stbi_uc>(TEXTURE_PATH, STBI_rgb_alpha));
            texture->hashedKey = hasher(mtl.metallic_texname);
            texture->name = mtl.metallic_texname;
            AssetID textureID = mRegistry.add<TextureCPU>(std::move(texture));
            material->metallicMap = textureID;
            material->mConstants.metallic = mtl.metallic;
        }

        if (!mtl.roughness_texname.empty())
        {
            auto texture = std::make_unique<TextureCPU>(
                LoadImageTemplate<stbi_uc>(TEXTURE_PATH, STBI_rgb_alpha));
            texture->hashedKey = hasher(mtl.roughness_texname);
            texture->name = mtl.roughness_texname;
            AssetID textureID = mRegistry.add<TextureCPU>(std::move(texture));
            material->roughnessMap = textureID;
            material->mConstants.roughness = mtl.roughness;
        }

        if (!mtl.emissive_texname.empty())
        {
            auto texture = std::make_unique<TextureCPU>(
                LoadImageTemplate<stbi_uc>(TEXTURE_PATH, STBI_rgb_alpha));
            texture->hashedKey = hasher(mtl.emissive_texname);
            texture->name = mtl.emissive_texname;
            AssetID textureID = mRegistry.add<TextureCPU>(std::move(texture));
            material->emissiveMap = textureID;
            material->mConstants.emissive = glm::vec4{mtl.emission[0],
                                                      mtl.emission[1], mtl.emission[2], 0.0};
        }

        material->mConstants.flags = material->computeFlags();
        AssetID materialID = mRegistry.add<Material>(std::move(material));
        mesh->materialIds.push_back(materialID);
    }

    std::unordered_map<VertexUnique, uint32_t, VertexHash> uniqueVertices;
    mesh->submeshes.reserve(shapes.size());
    uint32_t vertexOffset = 0;

    std::cout << "Shape *" << std::endl;
    for (const auto &shape : shapes)
    {
        std::unordered_map<int, std::vector<uint32_t>> facesPerMaterial;

        // Group same material face
        // Same material shape might not be semantically tied
        for (uint32_t face = 0; face < shape.mesh.material_ids.size(); ++face)
        {
            int matId = shape.mesh.material_ids[face];
            facesPerMaterial[matId].push_back(face);
        }

        for (const auto &[matId, faces] : facesPerMaterial)
        {
            Mesh::Submesh submesh;
            submesh.indexOffset = mesh->indices.size();
            submesh.materialId = (matId >= 0)
                                     ? static_cast<uint32_t>(matId)
                                     : INVALID_ASSET_ID;
                                     
            for (const uint32_t face : faces)
            {
                for (uint32_t v = 0; v < 3; ++v)
                {
                    const auto &idx = shape.mesh.indices[face * 3 + v];
                    glm::vec3 pos = {
                        attrib.vertices[3 * idx.vertex_index],
                        attrib.vertices[3 * idx.vertex_index + 1],
                        attrib.vertices[3 * idx.vertex_index + 2]};

                    glm::vec3 norm = glm::vec3(0.0f);
                    if (hasNormals && idx.normal_index >= 0)
                    {
                        norm = {
                            attrib.normals[3 * idx.normal_index],
                            attrib.normals[3 * idx.normal_index + 1],
                            attrib.normals[3 * idx.normal_index + 2]};
                    }

                    glm::vec2 uv = glm::vec2(0.0f);
                    if (hasTexcoords && idx.texcoord_index >= 0)
                    {
                        uv = {
                            attrib.texcoords[2 * idx.texcoord_index],
                            1.0f - attrib.texcoords[2 * idx.texcoord_index + 1]};
                    }

                    VertexUnique vertex = {pos, norm, uv};

                    if (uniqueVertices.count(vertex) == 0)
                    {
                        uniqueVertices[vertex] = static_cast<uint32_t>(mesh->positions.size());
                        mesh->positions.emplace_back(pos);
                        mesh->normals.emplace_back(norm);
                        mesh->uvs.emplace_back(uv);
                    }
                    mesh->indices.emplace_back(uniqueVertices[vertex]);
                }
            }

            submesh.indexCount =
                static_cast<uint32_t>(mesh->indices.size()) - submesh.indexOffset;
            submesh.vertexOffset = static_cast<uint32_t>(mesh->positions.size());

            mesh->submeshes.push_back(submesh);
        }
    }

    std::cout << "Model loaded successfully: " << filename << std::endl;
    std::cout << " - Total unique vertices: " << mesh->positions.size() << std::endl;
    std::cout << " - Total indices: " << mesh->indices.size() << std::endl;
    std::cout << " - Total shapes: " << shapes.size() << std::endl;
    std::cout << " - Total materials: " << materials.size() << std::endl;

    AssetID meshID = mRegistry.add<Mesh>(std::move(mesh));
    return meshID;
}

// Loads a mesh from file and returns an AssetID<Mesh>
/*
AssetID<Mesh> AssetSystem::loadMesh(const std::string &path)
{
    // Name based lookup called in registry once Mesh derivate from : asset
    auto meshPtr = mLoader.loadMeshFromFile(path);
    if (!meshPtr)
    {
        return AssetID<Mesh>{};
    }
    return mRegistry.add<Mesh>(std::move(meshPtr));
}

AssetID<TextureCPU> AssetSystem::loadTexture(const std::string &path)
{
    auto texPtr = mLoader.loadTextureFromFile(path);
    if (!texPtr)
    {
        return AssetID<TextureCPU>{};
    };
    return mRegistry.add<TextureCPU>(std::move(texPtr));
}

AssetID<Material> AssetSystem::loadMaterial(const std::string &path)
{
    auto matPtr = mLoader.loadMaterialFromFile(path);
    if (!matPtr)
    {
        return AssetID<Material>{};
    };
    return mRegistry.add<Material>(std::move(matPtr));
}
*/
// Provide direct access to registry and loader if needed
AssetRegistry &AssetSystem::registry() { return mRegistry; }

AssetIO &AssetSystem::loader() { return mLoader; }