#pragma once
#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;

// Todo: Read on why people don't like filesystem
namespace cico {

namespace filesystem
{

    struct Paths
    {
        std::filesystem::path root;
        std::filesystem::path shaders;
        std::filesystem::path textures;
        std::filesystem::path meshes;
    };

    void setRoot(const fs::path &root);
    const fs::path &root();
    void setShaders(const fs::path &root);
    const fs::path &shaders();
    void setTextures(const fs::path &root);
    const fs::path &textures();
    void setMeshes(const fs::path &root);
    const fs::path &meshes();

    bool exists(const fs::path &path);

    std::vector<uint8_t> readBinary(const fs::path &path);
    std::string readText(const fs::path &path);

    bool writeBinary(const fs::path &path, const void *data, size_t size);
    bool writeText(const fs::path &path, const std::string &text);

};
};
/*


*/