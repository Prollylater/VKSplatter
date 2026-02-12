#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

class LogicalDeviceManager;

namespace vkUtils
{
    namespace Texture
    {
        struct ImageCreateConfig;
        struct ImageViewCreateConfig;
    }
}

class Image
{
public:
    Image() {};
    ~Image() = default;

    // More specific version for quick mipmapped texture
    void createImage(VkDevice device, VkPhysicalDevice physDevice, VkExtent3D extent, uint32_t mipLevels, VmaAllocator alloc = VK_NULL_HANDLE);
    void createImage(vkUtils::Texture::ImageCreateConfig &config);

    // Prefer this function as their is no way to impact descriptor otherwise
    void transitionImage(VkImageLayout newlayout,
                         VkImageAspectFlags aspectMask,
                         const LogicalDeviceManager &deviceM,
                         uint32_t queueIndice);

     void createImageView( vkUtils::Texture::ImageViewCreateConfig & config);
    void createImageView(VkDevice device, VkImageAspectFlags aspectflag);
    void createImageSampler(VkDevice device, VkPhysicalDevice physDevice);

    void destroyImage(VkDevice device, VmaAllocator = VK_NULL_HANDLE);

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
    VmaAllocation mImageAlloc{};
    bool useVma = false;
    // TODO: The whole Vma should be rethought
    //  Attributes of the Image
    VkExtent3D mExtent = {1, 1, 1};
    VkFormat mFormat = VK_FORMAT_UNDEFINED;
    uint32_t mMipLevels = 1;
    VkSampleCountFlagBits mSamples = VK_SAMPLE_COUNT_1_BIT; // MSAA Not introduced yet
    uint32_t mArrayLayers = 1;

    // Todo:
    //  VkImageLayout mLayout; // Undecided and would

    VkDescriptorImageInfo mDescriptor{};
};
/*
  // descriptor.imageLayout represents the current imageLayout
  // descriptor.imageView may exist, created/destroyed by `nvvk::ResourceAllocator`
  // descriptor.sampler may exist, not managed by `nvvk::ResourceAllocator`
  VkDescriptorImageInfo descriptor{};

*/

// Semantic Helper to create Texture


#include "TextureC.h"
//Notes: Currently this is more an helper to texture 2Sthan anything
class Texture 
{
public:
    Texture() = default;
    ~Texture() = default;

    // Introduce parameter to decide mMipLevel and format etc..
    void createTextureImage(VkPhysicalDevice physDevice,
                            const LogicalDeviceManager &deviceM, const std::string &filepath,
                            uint32_t queuIndice, VmaAllocator alloc = VK_NULL_HANDLE);
    void createTextureImage(VkPhysicalDevice physDevice,
                            const LogicalDeviceManager &deviceM, ImageData<stbi_uc> &textureData,
                            uint32_t queuIndice, VmaAllocator allocator = VK_NULL_HANDLE);
    void createTextureImageView(VkDevice device);
    void createTextureSampler(VkDevice device, VkPhysicalDevice physDevice);

    void destroy(VkDevice device, VmaAllocator alloc = VK_NULL_HANDLE);

    const Image &getImage() const { return mImage; }
    VkImageView getView() const { return mImage.getView(); }
    VkSampler getSampler() const { return mImage.getSampler(); }

    static Texture *getDummyAlbedo(VkPhysicalDevice physDevice,
                                   const LogicalDeviceManager &deviceM,
                                   uint32_t queuIndice,
                                   VmaAllocator allocator);
    static Texture *getDummyNormal(VkPhysicalDevice physDevice,
                                   const LogicalDeviceManager &deviceM,
                                   uint32_t queuIndice,
                                   VmaAllocator allocator);
    static Texture *getDummyMetallic(VkPhysicalDevice physDevice,
                                     const LogicalDeviceManager &deviceM,
                                     uint32_t queuIndice,
                                     VmaAllocator allocator);
    static Texture *getDummyRoughness(VkPhysicalDevice physDevice,
                                      const LogicalDeviceManager &deviceM,
                                      uint32_t queuIndice,
                                      VmaAllocator allocator);

private:
    Image mImage;
    VkDevice mDevice = VK_NULL_HANDLE;
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