#pragma once

#include "BaseVk.h"
#include <stb_image.h>
#include "Buffer.h"

template <typename T>
void FreeImage(T *data)
{
    stbi_image_free(data);
}

template <typename T>
struct ImageData
{
    T *data;
    uint32_t width;
    uint32_t height;
    int channels;

    void freeImage()
    {
        FreeImage<T>(data);
    };
};

struct TexturesRessources;

template <typename T>
ImageData<T> LoadImageTemplate(
    const std::string &filepath,
    int desired_channels = 0,
    bool flip_vertically = false,
    bool verbose = false);

class TextureManager
{
public:

  TextureManager() = default;
    ~TextureManager() = default;

    void createTextureImage(VkDevice deviceMdevice, VkPhysicalDevice physDevice,
                            const LogicalDeviceManager &deviceM, const CommandPoolManager &cmdPoolM,
                            const QueueFamilyIndices &indices);

    void createTextureImageView(VkDevice device);

    void createTextureSampler(VkDevice device);

    void createImage(VkDevice device, VkPhysicalDevice physDevice, const ImageData<stbi_uc> &imgData);

    void createTextureSampler(VkDevice device, VkPhysicalDevice physDevice);

    void destroyTexture(VkDevice device);

    void copyBufferToImage(VkBuffer buffer, VkImage image, const ImageData<stbi_uc> &imgData, const LogicalDeviceManager &deviceM, const CommandPoolManager &cmdPoolM,
                           const QueueFamilyIndices &indices);

    void destroyTextureView(VkDevice device);
    void destroySampler(VkDevice device);

    const VkImage getTexture() const
    {
        return mTextureImage;
    }

    const VkImageView getTextureView() const
    {
        return mTextureImageView;
    }

    const VkSampler getSampler() const
    {
        return mTextureSampler;
    }

private:
    // Maybe a texture class in addition to ImageData with much more metadata including format etc...
    //
    VkImage mTextureImage = VK_NULL_HANDLE;
    VkDeviceMemory mTextureImageMemory = VK_NULL_HANDLE;

    VkImageView mTextureImageView = VK_NULL_HANDLE;

    VkSampler mTextureSampler = VK_NULL_HANDLE;
};

namespace vkUtils
{

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, const LogicalDeviceManager &deviceM, const CommandPoolManager &cmdPoolM,
                               const QueueFamilyIndices &indices);

    VkImage createImage(
    VkDevice device,
    VkPhysicalDevice physDevice,
    VkDeviceMemory& imageMemory,
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkImageType imageType = VK_IMAGE_TYPE_2D,
    VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    uint32_t mipLevels = 1,
    uint32_t arrayLayers = 1,
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
    VkImageCreateFlags flags = 0
);

    VkImageView createImageView(
        VkDevice device,
        VkImage image,
        VkFormat format,
        VkImageAspectFlags aspectFlags,
        VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D,
        uint32_t baseMipLevel = 0,
        uint32_t levelCount = 1,
        uint32_t baseArrayLayer = 0,
        uint32_t layerCount = 1,
        VkComponentMapping components = {
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY});

}
// void createImageView(VkDevice device);
