
#pragma once
#include "BaseVk.h"
#include <unordered_map>
#include <functional>

// Materials ?

// Not ideal

#include "Material.h"
#include "Mesh.h"
#include "Texture.h"

/*

class Assset {
std::string name;
}
Everything derivate from this

*/

class AssetRegistry
{
public:
    // Load or retrieve existing asset
    template <typename T>
    AssetID<T> add(const std::string &name, std::unique_ptr<T> asset)
    {
        auto &assetMap = getAssetMap<T>();

        for (auto &[id, record] : assetMap)
        { /*
           if (record.name == name)
           {
               ++record.refCount;
               return AssetID<T>(id);
           }*/
        }

        uint32_t id = nextID++;
        AssetRecord<T> record;
        // record.name = name;
        record.asset = std::move(asset);
        record.refCount = 1;

        assetMap[id] = std::move(record);
        return AssetID<T>(id);
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

    

    // Release asset reference
    // Todo: For texture they need to be destroyed
    template <typename T>
    void release(const AssetID<T> &handle)
    {
        auto &assetMap = getAssetMap<T>();
        auto it = assetMap.find(handle.getID());
        if (it != assetMap.end())
        {
            if (--it->second.refCount == 0)
            {
                assetMap.erase(it);
            }
        }
    }

private:
    template <typename T>
    struct AssetRecord
    {
        std::unique_ptr<T> asset;
        uint32_t refCount = 0;
        bool autoRelease = false;
        // RefCount is not really thorough either
        //  Todo, introduce it
        //  System made to remove data
        //  Read about other examples, what kind of data actually need it,
        //  If it need to concern GPU to ? etc..;
        //  Currently i really just want my asset stored in one class
        //  Potential hole the more we release
        //  NOthing prevent multiple Asset
    };
    // Todo: Look into replacing unodered_map by other contianers

    std::unordered_map<uint32_t, AssetRecord<Mesh>> meshes;
    // Todo: Design problem
    // Textures are bit treachrous has they actually load data in gpu
    std::unordered_map<uint32_t, AssetRecord<Texture>> textures;
    std::unordered_map<uint32_t, AssetRecord<Material>> materials;

    uint32_t nextID = 1;


    template <typename T>
    std::unordered_map<uint32_t, AssetRecord<T>> &getAssetMap();

    template <typename T>
    const std::unordered_map<uint32_t, AssetRecord<T>> &getAssetMap() const;

};

// Mesh being an object containing Material is not the best idea as we need to add a Mesh to then have it have separate material
//  Explicit template instantiation
// Todo: Move definition to cpp
// Todo:Const ref is not very much needed
template Mesh *AssetRegistry::get<Mesh>(const AssetID<Mesh> &) ;
template Texture *AssetRegistry::get<Texture>(const AssetID<Texture> &) ;
template Material *AssetRegistry::get<Material>(const AssetID<Material> &) ;
template Mesh *AssetRegistry::get<Mesh>(const AssetID<Mesh> &) const;
template Texture *AssetRegistry::get<Texture>(const AssetID<Texture> &) const ;
template Material *AssetRegistry::get<Material>(const AssetID<Material> &) const;


template void AssetRegistry::release<Mesh>(const AssetID<Mesh> &);
template void AssetRegistry::release<Texture>(const AssetID<Texture> &);
template void AssetRegistry::release<Material>(const AssetID<Material> &);

// Small helper to get data from loaded Mesh

struct AssetResolver
{
    AssetRegistry &registry;

    std::vector<Material *> resolveMaterials(AssetID<Mesh> meshHandle);
    std::vector<Texture *> resolveTextures(AssetID<Mesh> meshHandle);
};
