#include "AssetRegistry.h"

template <>
std::unordered_map<uint64_t, AssetRegistry::AssetRecord<Mesh>> &AssetRegistry::getAssetMap<Mesh>()
{
    return meshes;
}

template <>
std::unordered_map<uint64_t, AssetRegistry::AssetRecord<Texture>> &AssetRegistry::getAssetMap<Texture>()
{
    return textures;
}

template <>
std::unordered_map<uint64_t, AssetRegistry::AssetRecord<Material>> &AssetRegistry::getAssetMap<Material>()
{
    return materials;
}

template <>
const std::unordered_map<uint64_t, AssetRegistry::AssetRecord<Mesh>> &AssetRegistry::getAssetMap<Mesh>() const
{
    return meshes;
}

template <>
const std::unordered_map<uint64_t, AssetRegistry::AssetRecord<Texture>> &AssetRegistry::getAssetMap<Texture>() const
{
    return textures;
}

template <>
const std::unordered_map<uint64_t, AssetRegistry::AssetRecord<Material>> &AssetRegistry::getAssetMap<Material>() const
{
    return materials;
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

std::vector<TextureCPU *> AssetResolver::resolveTextures(AssetID<Mesh> meshHandle)
{
    std::vector<TextureCPU *> result;
    Mesh *mesh = registry.get(meshHandle);
    if (!mesh)
        return result;

    for (auto matHandle : mesh->materialIds)
    {
        Material *mat = registry.get(matHandle);
        if (!mat)
        {
            continue;
        }
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
