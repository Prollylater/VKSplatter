// ResourceSystem.h
#pragma once
#include "AssetRegistry.h"
#include "AssetIO.h"


class AssetSystem
{
public:
    AssetSystem() = default;

    AssetID<Mesh> loadMeshWithMaterials(const std::string &filename);

    // Loads a mesh from file and returns an AssetID<Mesh>
    //Todo: Currently defunct
    AssetID<Mesh> loadMesh(const std::string &path) = delete;
    AssetID<TextureCPU> loadTexture(const std::string &path) = delete;
    AssetID<Material> loadMaterial(const std::string &path) = delete;
    
    // Provide direct access to registry and loader if needed
    AssetRegistry &registry();

    AssetIO &loader(); 

private:
    AssetRegistry mRegistry; 
    AssetIO mLoader;
};



