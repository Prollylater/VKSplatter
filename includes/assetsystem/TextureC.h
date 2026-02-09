#pragma once
#include "stb_image.h"
#include "AssetTypes.h"
#include "iostream"

template <typename T>
void FreeImage(T *data)
{
    stbi_image_free(data);
}

template <typename T>
struct ImageData : AssetBase
{
    // static AssetType getStaticType() { return AssetType::Texture; }
    // AssetType type() const override { return getStaticType(); }
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

    static ImageData<stbi_uc> makeDummyImage(const stbi_uc pixel[4])
    {
        return ImageData<stbi_uc>{
            .data = const_cast<T *>(pixel),
            .width = 1,
            .height = 1,
            .channels = 4,
            .freeInGPU = false};
    }

    static ImageData<stbi_uc> getDummyAlbedoImage()
    {
        static stbi_uc white[4] = {255, 255, 255, 255};
        return makeDummyImage(white);
    }

    static ImageData<stbi_uc> getDummyNormalImage()
    {
        static stbi_uc flatNormal[4] = {128, 128, 255, 255};
        return makeDummyImage(flatNormal);
    }

    static ImageData<stbi_uc> getDummyRoughnessImage()
    {
        static stbi_uc white[4] = {255, 255, 255, 255};
        return makeDummyImage(white);
    }

    static ImageData<stbi_uc> getDummyMetallicImage()
    {
        static stbi_uc black[4] = {0, 0, 0, 255};
        return makeDummyImage(black);
    }
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
    // Aggregate name initialization
    if (!data)
    {
        std::cerr << "Failed to load image: " << filepath << "\nReason: " << stbi_failure_reason() << std::endl;
        return {INVALID_ASSET_ID, "", nullptr, 0, 0, 0};
    }

    if (verbose)
    {
        std::cout << "Loaded image: " << filepath << "\n";
        std::cout << "Size: " << width << "x" << height << "\n";
        std::cout << "Channels: " << (desired_channels ? desired_channels : channels) << "\n";
        std::cout << "Type: " << (std::is_same_v<T, float> ? "float" : "unsigned char") << "\n";
    }

    return {std::hash<std::string>{}(filepath), filepath, data, static_cast<uint32_t>(width), static_cast<uint32_t>(height), desired_channels ? desired_channels : channels};
}
using TextureCPU = ImageData<stbi_uc>;