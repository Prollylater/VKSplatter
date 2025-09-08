
#include "BaseVk.h"
#include "Texture.h"
#include "SwapChain.h"

// TODO: Rethink how command are passed

template <typename T>
ImageData<T> LoadImageTemplate(
    const std::string &filepath,
    int desired_channels,
    bool flip_vertically,
    bool verbose)
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
    return {data, width, height, desired_channels ? desired_channels : channels};
}

void Image::createImage(VkDevice device, VkPhysicalDevice physDevice, vkUtils::Texture::ImageCreateConfig &config)
{
    // Linear is better if we need to access ourself the texels
    mMipLevels = config.mipLevels;
    mFormat = config.format;
    mDescriptor.imageLayout = config.initialLayout;
    mExtent = {config.width, config.height, config.depth};
    mArrayLayers = config.arrayLayers;
    //  Necessary for blitting and thus for mimaping
    config.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    mImage = vkUtils::Texture::createImage(config);
    mImageMemory = config.imageMemory;
}

// Todo: Better handling of mImageMemory
//  TOdo:Classic sampled2D image rename to createImage2D and make it an utils
void Image::createImage(VkDevice device, VkPhysicalDevice physDevice, VkExtent3D extent, uint32_t mipLevels)
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
        .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

    config.imageType = (extent.height == 1 && extent.depth == 1) ? VK_IMAGE_TYPE_1D
                       : (extent.depth == 1)                     ? VK_IMAGE_TYPE_2D
                                                                 : VK_IMAGE_TYPE_3D;

    createImage(device, physDevice, config);
}

void Image::destroyImage(VkDevice device)
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
        vkDestroyImage(device, mImage, nullptr);
        vkFreeMemory(device, mImageMemory, nullptr);
        mImage = VK_NULL_HANDLE;
        mImageMemory = VK_NULL_HANDLE;
    }
}

// Todo: This helper should work with the already exiting value to infer the rest
void Image::createImageSampler(VkDevice device, VkPhysicalDevice physDevice)
{
     mDescriptor.sampler = vkUtils::Texture::createSampler(device, physDevice, mMipLevels);
}

// Todo: This helper should work with the already exiting value to infer the rest
void Image::createImageView(VkDevice device)
{
    // TOdo:Should also pass the Info or use a make config
    vkUtils::Texture::ImageViewCreateConfig config = {
        .device = device,
        .image = mImage,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = mMipLevels};
    mDescriptor.imageView = vkUtils::Texture::createImageView(config);
    // Change nothing but that'as an example of usage.
    //  I could set everything to red and rgba would be red
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
    mImage.createImageView(device);
};

// This should be removed
void Texture::createTextureSampler(VkDevice device, VkPhysicalDevice physDevice)
{
    mImage.createImageSampler(device, physDevice);
}

// Look into what is necessary for Texture and not
void Texture::createTextureImage(VkPhysicalDevice physDevice,
                                 const LogicalDeviceManager &deviceM,
                                 const QueueFamilyIndices &indice)
{
    ImageData<stbi_uc> textureData = LoadImageTemplate<stbi_uc>(TEXTURE_PATH.c_str(), STBI_rgb_alpha);
    VkDeviceSize imageSize = textureData.width * textureData.height * textureData.channels;

    // Todo:
    int mimaplevel = (true) ? 1 : static_cast<uint32_t>(std::floor(std::log2(std::max(textureData.width, textureData.height)))) + 1;

    if (!textureData.data)
    {
        throw std::runtime_error("No data in loaded Image");
    }

    VkDevice device = deviceM.getLogicalDevice();

    Buffer stagingBuffer;
    stagingBuffer.createBuffer(device, physDevice, imageSize,
                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    // Could we  just use the upload function ? CUrrenttly no as our upload also copy Buffer data
    vkUtils::BufferHelper::uploadBufferDirect(stagingBuffer.getMemory(), textureData.data, device, imageSize, 0);
    // void *data;
    // vkMapMemory(device, stagingBuffer.getMemory(), 0, imageSize, 0, &data);
    // memcpy(data, textureData.data, static_cast<size_t>(imageSize));
    // vkUnmapMemory(device, stagingBuffer.getMemory());

    textureData.freeImage();

    // Texture is freed here, what is actually used in creation are the dimensions
    mImage.createImage(device, physDevice, {textureData.width, textureData.height, 1}, mimaplevel);

    VkImage textureImg = mImage.getImage();
    // Todo: Find a better way to add this to the user option

    mImage.transitionImage(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,VK_IMAGE_ASPECT_COLOR_BIT, deviceM,  indice.graphicsFamily.value());
    vkUtils::Texture::copyBufferToImage(stagingBuffer.getBuffer(), textureImg, mImage.getExtent(), deviceM, indice);

    if (mImage.getMipLevels() == 1)
    {

         mImage.transitionImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,VK_IMAGE_ASPECT_COLOR_BIT, deviceM,  indice.graphicsFamily.value());
    }
    else
    {
        // Transition are done inside
        vkUtils::Texture::generateMipmaps(device, physDevice, indice, textureImg, deviceM.getGraphicsQueue(), VK_FORMAT_R8G8B8A8_SRGB,
                                          textureData.width, textureData.height, mImage.getMipLevels());
    }

    stagingBuffer.destroyBuffer();
}

void Texture::destroyTexture(VkDevice device)
{
    mImage.destroyImage(device);
}

// Creation utils

namespace vkUtils
{
    namespace Texture
    {
        VkImageView createImageView(ImageViewCreateConfig &config)
        {
            return createImageView(config.device, config.image,
                                   config.format, config.aspectFlags, config.viewType, config.baseMipLevel, config.levelCount,
                                   config.baseArrayLayer, config.layerCount, config.components);
        }

        VkImageView createImageView(
            VkDevice device,
            VkImage image,
            VkFormat format,
            VkImageAspectFlags aspectFlags,
            VkImageViewType viewType,
            uint32_t baseMipLevel,
            uint32_t levelCount,
            uint32_t baseArrayLayer,
            uint32_t layerCount,
            VkComponentMapping components)
        {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image;
            viewInfo.viewType = viewType;
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel = baseMipLevel;
            viewInfo.subresourceRange.levelCount = levelCount;
            viewInfo.subresourceRange.baseArrayLayer = baseArrayLayer;
            viewInfo.subresourceRange.layerCount = layerCount;
            viewInfo.components = components;

            VkImageView imageView;
            if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create image view!");
            }

            return imageView;
        }

        VkImage createImage(
            VkDevice device,
            VkPhysicalDevice physDevice,
            VkDeviceMemory &imageMemory,
            uint32_t width,
            uint32_t height,
            uint32_t depth,
            VkFormat format,
            VkImageTiling tiling,
            VkImageUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkImageType imageType,
            VkImageLayout initialLayout,
            VkSharingMode sharingMode,
            uint32_t mipLevels,
            uint32_t arrayLayers,
            VkSampleCountFlagBits samples,
            VkImageCreateFlags flags)
        {
            VkImage image;
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = imageType;
            imageInfo.extent.width = width;
            imageInfo.extent.height = height;
            imageInfo.extent.depth = depth;
            imageInfo.mipLevels = mipLevels;
            imageInfo.arrayLayers = arrayLayers;
            imageInfo.format = format;
            imageInfo.tiling = tiling;
            imageInfo.initialLayout = initialLayout;
            imageInfo.usage = usage;
            imageInfo.sharingMode = sharingMode;
            imageInfo.samples = samples;
            imageInfo.flags = flags;

            if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create image!");
            }

            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(device, image, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = findMemoryType(physDevice, memRequirements.memoryTypeBits, properties);

            if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to allocate image memory!");
            }

            if (vkBindImageMemory(device, image, imageMemory, 0) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to bind image memory!");
            }

            return image;
        }

        //DeviceMemory of an IMage must be initialized with a config that has be tbrough this function
        VkImage createImage(ImageCreateConfig &config)
        {

            return createImage(config.device, config.physDevice, config.imageMemory, config.width,
                               config.height, config.depth, config.format, config.tiling, config.usage, config.properties,
                               config.imageType, config.initialLayout, config.sharingMode, config.mipLevels, config.arrayLayers, config.samples, config.flags);
        }

        inline ImageTransition makeTransition(VkImage image,
                                              VkImageLayout oldLayout,
                                              VkImageLayout newLayout,
                                              VkImageAspectFlags aspectMask,
                                              uint32_t mipLevels,
                                              uint32_t layers)
        {
            ImageTransition transitionObject{};
            transitionObject.image = image;
            transitionObject.oldLayout = oldLayout;
            transitionObject.newLayout = newLayout;

            transitionObject.subresourceRange.aspectMask = aspectMask;

            transitionObject.subresourceRange.baseMipLevel = 0;
            transitionObject.subresourceRange.levelCount = mipLevels;
            transitionObject.subresourceRange.baseArrayLayer = 0;
            transitionObject.subresourceRange.layerCount = layers;

            // Infer sensible defaults for common transitions
            if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                transitionObject.srcAccessMask = 0;
                transitionObject.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                transitionObject.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                transitionObject.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                transitionObject.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                transitionObject.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                transitionObject.srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
                transitionObject.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
            {
                transitionObject.srcAccessMask = 0;
                transitionObject.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                                 VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                transitionObject.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                transitionObject.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            }
            else
            {
                // throw std::invalid_argument("unsupported layout transition!");
            }

            return transitionObject;
        }

        void transitionImageLayout(
            const ImageTransition &transitionObject,
            const LogicalDeviceManager &deviceM,
            uint32_t queueIndice)
        {

            const auto &graphicsQueue = deviceM.getGraphicsQueue();
            const auto &device = deviceM.getLogicalDevice();

            CommandPoolManager cmdPoolM;
            cmdPoolM.createCommandPool(device, CommandPoolType::Transient, queueIndice);
            VkCommandBuffer commandBuffer = cmdPoolM.beginSingleTime();
            recordImageMemoryBarrier(commandBuffer, transitionObject);

            cmdPoolM.endSingleTime(commandBuffer, graphicsQueue);
            cmdPoolM.destroyCommandPool();
        }

        void recordImageMemoryBarrier(VkCommandBuffer cmdBuffer,
                                      const ImageTransition &transitionObject)
        {

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = transitionObject.oldLayout;
            barrier.newLayout = transitionObject.newLayout;
            // Default choice for now
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = transitionObject.image;
            barrier.subresourceRange = transitionObject.subresourceRange;
            barrier.srcAccessMask = transitionObject.srcAccessMask;
            barrier.dstAccessMask = transitionObject.dstAccessMask;

            vkCmdPipelineBarrier(
                cmdBuffer,
                transitionObject.srcStageMask, transitionObject.dstStageMask,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier);
        }

        void generateMipmaps(VkDevice device, VkPhysicalDevice physicalDevice, const QueueFamilyIndices &indices, VkImage image, VkQueue graphicsQueue,
                             VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
        {

            // Check if image format supports linear blitting
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

            if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
            {
                throw std::runtime_error("texture image format does not support linear blitting!");
            }

            CommandPoolManager cmdPoolM;
            cmdPoolM.createCommandPool(device, CommandPoolType::Transient, indices.graphicsFamily.value());
            VkCommandBuffer commandBuffer = cmdPoolM.beginSingleTime();
            // Recording

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.image = image;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.levelCount = 1;

            int32_t mipWidth = texWidth;
            int32_t mipHeight = texHeight;

            for (uint32_t i = 1; i < mipLevels; i++)
            {
                barrier.subresourceRange.baseMipLevel = i - 1;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                vkCmdPipelineBarrier(commandBuffer,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                                     0, nullptr,
                                     0, nullptr,
                                     1, &barrier);

                VkImageBlit blit{};
                blit.srcOffsets[0] = {0, 0, 0};
                blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
                blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.srcSubresource.mipLevel = i - 1;
                blit.srcSubresource.baseArrayLayer = 0;
                blit.srcSubresource.layerCount = 1;
                blit.dstOffsets[0] = {0, 0, 0};
                blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
                blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.dstSubresource.mipLevel = i;
                blit.dstSubresource.baseArrayLayer = 0;
                blit.dstSubresource.layerCount = 1;

                vkCmdBlitImage(commandBuffer,
                               image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &blit,
                               VK_FILTER_LINEAR);

                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(commandBuffer,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                     0, nullptr,
                                     0, nullptr,
                                     1, &barrier);

                if (mipWidth > 1)
                    mipWidth /= 2;
                if (mipHeight > 1)
                    mipHeight /= 2;
            }

            barrier.subresourceRange.baseMipLevel = mipLevels - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                 0, nullptr,
                                 0, nullptr,
                                 1, &barrier);

            cmdPoolM.endSingleTime(commandBuffer, graphicsQueue);
            cmdPoolM.destroyCommandPool();
        }

        // Only copy an entire Buffer to a 2D Image
        void copyBufferToImage(VkBuffer buffer, VkImage image, VkExtent3D imgData, const LogicalDeviceManager &deviceM,
                               const QueueFamilyIndices &indices)
        {
            const auto &graphicsQueue = deviceM.getGraphicsQueue();
            const auto &device = deviceM.getLogicalDevice();

            CommandPoolManager cmdPoolM;
            cmdPoolM.createCommandPool(device, CommandPoolType::Transient, indices.graphicsFamily.value());
            VkCommandBuffer commandBuffer = cmdPoolM.beginSingleTime();

            // Recording
            // Single region
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = {0, 0, 0};
            region.imageExtent = {
                imgData.width,
                imgData.height,
                imgData.depth};

            vkCmdCopyBufferToImage(
                commandBuffer,
                buffer,
                image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // It is either this or General layout
                1,                                    // Number of regions
                &region);

            cmdPoolM.endSingleTime(commandBuffer, graphicsQueue);
            cmdPoolM.destroyCommandPool();
        }

        // Images from which we copy data must be created with a VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        // PLus the proper transition
        void copyImageToBuffer(VkImage image, VkBuffer buffer, VkExtent3D imgData, const LogicalDeviceManager &deviceM,
                               const QueueFamilyIndices &indices)
        {
            const auto &graphicsQueue = deviceM.getGraphicsQueue();
            const auto &device = deviceM.getLogicalDevice();

            CommandPoolManager cmdPoolM;
            cmdPoolM.createCommandPool(device, CommandPoolType::Transient, indices.graphicsFamily.value());
            VkCommandBuffer commandBuffer = cmdPoolM.beginSingleTime();

            // Recording
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = {0, 0, 0};
            region.imageExtent = {
                imgData.width,
                imgData.height,
                imgData.depth};

            vkCmdCopyImageToBuffer(
                commandBuffer,
                image,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                buffer,
                1,
                &region);

            cmdPoolM.endSingleTime(commandBuffer, graphicsQueue);
            cmdPoolM.destroyCommandPool();
        }

        // return rather than references ?
        VkSampler createSampler(VkDevice device, VkPhysicalDevice physDevice, int mipmaplevel)
        {
            VkSampler sampler;
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

            samplerInfo.anisotropyEnable = VK_TRUE;
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(physDevice, &properties);
            samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;

            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

            // MpipMapping stuff
            // PSeudo codehttps://vulkan-tutorial.com/Generating_Mipmaps
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = static_cast<float>(mipmaplevel);

            if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create texture sampler!");
            }
            return sampler;
        }
    }
}

/*

Something equivalent as helper?

makeImage2DCreateInfo : aids 2d image creation

makeImage3DCreateInfo : aids 3d descriptor set updating

makeImageCubeCreateInfo : aids cube descriptor set updating

makeImageViewCreateInfo : aids common image view creation, derives info from VkImageCreateInfo

//Soùe
cmdGenerateMipmaps : basic mipmap creation for images (meant for one-shot operations)

accessFlagsForImageLayout : helps resource transtions

pipelineStageForLayout : helps resource transitions

cmdBarrierImageLayout : inserts barrier for image transition
*/
