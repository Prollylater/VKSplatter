#pragma once
#include <filesystem>
#include <vector>
#include <string>

namespace stdfs = std::filesystem;

// Todo: Read on why people don't like std::filesystem
// Todo: Introduce some sort of fallback
namespace cico {

namespace fs
{

    struct Paths
    {
        std::filesystem::path root;
        std::filesystem::path shaders;
        std::filesystem::path textures;
        std::filesystem::path meshes;
    };

    void setRoot(const stdfs::path &root);
    const stdfs::path &root();
    void setShaders(const stdfs::path &root);
    const stdfs::path &shaders();
    void setTextures(const stdfs::path &root);
    const stdfs::path &textures();
    void setMeshes(const stdfs::path &root);
    const stdfs::path &meshes();

    bool exists(const stdfs::path &path);

    std::vector<uint8_t> readBinary(const stdfs::path &path);
    std::string readText(const stdfs::path &path);

    bool writeBinary(const stdfs::path &path, const void *data, size_t size);
    bool writeText(const stdfs::path &path, const std::string &text);

};
};
/*


*/