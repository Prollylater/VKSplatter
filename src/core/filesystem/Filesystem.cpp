#include "Filesystem.h"
#include <fstream>

namespace {
    cico::filesystem::Paths paths;
}

namespace cico {

namespace filesystem {

void setRoot(const fs::path& root) {
    paths.root = fs::absolute(root);
}

const fs::path& root() {
    return paths.root;
}

void setShaders(const fs::path& shaders) {
    paths.shaders = fs::absolute(shaders);
}

const fs::path& shaders() {
    return paths.shaders;
}

void setTextures(const fs::path& textures) {
    paths.textures = fs::absolute(textures);
}

const fs::path& textures() {
    return paths.textures;
}

void setMeshes(const fs::path& meshes) {
    paths.meshes = fs::absolute(meshes);
}

const fs::path& meshes() {
    return paths.meshes;
}

}

namespace filesystem {

bool exists(const fs::path& path) {
    return fs::exists(path);
}

std::vector<uint8_t> readBinary(const fs::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
        throw std::runtime_error("Failed to open file: " + path.string());

    size_t size = file.tellg();
    std::vector<uint8_t> buffer(size);

    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

std::string readText(const fs::path& path) {
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("Failed to open file: " + path.string());

    return { std::istreambuf_iterator<char>(file),
             std::istreambuf_iterator<char>() };
}

bool writeBinary(const fs::path& path, const void* data, size_t size) {
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    file.write(reinterpret_cast<const char*>(data), size);
    return true;
}

bool writeText(const fs::path& path, const std::string& text) {
    std::ofstream file(path);
    if (!file) return false;
    file << text;
    return true;
}

};
};