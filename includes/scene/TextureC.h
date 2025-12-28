#pragma once

#include "BaseVk.h"
#include "stb_image.h"
#include "Buffer.h"

template <typename T>
void FreeImage(T *data)
{
    stbi_image_free(data);
}

template <typename T>
struct ImageData : AssetBase
{
    T *data;
    uint32_t width;
    uint32_t height;
    int channels;

    // Todo: Temporary
    bool freeInGPU = false;

    void freeImage()
    {
        if (freeInGPU)
        {
            FreeImage<T>(data);
        }
    };
};

template <typename T>
ImageData<T> LoadImageTemplate(
    const std::string &filepath,
    int desired_channels = 0,
    bool flip_vertically = false,
    bool verbose = false)
{
    static_assert(std::is_same_v<T, unsigned char> || std::is_same_v<T, float>,
                  "Unsupported type");

    stbi_set_flip_vertically_on_load(flip_vertically);

    int width, height, channels;
    T *data = nullptr;

    if constexpr (std::is_same_v<T, float>)
    {
        data = stbi_loadf(filepath.c_str(), &width, &height, &channels, desired_channels);
    }
    else
    {
        data = stbi_load(filepath.c_str(), &width, &height, &channels, desired_channels);
    }

    if (!data)
    {
        std::cerr << "Failed to load image: " << filepath << "\nReason: " << stbi_failure_reason() << std::endl;
        return {nullptr, 0, 0, 0};
    }

    if (verbose)
    {
        std::cout << "Loaded image: " << filepath << "\n";
        std::cout << "Size: " << width << "x" << height << "\n";
        std::cout << "Channels: " << (desired_channels ? desired_channels : channels) << "\n";
        std::cout << "Type: " << (std::is_same_v<T, float> ? "float" : "unsigned char") << "\n";
    }

    // Warining here
    return {std::hash<std::string>{}(filepath), filepath, data, width, height, desired_channels ? desired_channels : channels};
}
using TextureCPU = ImageData<stbi_uc>;