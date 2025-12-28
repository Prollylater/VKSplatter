
#pragma once
#include "BaseVk.h"
#include <unordered_map>
#include <functional>



// Materials ?

//Not ideal


#include "Material.h"
#include "Mesh.h"
#include "TextureC.h"
#include "filesystem/Filesystem.h"
//Todo: Revisit this both for filesystem and functions

class AssetIO {
public:

    std::unique_ptr<Mesh>  loadMeshFromFile(const std::string &path);
    std::unique_ptr<TextureCPU> loadTextureFromFile(const std::string &path);
    std::unique_ptr<Material> loadMaterialFromFile(const std::string &path);

    //Directly use filesystemp
    //void addSearchPath(const std::string &path) { mSearchPaths.push_back(path); }
private:
    //std::vector<std::string> mSearchPaths;
};

