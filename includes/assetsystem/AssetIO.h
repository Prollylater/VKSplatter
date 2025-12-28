
#pragma once
#include "AssetTypes.h"
#include "filesystem/Filesystem.h"

//Todo:
#include "Material.h"
#include "Mesh.h"
#include "TextureC.h"

class AssetIO {
public:

    std::unique_ptr<AssetBase> loadAsset(AssetType type, const std::string &path);
    std::unique_ptr<Mesh>  loadMeshFromFile(const std::string &path);
    std::unique_ptr<TextureCPU> loadTextureFromFile(const std::string &path);
    std::unique_ptr<Material> loadMaterialFromFile(const std::string &path);

private:
    //std::vector<std::string> mSearchPaths;
};

