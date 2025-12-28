
#pragma once
#include <string>
#include <cstdint>

#define INVALID_ASSET_ID 0
enum class AssetType { Mesh, Texture, Material };


struct AssetBase {
    uint64_t hashedKey;  
    std::string name;    
    //virtual AssetType type() const = 0;
    //virtual ~AssetBase() = default;
};


template <typename T>
struct AssetID
{
    uint64_t id = INVALID_ASSET_ID; 

    AssetID() = default;
    explicit AssetID(uint64_t _id) : id(_id) {};
    //explicit AssetID(std::string name) : id(std::hash<std::string>{}()) {};

    uint64_t getID() const { return id; }
    bool isValid() const { return id != INVALID_ASSET_ID; }

    bool operator==(const AssetID &other) const { return id == other.id; }
    bool operator!=(const AssetID &other) const { return id != other.id; }
};

