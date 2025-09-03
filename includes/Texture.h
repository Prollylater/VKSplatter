#pragma once

#include "BaseVk.h"
#include "stb_image.h"
#include "Buffer.h"

// Shared Sampler concept ?
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

template <typename T>
ImageData<T> LoadImageTemplate(
    const std::string &filepath,
    int desired_channels = 0,
    bool flip_vertically = false,
    bool verbose = false);

// Utility related to textures
namespace vkUtils
{
    namespace Texture
    {
        // Second namespace here
        // Too big need to rework
        void generateMipmaps(VkDevice device, VkPhysicalDevice physicalDevice, const QueueFamilyIndices &indices, VkImage image, VkQueue graphicsQueue,
                             VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

        struct ImageTransition
        {
            VkImage image = VK_NULL_HANDLE;
            VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkImageLayout newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkImageSubresourceRange subresourceRange{};
            VkAccessFlags srcAccessMask = 0;
            VkAccessFlags dstAccessMask = 0;
            VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        };

        ImageTransition makeTransition(VkImage image,
                                       VkImageLayout oldLayout,
                                       VkImageLayout newLayout,
                                       VkImageAspectFlags aspectMask,
                                       uint32_t mipLevels = VK_REMAINING_MIP_LEVELS,
                                       uint32_t layers = VK_REMAINING_ARRAY_LAYERS);

        void transitionImageLayout(
            const ImageTransition &transitionObject,
            const LogicalDeviceManager &deviceM,
            uint32_t queueIndice);

        void recordImageMemoryBarrier(VkCommandBuffer cmdBuffer,
                                      const ImageTransition &transitionObject);

        struct ImageCreateConfig
        {
            VkDevice device = VK_NULL_HANDLE;
            VkPhysicalDevice physDevice = VK_NULL_HANDLE;
            VkDeviceMemory imageMemory = VK_NULL_HANDLE;
            uint32_t width = 1;
            uint32_t height = 1;
            uint32_t depth = 1;
            uint32_t mipLevels = 1;
            uint32_t arrayLayers = 1;
            VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
            VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
            VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
            VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            VkImageType imageType = VK_IMAGE_TYPE_2D;
            VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
            VkImageCreateFlags flags = 0;
        };

        VkImage createImage(
            VkDevice device,
            VkPhysicalDevice physDevice,
            VkDeviceMemory &imageMemory,
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
            VkImageCreateFlags flags = 0);

        VkImage createImage(ImageCreateConfig &);

        struct ImageViewCreateConfig
        {
            VkDevice device = VK_NULL_HANDLE;
            VkImage image = VK_NULL_HANDLE;
            VkFormat format = VK_FORMAT_UNDEFINED;
            VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
            VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
            uint32_t baseMipLevel = 0;
            uint32_t levelCount = VK_REMAINING_MIP_LEVELS;
            uint32_t baseArrayLayer = 0;
            uint32_t layerCount = VK_REMAINING_ARRAY_LAYERS;
            VkComponentMapping components = {
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY,
                VK_COMPONENT_SWIZZLE_IDENTITY};
        };

        VkImageView createImageView(ImageViewCreateConfig &);

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

        // void record
        void copyBufferToImage(VkBuffer buffer, VkImage image, VkExtent3D imgData, const LogicalDeviceManager &deviceM,
                               const QueueFamilyIndices &indices);
        void copyImageToBuffer(VkImage image, VkBuffer buffer, const LogicalDeviceManager &deviceM,
                               const QueueFamilyIndices &indices);

        VkSampler createSampler(VkDevice device, VkPhysicalDevice physDevice, int mipmaplevel);

    }
}

class Image
{
public:
    Image() = default;
    ~Image() = default;

    void createImage(VkDevice device, VkPhysicalDevice physDevice, VkExtent3D extent, uint32_t mipLevels);
    void createImage(VkDevice device, VkPhysicalDevice physDevice, vkUtils::Texture::ImageCreateConfig &config);

    // Prefer this function as their is no way to impact descriptor otherwise
    void transitionImage(VkImageLayout newlayout,
                         VkImageAspectFlags aspectMask,
                         const LogicalDeviceManager &deviceM,
                         uint32_t queueIndice);

    void createImageView(VkDevice device);
    void createImageSampler(VkDevice device, VkPhysicalDevice physDevice);

    void destroyImage(VkDevice device);

    // Getters
    VkImage getImage() const { return mImage; }
    VkDeviceMemory getMemory() const { return mImageMemory; }
    VkExtent3D getExtent() const { return mExtent; }
    VkFormat getFormat() const { return mFormat; }
    uint32_t getMipLevels() const { return mMipLevels; }
    uint32_t getArrayLayers() const { return mArrayLayers; }
    VkImageLayout getLayout() const { return mDescriptor.imageLayout; }
    VkImageView getView() const { return mDescriptor.imageView; }
    VkSampler getSampler() const { return mDescriptor.sampler; }
    VkDescriptorImageInfo getDescriptor() const { return mDescriptor; }

private:
    VkImage mImage = VK_NULL_HANDLE;
    VkDeviceMemory mImageMemory = VK_NULL_HANDLE;
    // VkImageView mImageView = VK_NULL_HANDLE;

    // Attributes of the Image
    VkExtent3D mExtent = {1, 1, 1};
    VkFormat mFormat = VK_FORMAT_UNDEFINED;
    uint32_t mMipLevels = 1;
    VkSampleCountFlagBits mSamples = VK_SAMPLE_COUNT_1_BIT; // MSAA Not introduced yet
    uint32_t mArrayLayers = 1;

    // VkImageLayout mLayout; // Undecided

    VkDescriptorImageInfo mDescriptor{};
};
/*
  // descriptor.imageLayout represents the current imageLayout
  // descriptor.imageView may exist, created/destroyed by `nvvk::ResourceAllocator`
  // descriptor.sampler may exist, not managed by `nvvk::ResourceAllocator`
  VkDescriptorImageInfo descriptor{};

*/

// Semantic Helper
class Texture
{
public:
    Texture() = default;
    ~Texture() = default;

    // Introduce parameter to decide mMipLevel and format etc..
    void createTextureImage(VkPhysicalDevice physDevice,
                            const LogicalDeviceManager &deviceM,
                            const QueueFamilyIndices &indices);

    void createTextureImageView(VkDevice device);
    void createTextureSampler(VkDevice device, VkPhysicalDevice physDevice);

    void destroyTexture(VkDevice device);

    const Image &getImage() const { return mImage; }
    VkImageView getView() const { return mImage.getView(); }
    VkSampler getSampler() const { return mImage.getSampler(); }

private:
    Image mImage;
    VkDevice mDevice = VK_NULL_HANDLE;
};

class RenderTargets
{
private:
    std::vector<Image> gBufferColor{};
    Image gBufferDepth{};
    VkExtent2D m_size{}; // Width and height of the buffers
};

// Creating Descriptor Sets Chapter
/*
Since VkFrameBuffer are deprecated we shoudl moveto this
Take the physical device on which operations are performed and store its handle in a variable of type VkPhysicalDevice named physical_device.
Select a format for an image and use it to initialize a variable of type VkFormat named format.
Create a variable of type VkFormatProperties named format_properties.
Call vkGetPhysicalDeviceFormatProperties( physical_device, format, &format_properties )
If the image's color data will be read, make sure the selected format is suitable for such usage. For this, check whether a VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT bit is set in an optimalTilingFeatures member of the format_properties variable.
If the image's depth or stencil data will be read, check whether the selected format can be used for reading depth or stencil data. Do that by making sure that a VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT bit is set in an optimalTilingFeatures member of the format_properties variable.
Create an image using the logical_device and format variables, and select appropriate values for the rest of the image's parameters.
 Make sure the VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT usage is specified during the image creation. Store the created handle in a variable of type VkImage named input_attachment (refer to the Creating an image recipe from Chapter 4, Resources and Memory).
Allocate a memory object with a VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT property (or use a range of an existing memory object)
Create an image view using the logical_device, input_attachment, and format variables, and choose the rest of the image view's parameters.



e VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT bit of an optimalTilingFeatures member of the format_properties variable is set.
Do that by checking whether the VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT bit of an optimalTilingFeatures member of the format_properties variable is set.
*/