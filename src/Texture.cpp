
#include "BaseVk.h"
#include "Texture.h"
#include "SwapChain.h"

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

void TextureManager::createTextureImage( VkPhysicalDevice physDevice,
                                        const LogicalDeviceManager &deviceM, const CommandPoolManager &cmdPoolM,
                                        const QueueFamilyIndices &indices)
{
    // Todo: This implictly move ?
    ImageData<stbi_uc> texture = LoadImageTemplate<stbi_uc>(TEXTURE_PATH.c_str(), STBI_rgb_alpha);
    VkDeviceSize imageSize = texture.width * texture.height * texture.channels;

    mMipMapLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texture.width, texture.height)))) + 1;
    if (!texture.data)
    {
        throw std::runtime_error("");
    }

    VkDevice device = deviceM.getLogicalDevice();
    // Transfer the image data
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(stagingBuffer, stagingBufferMemory, device, physDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    void *data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, texture.data, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);
    texture.freeImage();
    ///
    createImage(device, physDevice, texture);

    // TODO: BEtter and easier way to implement transaiton
    vkUtils::transitionImageLayout(mTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, deviceM, cmdPoolM, indices, mMipMapLevels);
    copyBufferToImage(stagingBuffer, mTextureImage, texture, deviceM, cmdPoolM, indices);
    if(mMipMapLevels == 1){
    vkUtils::transitionImageLayout(mTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, deviceM, cmdPoolM, indices , mMipMapLevels);
    } else {
    const auto &commandPool = cmdPoolM.createSubCmdPool(device, indices, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    vkUtils::generateMipmaps(device, physDevice, commandPool, mTextureImage, deviceM.getGraphicsQueue(),VK_FORMAT_R8G8B8A8_SRGB ,
    texture.width,texture.height, mMipMapLevels);
    }
    
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void TextureManager::createImage(VkDevice device, VkPhysicalDevice physDevice, const ImageData<stbi_uc> &imgData)
{
    // Todo; stuff
    // Chuck into a function
    // COuld need to check the support for this
    // imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    // Linear is better if we need to access ourseulves the texels
    // imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

    // Could also simply pass the Image Info
    vkUtils::ImageCreateConfig config = {
        .device = device,
        .physDevice = physDevice,
        .imageMemory = mTextureImageMemory,
        .width = imgData.width,
        .height = imgData.height,
        .mipLevels = mMipMapLevels,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        // Only override what's needed
    };
    //Necessary for blitting and thus for mimaping
    config.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    mTextureImage = vkUtils::createImage(config);
    mTextureImageMemory = config.imageMemory;

}

void TextureManager::destroyTexture(VkDevice device)
{
    vkDestroyImage(device, mTextureImage, nullptr);
    vkFreeMemory(device, mTextureImageMemory, nullptr);
}

void TextureManager::destroyTextureView(VkDevice device)
{
    vkDestroyImageView(device, mTextureImageView, nullptr);
}

void TextureManager::copyBufferToImage(VkBuffer buffer, VkImage image, const ImageData<stbi_uc> &imgData, const LogicalDeviceManager &deviceM, const CommandPoolManager &cmdPoolM,
                                       const QueueFamilyIndices &indices)
{
    const auto &graphicsQueue = deviceM.getGraphicsQueue();
    const auto &device = deviceM.getLogicalDevice();
    const auto &commandPool = cmdPoolM.createSubCmdPool(device, indices, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    CommandBuffer cmdBuffer;

    int uniqueIndex = 0;
    cmdBuffer.createCommandBuffers(deviceM.getLogicalDevice(), commandPool, 1);
    VkCommandBuffer commandBuffer = cmdBuffer.get(uniqueIndex);

    // Recording
    cmdBuffer.beginRecord(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, uniqueIndex);
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
        1};

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    cmdBuffer.endRecord(uniqueIndex);

    // Lot of thing to rethink here
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    vkDestroyCommandPool(device, commandPool, nullptr);
}

void TextureManager::createTextureImageView(VkDevice device)
{

    // Could also simply pass the Image Info
    vkUtils::ImageViewCreateConfig config = {
        .device = device,
        .image = mTextureImage,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT,
        .levelCount = mMipMapLevels
                // Only override what's needed
    };
    mTextureImageView = vkUtils::createImageView(config);
    // Change nothing but that'as an example of usage.
    //  I could set everything to red and rgba would be red
}

void TextureManager::createTextureSampler(VkDevice device, VkPhysicalDevice physDevice)
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    samplerInfo.anisotropyEnable = VK_TRUE;
    // Physical device has a limiti on that

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physDevice, &properties);
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    // MpipMapping stuff
    //PSeudo codehttps://vulkan-tutorial.com/Generating_Mipmaps
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mMipMapLevels);




    if (vkCreateSampler(device, &samplerInfo, nullptr, &mTextureSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create texture sampler!");
    }

    // Probably plentyof thing are device dependent here
}

void TextureManager::destroySampler(VkDevice device)
{

    vkDestroySampler(device, mTextureSampler, nullptr);
}


VkImageView vkUtils::createImageView( ImageViewCreateConfig & config)
{
    return createImageView(config.device, config.image,
    config.format, config.aspectFlags, config.viewType, config.baseMipLevel, config.levelCount,
    config.baseArrayLayer, config.layerCount, config.components);
}


VkImageView vkUtils::createImageView(
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

VkImage vkUtils::createImage(ImageCreateConfig &config)
{

    return createImage(config.device, config.physDevice, config.imageMemory, config.width,
                       config.height, config.format, config.tiling, config.usage, config.properties,
                       config.imageType, config.initialLayout, config.sharingMode, config.mipLevels, config.arrayLayers, config.samples, config.flags);
}

VkImage vkUtils::createImage(
    VkDevice device,
    VkPhysicalDevice physDevice,
    VkDeviceMemory &imageMemory,
    uint32_t width,
    uint32_t height,
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
    imageInfo.extent.depth = 1;
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

//Re do the function
void vkUtils::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, const LogicalDeviceManager &deviceM, const CommandPoolManager &cmdPoolM,
                                    const QueueFamilyIndices &indices , uint32_t mipLevels)
{
    const auto &graphicsQueue = deviceM.getGraphicsQueue();
    const auto &device = deviceM.getLogicalDevice();
    const auto &commandPool = cmdPoolM.createSubCmdPool(device, indices, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
    CommandBuffer cmdBuffer;

    int uniqueIndex = 0;
    cmdBuffer.createCommandBuffers(deviceM.getLogicalDevice(), commandPool, 1);
    VkCommandBuffer commandBuffer = cmdBuffer.get(uniqueIndex);

    cmdBuffer.beginRecord(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, uniqueIndex);
    // Recording
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.image = image;
    // Why
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    // Lot of thing to reread here
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }

    bool hasStencil = (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT);
    // Relate dto depth BUffer
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencil)
        {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    cmdBuffer.endRecord(uniqueIndex);

    // Lot of thing to rethink here
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    vkDestroyCommandPool(device, commandPool, nullptr);
}

void vkUtils::generateMipmaps(VkDevice device,VkPhysicalDevice physicalDevice,  VkCommandPool commandPool, VkImage  image, VkQueue graphicsQueue,
    VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
    {

        // Check if image format supports linear blitting
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }

        // Single time record is about this
        CommandBuffer cmdBuffer;
        int uniqueIndex = 0;
        cmdBuffer.createCommandBuffers(device, commandPool, 1);
        VkCommandBuffer commandBuffer = cmdBuffer.get(uniqueIndex);
        cmdBuffer.beginRecord(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, uniqueIndex);
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

        for (uint32_t i = 1; i < mipLevels; i++) {
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
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
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

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
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

        cmdBuffer.endRecord(uniqueIndex);

        // Lot of thing to retrethinkhink here
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        vkDestroyCommandPool(device, commandPool, nullptr);
    }

/*

Something equivalent ?


makeImageMemoryBarrier : returns VkImageMemoryBarrier for an image based on provided layouts and access flags.

makeImage2DCreateInfo : aids 2d image creation

makeImage3DCreateInfo : aids 3d descriptor set updating

makeImageCubeCreateInfo : aids cube descriptor set updating

makeImageViewCreateInfo : aids common image view creation, derives info from VkImageCreateInfo

//Soùe 
cmdGenerateMipmaps : basic mipmap creation for images (meant for one-shot operations)

makeImageMemoryBarrier : returns VkImageMemoryBarrier for an image based on provided layouts and access flags.

mipLevels : return number of mips for 2d/3d extent

accessFlagsForImageLayout : helps resource transtions

pipelineStageForLayout : helps resource transitions

cmdBarrierImageLayout : inserts barrier for image transition
*/