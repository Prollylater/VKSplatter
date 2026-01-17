#include "Filesystem.h"
#include <fstream>

namespace {
    cico::fs::Paths paths;
}

namespace cico {

namespace fs {

void setRoot(const stdfs::path& root) {
    paths.root = stdfs::absolute(root);
}

const stdfs::path& root() {
    return paths.root;
}

void setShaders(const stdfs::path& shaders) {
    paths.shaders = stdfs::absolute(shaders);
}

const stdfs::path& shaders() {
    return paths.shaders;
}

void setTextures(const stdfs::path& textures) {
    paths.textures = stdfs::absolute(textures);
}

const stdfs::path& textures() {
    return paths.textures;
}

void setMeshes(const stdfs::path& meshes) {
    paths.meshes = stdfs::absolute(meshes);
}

const stdfs::path& meshes() {
    return paths.meshes;
}

}

namespace filesystem {

bool exists(const stdfs::path& path) {
    return stdfs::exists(path);
}

std::vector<uint8_t> readBinary(const stdfs::path& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
        throw std::runtime_error("Failed to open file: " + path.string());

    size_t size = file.tellg();
    std::vector<uint8_t> buffer(size);

    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

std::string readText(const stdfs::path& path) {
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("Failed to open file: " + path.string());

    return { std::istreambuf_iterator<char>(file),
             std::istreambuf_iterator<char>() };
}

bool writeBinary(const stdfs::path& path, const void* data, size_t size) {
    std::ofstream file(path, std::ios::binary);
    if (!file) return false;
    file.write(reinterpret_cast<const char*>(data), size);
    return true;
}

bool writeText(const stdfs::path& path, const std::string& text) {
    std::ofstream file(path);
    if (!file) return false;
    file << text;
    return true;
}

};
};