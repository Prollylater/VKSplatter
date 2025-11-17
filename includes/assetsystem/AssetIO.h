
#pragma once
#include "BaseVk.h"
#include <unordered_map>
#include <functional>



// Materials ?

//Not ideal


#include "Material.h"
#include "Mesh.h"
#include "Texture.h"


class AssetIO {
public:
    AssetIO() = default;

    std::unique_ptr<Mesh>  loadMeshFromFile(const std::string &path);
    std::unique_ptr<Texture> loadTextureFromFile(const std::string &path);
    std::unique_ptr<Material> loadMaterialFromFile(const std::string &path);

    //void addSearchPath(const std::string &path) { mSearchPaths.push_back(path); }
private:
    //std::vector<std::string> mSearchPaths;
};


#include <tiny_obj_loader.h>


Material loadMaterial(AssetIO &assets, const tinyobj::material_t &objMaterial);

// Todo: Decoyple Asset Registry from t
// Ex: Multi steps with first step sending back Mesh + Material names
Mesh loadMesh(AssetIO &assets, std::string filename);


