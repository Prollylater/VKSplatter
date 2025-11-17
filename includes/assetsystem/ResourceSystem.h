// ResourceSystem.h
#pragma once
#include "AssetRegistry.h"
#include "AssetIO.h"
#include <memory>
#include <string>

//Todo: Not all asset are equal from the loading stae
//Currently texture are not loaded CPU and GPU side but should
//Mesh are loaded CPU. I am not sure how i want to handle texture yet

class AssetSystem
{
public:
    AssetSystem() = default;

    AssetID<Mesh> loadMeshWithMaterials(const std::string &filename);

    // Loads a mesh from file and returns an AssetID<Mesh>
    AssetID<Mesh> loadMesh(const std::string &path);
    AssetID<Texture> loadTexture(const std::string &path);

    AssetID<Material> loadMaterial(const std::string &path);
    // Provide direct access to registry and loader if needed
    AssetRegistry &registry();

    AssetIO &loader(); 

private:
    AssetRegistry mRegistry; // reference to the central registry (owned by Application)
    AssetIO mLoader;
};
