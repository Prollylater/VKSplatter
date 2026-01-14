
#include "utils/RessourceHelper.h"
#include "LogicalDevice.h"
#include "CommandPool.h"

#include <deque>
#include <functional>

///////////////////////////////////
// Buffer
///////////////////////////////////
uint32_t findMemoryType(const VkPhysicalDevice &physDevice, uint32_t memoryTypeBitsRequirement, const VkMemoryPropertyFlags &requiredProperties)
{

    // Todo: Could we keep this around in a variable somewhere ? But where
    VkPhysicalDeviceMemoryProperties pMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physDevice, &pMemoryProperties);

    // Find a memory in `memoryTypeBitsRequirement` that includes all of `requiredProperties`
    // Tldr is, memoryTypeBits is a bit mask defined in hex so we check each property by shifting to it
    // An see if it is set with the &
    // Try debug to see if all value are always here
    const uint32_t memoryCount = pMemoryProperties.memoryTypeCount;
    for (uint32_t memoryIndex = 0; memoryIndex < memoryCount; ++memoryIndex)
    {
        const uint32_t memoryTypeBits = (1 << memoryIndex);
        // Check if the element bit is part of the requirement
        const bool isRequiredMemoryType = memoryTypeBitsRequirement & memoryTypeBits;

        // Then we check if the required property are here
        const VkMemoryPropertyFlags properties =
            pMemoryProperties.memoryTypes[memoryIndex].propertyFlags;
        const bool hasRequiredProperties =
            (properties & requiredProperties) == requiredProperties;

        if (isRequiredMemoryType && hasRequiredProperties)
            return static_cast<int32_t>(memoryIndex);
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

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
            VkDeviceMemory *outimageMemory,
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
            VkImageCreateFlags flags,
            VmaAllocator allocator,
            VmaAllocation *outAlloc)
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

            if (allocator != VK_NULL_HANDLE)
            {
                VmaAllocationCreateInfo allocInfo{};
                //Todo: should be usage and we decide if it's auto
                allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
                if (vmaCreateImage(allocator, &imageInfo, &allocInfo, &image, outAlloc, nullptr) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to allocate image through VMA!");
                }
            }
            else
            {
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

                if (vkAllocateMemory(device, &allocInfo, nullptr, outimageMemory) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to allocate image memory!");
                }

                if (vkBindImageMemory(device, image, *outimageMemory, 0) != VK_SUCCESS)
                {
                    throw std::runtime_error("failed to bind image memory!");
                }
            }
            return image;
        }

        // DeviceMemory of an IMage must be initialized with a config that has be tbrough this function
        VkImage createImage(ImageCreateConfig &config)
        {

            return createImage(config.device, config.physDevice, &config.imageMemory, config.width,
                               config.height, config.depth, config.format, config.tiling, config.usage, config.properties,
                               config.imageType, config.initialLayout, config.sharingMode, config.mipLevels, config.arrayLayers, config.samples, config.flags, config.allocator, &config.allocation);
        }
        /*
        VkPipelineStageFlags accessToStage(VkAccessFlags access) {
    if (access & VK_ACCESS_INDIRECT_COMMAND_READ_BIT)
        return VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
    if (access & VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
        return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    if (access & VK_ACCESS_SHADER_READ_BIT)
        return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
}

        */

        ImageTransition makeTransition(VkImage image,
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
            { // Staging image
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

        // This version use directly a transient CommandBuffer
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

        void generateMipmaps(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t queueIndice, VkImage image, VkQueue graphicsQueue,
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
            cmdPoolM.createCommandPool(device, CommandPoolType::Transient, queueIndice);
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
                // TODO: Blit2
                // https://vkguide.dev/docs/new_chapter_2/vulkan_new_rendering/
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

        void copyBufferToImage(VkCommandBuffer cmdBuffer, VkBuffer buffer, VkImage image, VkExtent3D imgData)
        {
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
                cmdBuffer,
                buffer,
                image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // It is either this or General layout
                1,                                    // Number of regions
                &region);
        }

        // Images from which we copy data must be created with a VK_BUFFER_USAGE_TRANSFER_SRC_BIT
        // PLus the proper transition
        void copyImageToBuffer(VkCommandBuffer cmdBuffer, VkImage image, VkBuffer buffer, VkExtent3D imgData)
        {
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
                cmdBuffer,
                image,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                buffer,
                1,
                &region);
        }

        // Todo:Remove singletime comand BUffer from copyImageToBuffer
        void copyImageToImage(
            VkCommandBuffer cmd,
            VkImage srcImage,
            VkImage dstImage,
            VkExtent3D srcExtent,
            VkExtent3D dstExtent,
            VkImageLayout srcLayout,
            VkImageLayout dstLayout,
            VkFilter filter)
        {
            // Define the region to blit
            VkImageBlit blitRegion{};
            blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blitRegion.srcSubresource.mipLevel = 0;
            blitRegion.srcSubresource.baseArrayLayer = 0;
            blitRegion.srcSubresource.layerCount = 1;
            blitRegion.srcOffsets[0] = {0, 0, 0};
            blitRegion.srcOffsets[1] = {
                static_cast<int32_t>(srcExtent.width),
                static_cast<int32_t>(srcExtent.height),
                static_cast<int32_t>(srcExtent.depth)};

            blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blitRegion.dstSubresource.mipLevel = 0;
            blitRegion.dstSubresource.baseArrayLayer = 0;
            blitRegion.dstSubresource.layerCount = 1;
            blitRegion.dstOffsets[0] = {0, 0, 0};
            blitRegion.dstOffsets[1] = {
                static_cast<int32_t>(dstExtent.width),
                static_cast<int32_t>(dstExtent.height),
                static_cast<int32_t>(dstExtent.depth)};

            // Issue the blit command
            vkCmdBlitImage(
                cmd,
                srcImage, srcLayout,
                dstImage, dstLayout,
                1, &blitRegion,
                filter);
        }

        // Very limited customization for now
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
// namespace vkUtils::Texture

namespace vkUtils
{
    namespace BufferHelper
    {
        // Buffer View can be recreated even after Buffer has been deleted
        VkBufferView createBufferView(VkDevice device, VkBuffer buffer, VkFormat format, VkDeviceSize offset, VkDeviceSize size)
        {
            VkBufferViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
            viewInfo.buffer = buffer;
            viewInfo.offset = offset;
            viewInfo.range = size;
            viewInfo.format = format; // must match how the shader interprets it

            VkBufferView bufferView = VK_NULL_HANDLE;
            VkResult result = vkCreateBufferView(device, &viewInfo, nullptr, &bufferView);
            if (result != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create VkBufferView");
            }

            return bufferView;
        }

        ///////////////////////////////////////////////////////////////////
        ////////////////////////Creation utility///////////////////////////
        ///////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////
        ////////////////////////Memory Barrier utility///////////////////////////
        ///////////////////////////////////////////////////////////////////

        void transitionBuffer(
            const BufferTransition &transitionObject,
            const LogicalDeviceManager &deviceM,
            uint32_t indice)
        {
            const auto &graphicsQueue = deviceM.getGraphicsQueue();
            const auto &device = deviceM.getLogicalDevice();

            CommandPoolManager cmdPoolM;
            cmdPoolM.createCommandPool(device, CommandPoolType::Transient, indice);
            VkCommandBuffer commandBuffer = cmdPoolM.beginSingleTime();

            VkBufferMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.srcAccessMask = transitionObject.srcAccessMask;
            barrier.dstAccessMask = transitionObject.dstAccessMask;

            // Default choice for now
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.buffer = transitionObject.buffer;
            barrier.offset = transitionObject.offset;
            barrier.size = transitionObject.size;

            vkCmdPipelineBarrier(
                commandBuffer,
                transitionObject.srcStageMask, transitionObject.dstStageMask,
                0,
                0, nullptr,
                1, &barrier,
                0, nullptr);

            cmdPoolM.endSingleTime(commandBuffer, graphicsQueue);
            cmdPoolM.destroyCommandPool();
        }

        /////////////////////////////////////////////////
        /////////////////Recording Utility///////////////
        /////////////////////////////////////////////////

        void recordCopy(VkCommandBuffer cmdBuffer,
                        VkBuffer srcBuffer,
                        VkBuffer dstBuffer,
                        VkDeviceSize size,
                        VkDeviceSize srcOffset,
                        VkDeviceSize dstOffset)
        {
            // Recording
            VkBufferCopy copyRegion{};
            copyRegion.srcOffset = srcOffset; // Optional
            copyRegion.dstOffset = dstOffset; // Optional
            copyRegion.size = size;
            vkCmdCopyBuffer(cmdBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
            // Could use a Buffer Memory Barrier and transition here
            // Chapter Copy dataBetween Buffer
        }

        void copyBufferTransient(VkBuffer srcBuffer, VkBuffer dstBuffer,
                                 VkDeviceSize size,
                                 const LogicalDeviceManager &deviceM,
                                 uint32_t indice,
                                 VkDeviceSize srcOffset,
                                 VkDeviceSize dstOffset)
        {
            const auto &graphicsQueue = deviceM.getGraphicsQueue();
            const auto &device = deviceM.getLogicalDevice();

            CommandPoolManager cmdPoolM;
            cmdPoolM.createCommandPool(device, CommandPoolType::Transient, indice);
            VkCommandBuffer commandBuffer = cmdPoolM.beginSingleTime();
            vkUtils::BufferHelper::recordCopy(commandBuffer, srcBuffer, dstBuffer,
                                              size, srcOffset, dstOffset);
            cmdPoolM.endSingleTime(commandBuffer, graphicsQueue);
            cmdPoolM.destroyCommandPool();
        }

        void uploadBufferDirect(VkDevice device, VkDeviceMemory memory,
                                const void *data, VkDeviceSize size, VkDeviceSize offset )
        {
            void *mapped = nullptr;
            if (vkMapMemory(device, memory, offset, size, 0, &mapped) != VK_SUCCESS){
                throw std::runtime_error("Failed to map Vulkan buffer memory!");}

            std::memcpy(static_cast<char *>(mapped), data, static_cast<size_t>(size));
            vkUnmapMemory(device, memory);
        }

        void uploadBufferVMA(VmaAllocator allocator, VmaAllocation allocation,
                             const void *data, VkDeviceSize size, VkDeviceSize offset )
        {
            void *mapped = nullptr;
            if (vmaMapMemory(allocator, allocation, &mapped) != VK_SUCCESS){
                throw std::runtime_error("Failed to map VMA buffer memory!");}

            std::memcpy(static_cast<char *>(mapped) + offset, data, static_cast<size_t>(size));
            vmaUnmapMemory(allocator, allocation);
        }

    }

    class DeletionQueue
    {
    public:
        void push(std::function<void()> &&func)
        {
            deletions.emplace_back(std::move(func));
        }

        void flush()
        {
            for (auto &deletion : deletions)
            {
                if (deletion)
                {
                    deletion();
                }
            }
            deletions.clear();
        }

        void flushR()
        {

            for (auto it = deletions.rbegin(); it != deletions.rend(); it++)
            {
                (*it)();
            }
            deletions.clear();
        }

        void clear()
        {
            deletions.clear();
        }

    private:
        std::deque<std::function<void()>> deletions;
    };

}