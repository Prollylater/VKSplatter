
#pragma once
#include "BaseVk.h"
#include <unordered_map>
#include <functional>

// Materials ?

#include "Material.h"
#include "Mesh.h"
#include "TextureC.h"
//#include "Hashmap.h"
#include "logging/Logger.h"

template <typename T>
struct isAsset : std::is_base_of<AssetBase, T>
{
};
// Eventually
// concept AssetType = std::is_base_of_v<AssetBase, T>;
// Enforce it class wide ?

//This could actually be simple
//template<typename T>
//using AssetID = uint64_t;
//We would lose the explicit cosntructor and the direct hashing however

class AssetRegistry
{
public:
    // Load or retrieve existing asset
    template <typename T>
    AssetID<T> add(std::unique_ptr<T> asset) // requires AssetType<T>
    {
        static_assert(isAsset<T>::value, "T must derive from AssetBase");

        auto &assetMap = getAssetMap<T>();
        uint64_t key = asset->hashedKey;

        auto it = assetMap.find(key);
        if (it != assetMap.end())
        {
            //This might collide but it is unlikely in most realistic solution
            //Todo: Better solution ?
            //Test it or something
            //Rehash could be possible here since ID is for the better or worse

            if(it->second.asset->name != asset->name){
            _CERROR("Asset hash collision: ", 
                it->second.asset->name, 
                "and", asset->name, "at id", key);
            assert(it->second.asset->name == asset->name)};

            ++it->second.refCount;
            return AssetID<T>{key};
        }

        assetMap.emplace(key, AssetRecord<T>{
                                  .asset = std::move(asset),
                                  .refCount = 1});

        return AssetID<T>(key);
    }

    template <typename T>
    T *get(const AssetID<T> &handle)
    {
        auto &assetMap = getAssetMap<T>();
        auto it = assetMap.find(handle.getID());
        if (it != assetMap.end())
        {
            return it->second.asset.get();
        }
        return nullptr;
    }

    template <typename T>
    T *get(const AssetID<T> &handle) const
    {
        auto &assetMap = getAssetMap<T>();
        auto it = assetMap.find(handle.getID());
        if (it != assetMap.end())
        {
            return it->second.asset.get();
        }
        return nullptr;
    }

    template <typename T>
    void release(const AssetID<T> &handle)
    {
        auto &assetMap = getAssetMap<T>();
        auto it = assetMap.find(handle.getID());
        if (it != assetMap.end())
        {
            if (--it->second.refCount == 0)
            {
                //Notes: So far this won't destroy the content of those structures
                //Specifically, textures need to be freed manually due to lack of destructor
                if constexpr(std::is_same<T, TextureCPU>){
                    it->second.asset->freeImage();
                } 
                assetMap.erase(it);
            }
        }
    }

private:
    template <typename T>
    struct AssetRecord
    {
        std::unique_ptr<T> asset;
        // std::unique_ptr<AssetBase> asset;
        uint32_t refCount = 0;
    };

    // Todo: Replace unordered_map by RBHhashmap
    // Todo: Mutualize all containers to reduce template redundancy ?
    std::unordered_map<uint64_t, AssetRecord<Mesh>> meshes;
    // Todo: Design problem
    // Textures are bit treachrous has they actually load data in gpu
    std::unordered_map<uint64_t, AssetRecord<Texture>> textures;
    std::unordered_map<uint64_t, AssetRecord<Material>> materials;

    template <typename T>
    std::unordered_map<uint64_t, AssetRecord<T>> &getAssetMap();
    template <typename T>
    const std::unordered_map<uint64_t, AssetRecord<T>> &getAssetMap() const;
};

// Mesh being an object containing Material is not the best idea as we need to add a Mesh to then have it have separate material
// Explicit template instantiation

//Todo: Weird constnesss here is due to prototyping state

template Mesh *AssetRegistry::get<Mesh>(const AssetID<Mesh> &);
template TextureCPU *AssetRegistry::get<TextureCPU>(const AssetID<TextureCPU> &);
template Material *AssetRegistry::get<Material>(const AssetID<Material> &);

template Mesh *AssetRegistry::get<Mesh>(const AssetID<Mesh> &) const;
template TextureCPU *AssetRegistry::get<TextureCPU>(const AssetID<TextureCPU> &) const;
template Material *AssetRegistry::get<Material>(const AssetID<Material> &) const;

template void AssetRegistry::release<Mesh>(const AssetID<Mesh> &);
template void AssetRegistry::release<TextureCPU>(const AssetID<TextureCPU> &);
template void AssetRegistry::release<Material>(const AssetID<Material> &);

// Small helper to get data from loaded Mesh
struct AssetResolver
{
    AssetRegistry &registry;

    std::vector<Material *> resolveMaterials(AssetID<Mesh> meshHandle);
    std::vector<TextureCPU *> resolveTextures(AssetID<Mesh> meshHandle);
};
