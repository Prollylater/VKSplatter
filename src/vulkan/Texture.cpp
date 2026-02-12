
#include "Texture.h"
#include "utils/RessourceHelper.h"
#include "Buffer.h"
#include "LogicalDevice.h"
#include "CommandPool.h"
#include <cmath>

void Image::createImage(vkUtils::Texture::ImageCreateConfig &config)
{
    // Linear is better if we need to access ourself the texels
    mMipLevels = config.mipLevels;
    mFormat = config.format;
    mDescriptor.imageLayout = config.initialLayout;
    mExtent = {config.width, config.height, config.depth};
    mArrayLayers = config.arrayLayers;

    mImage = vkUtils::Texture::createImage(config);
    if (config.imageMemory != VK_NULL_HANDLE)
    {
        mImageMemory = config.imageMemory;
    }
    else
    {
        mImageAlloc = config.allocation;
        useVma = true;
    }
}

//  TOdo:Classic sampled2D image rename to createImage2D and make it an utils
void Image::createImage(VkDevice device, VkPhysicalDevice physDevice, VkExtent3D extent, uint32_t mipLevels, VmaAllocator alloc)
{
    vkUtils::Texture::ImageCreateConfig config = {
        .device = device,
        .physDevice = physDevice,
        .imageMemory = mImageMemory,
        .width = extent.width,
        .height = extent.height,
        .depth = extent.depth,
        .mipLevels = mipLevels,
        .arrayLayers = 1,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT |
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .allocator = alloc};

    // Notes: Directly setting this ?
    config.imageType = (extent.width > 1 && extent.height == 1 && extent.depth == 1) ? VK_IMAGE_TYPE_1D
                       : (extent.depth == 1)                                         ? VK_IMAGE_TYPE_2D
                                                                                     : VK_IMAGE_TYPE_3D;

    createImage(config);
}

void Image::destroyImage(VkDevice device, VmaAllocator allocator)
{

    if (mDescriptor.sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(device, mDescriptor.sampler, nullptr);
        mDescriptor.sampler = VK_NULL_HANDLE;
    }

    if (mDescriptor.imageView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(device, mDescriptor.imageView, nullptr);
        mDescriptor.imageView = VK_NULL_HANDLE;
    }

    if (mImage != VK_NULL_HANDLE)
    {
        if (useVma)
        {
            vmaDestroyImage(allocator, mImage, mImageAlloc);
        }
        else
        {
            vkDestroyImage(device, mImage, nullptr);
            vkFreeMemory(device, mImageMemory, nullptr);
        }

        mImage = VK_NULL_HANDLE;
        mImageMemory = VK_NULL_HANDLE;
        mImageAlloc = VK_NULL_HANDLE;
    }
}

// Todo: This might need a different system
void Image::createImageSampler(VkDevice device, VkPhysicalDevice physDevice)
{
    mDescriptor.sampler = vkUtils::Texture::createSampler(device, physDevice, mMipLevels);
}

// Todo: This helper should work with the already exiting value to infer the rest
//Todo: Not very useful, better make a converter from imageConfig to imageView
void Image::createImageView(VkDevice device, VkImageAspectFlags aspectflag)
{
    // TOdo:Should also pass the Info or use a make config
    vkUtils::Texture::ImageViewCreateConfig config = {
        .device = device,
        .image = mImage,
        .format = mFormat,
        .aspectFlags = aspectflag,
        .levelCount = mMipLevels};
    mDescriptor.imageView = vkUtils::Texture::createImageView(config);
    // Change nothing but that'as an example of usage.
    //  I could set everything to red and rgba would be red
}

void Image::createImageView(vkUtils::Texture::ImageViewCreateConfig& config)
{
    mDescriptor.imageView = vkUtils::Texture::createImageView(config);
}
void Image::transitionImage(VkImageLayout newlayout,
                            VkImageAspectFlags aspectMask,
                            const LogicalDeviceManager &deviceM,
                            uint32_t queueIndice)
{
    vkUtils::Texture::ImageTransition transitionObject = vkUtils::Texture::makeTransition(mImage, mDescriptor.imageLayout,
                                                                                          newlayout, VK_IMAGE_ASPECT_COLOR_BIT,
                                                                                          mMipLevels, mArrayLayers);

    vkUtils::Texture::transitionImageLayout(transitionObject, deviceM, queueIndice);
    mDescriptor.imageLayout = newlayout;
}
// This should be removed
void Texture::createTextureImageView(VkDevice device)
{
    mImage.createImageView(device, VK_IMAGE_ASPECT_COLOR_BIT);
};

// This should be removed
void Texture::createTextureSampler(VkDevice device, VkPhysicalDevice physDevice)
{
    mImage.createImageSampler(device, physDevice);
}


void Texture::createTextureImage(VkPhysicalDevice physDevice,
                                 const LogicalDeviceManager &deviceM, const std::string &filepath,
                                 uint32_t queueIndice, VmaAllocator allocator)
{
    ImageData<stbi_uc> textureData = LoadImageTemplate<stbi_uc>(filepath.c_str(), STBI_rgb_alpha);
    createTextureImage(physDevice, deviceM, textureData, queueIndice, allocator);
    textureData.freeImage();
}

//Todo:
//Texture data should be different
void Texture::createTextureImage(VkPhysicalDevice physDevice,
                                 const LogicalDeviceManager &deviceM, ImageData<stbi_uc> &textureData,
                                 uint32_t queueIndice, VmaAllocator allocator)
{
    VkDeviceSize imageSize = textureData.width * textureData.height * textureData.channels /* * uchar */;

    // Todo:
    int mimaplevel = (true) ? 1 : static_cast<uint32_t>(std::floor(std::log2(std::max(textureData.width, textureData.height)))) + 1;

    if (!textureData.data)
    {
        throw std::runtime_error("No data in loaded Image");
    }

    VkDevice device = deviceM.getLogicalDevice();

    // Todo: Change order of call of the creation function
    Buffer stagingBuffer;

    stagingBuffer.createBuffer(device, physDevice, imageSize,
                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, allocator);
    // Could we  just use the upload function ? CUrrenttly no as our upload also copy Buffer data
    if (allocator)
    {
        vkUtils::BufferHelper::uploadBufferVMA(allocator, stagingBuffer.getVMAMemory(), textureData.data, imageSize, 0);
    }
    else
    {
        vkUtils::BufferHelper::uploadBufferDirect(device, stagingBuffer.getMemory(), textureData.data, imageSize, 0);
    }

    // Texture is freed here, what is actually used in creation are the dimensions
    mImage.createImage(device, physDevice, {textureData.width, textureData.height, 1}, mimaplevel, allocator);

    VkImage textureImg = mImage.getImage();
    // Todo: Find a better way to add this to the user option

    mImage.transitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, deviceM, queueIndice);

    /////////////////
    // Only copy an entire Buffer to a 2D Image
    const auto &graphicsQueue = deviceM.getGraphicsQueue();

    CommandPoolManager cmdPoolM;
    cmdPoolM.createCommandPool(device, CommandPoolType::Transient, queueIndice);
    VkCommandBuffer commandBuffer = cmdPoolM.beginSingleTime();
    vkUtils::Texture::copyBufferToImage(commandBuffer, stagingBuffer.getBuffer(), textureImg, mImage.getExtent());

    cmdPoolM.endSingleTime(commandBuffer, graphicsQueue);
    cmdPoolM.destroyCommandPool();
    /////////////////

    if (mImage.getMipLevels() == 1)
    {

        mImage.transitionImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, deviceM, queueIndice);
    }
    else
    {
        // Transition are done inside
        vkUtils::Texture::generateMipmaps(device, physDevice, queueIndice, textureImg, deviceM.getGraphicsQueue(), VK_FORMAT_R8G8B8A8_SRGB,
                                          textureData.width, textureData.height, mImage.getMipLevels());
    }

    stagingBuffer.destroyBuffer(device, allocator);
}

void Texture::destroy(VkDevice device, VmaAllocator alloc)
{
    mImage.destroyImage(device, alloc);
}

//Todo: Dummy are not added to the map and are thus generally unreleased
Texture *Texture::getDummyAlbedo(VkPhysicalDevice physDevice,
                                 const LogicalDeviceManager &deviceM,
                                 uint32_t queueIndice,
                                 VmaAllocator allocator)
{
    static Texture dummyAlbedo;
    static bool created = false;

    if (!created)
    {
        stbi_uc whitePixel[4] = {255, 255, 255, 255};

        ImageData<stbi_uc> img{.data =  whitePixel, .width= 1, .height=  1, .channels = 4};

        dummyAlbedo.createTextureImage(physDevice, deviceM, img, queueIndice, allocator);
        dummyAlbedo.createTextureImageView(deviceM.getLogicalDevice());
        dummyAlbedo.createTextureSampler(deviceM.getLogicalDevice(), physDevice);

        created = true;
    }

    return &dummyAlbedo;
}

Texture *Texture::getDummyNormal(VkPhysicalDevice physDevice,
                                 const LogicalDeviceManager &deviceM,
                                 uint32_t queueIndices,
                                 VmaAllocator allocator)
{
    static Texture dummyNormal;
    static bool created = false;
    if (!created)
    {

        // This translate to a flat normal (0.5, 0.5, 1.0)
        stbi_uc flatNormal[4] = {128, 128, 255, 255};

        ImageData<stbi_uc> img{.data =  flatNormal, .width= 1, .height=  1, .channels = 4};
        dummyNormal.createTextureImage(physDevice, deviceM, img, queueIndices, allocator);
        dummyNormal.createTextureImageView(deviceM.getLogicalDevice());
        dummyNormal.createTextureSampler(deviceM.getLogicalDevice(), physDevice);

        created = true;
    }

    return &dummyNormal;
}

Texture *Texture::getDummyRoughness(VkPhysicalDevice physDevice,
                                    const LogicalDeviceManager &deviceM,
                                    uint32_t queueIndices,
                                    VmaAllocator allocator)
{
    static Texture dummyRough;
    static bool created = false;

    if (!created)
    {
        stbi_uc whitePixel[4] = {255, 255, 255, 255};
        ImageData<stbi_uc> img{.data =  whitePixel, .width= 1, .height=  1, .channels = 4};
        
        dummyRough.createTextureImage(physDevice, deviceM, img, queueIndices, allocator);
        dummyRough.createTextureImageView(deviceM.getLogicalDevice());
        dummyRough.createTextureSampler(deviceM.getLogicalDevice(), physDevice);
        created = true;
    }

    return &dummyRough;
}

Texture *Texture::getDummyMetallic(VkPhysicalDevice physDevice,
                                   const LogicalDeviceManager &deviceM,
                                   uint32_t queueIndices,
                                   VmaAllocator allocator)
{
    static Texture dummyMetallic;
    static bool created = false;

    if (!created)
    {
        stbi_uc blackPixel[4] = {0, 0, 0, 255}; // dielectric
        ImageData<stbi_uc> img{.data =  blackPixel, .width= 1, .height=  1, .channels = 4};

        dummyMetallic.createTextureImage(physDevice, deviceM, img, queueIndices, allocator);
        dummyMetallic.createTextureImageView(deviceM.getLogicalDevice());
        dummyMetallic.createTextureSampler(deviceM.getLogicalDevice(), physDevice);
        created = true;
    }

    return &dummyMetallic;
}
