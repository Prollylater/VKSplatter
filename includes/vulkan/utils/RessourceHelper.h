
#pragma once
#include "config/RessourceConfigs.h"

struct QueueFamilyIndices;
class LogicalDeviceManager;


//Undecided on this position
uint32_t findMemoryType(const VkPhysicalDevice &device, uint32_t memoryTypeBitsRequirement, const VkMemoryPropertyFlags &requiredProperties);


namespace vkUtils
{
    namespace Texture
    {
        // Second namespace here
        // Too big need to rework
        void generateMipmaps(VkDevice device, VkPhysicalDevice physicalDevice, const QueueFamilyIndices &indices, VkImage image, VkQueue graphicsQueue,
                             VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

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

        // void record
        void copyBufferToImage(VkBuffer buffer, VkImage image, VkExtent3D imgData, const LogicalDeviceManager &deviceM,
                               const QueueFamilyIndices &indices);
        void copyImageToBuffer(VkImage image, VkBuffer buffer, const LogicalDeviceManager &deviceM,
                               const QueueFamilyIndices &indices);

        VkSampler createSampler(VkDevice device, VkPhysicalDevice physDevice, int mipmaplevel);

        void recordImageMemoryBarrier(VkCommandBuffer cmdBuffer,
                                      const ImageTransition &transitionObject);

        VkImage createImage(
            VkDevice device,
            VkPhysicalDevice physDevice,
            // This may be dangerous
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

    }
} // namespace vkUtils::Texture

namespace vkUtils
{
    namespace BufferHelper
    {
        // Buffer View can be recreated even after Buffer has been deleted
         VkBufferView createBufferView(VkDevice device, VkBuffer buffer, VkFormat format, VkDeviceSize offset, VkDeviceSize size = VK_WHOLE_SIZE);
        ///////////////////////////////////////////////////////////////////
        ////////////////////////Creation utility///////////////////////////
        ///////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////
        ////////////////////////Memory Barrier utility///////////////////////////
        ///////////////////////////////////////////////////////////////////

        // Rename to insert memory Barrier
        void transitionBuffer(
            const BufferTransition &transitionObject,
            const LogicalDeviceManager &deviceM,
            uint32_t indice);

        /////////////////////////////////////////////////
        /////////////////Recording Utility///////////////
        /////////////////////////////////////////////////

         void recordCopy(VkCommandBuffer cmdBuffer,
                               VkBuffer srcBuffer,
                               VkBuffer dstBuffer,
                               VkDeviceSize size,
                               VkDeviceSize srcOffset,
                               VkDeviceSize dstOffset);

        void copyBufferTransient(VkBuffer srcBuffer, VkBuffer dstBuffer,
                                 VkDeviceSize size,
                                 const LogicalDeviceManager &deviceM,
                                 uint32_t indice,
                                 VkDeviceSize srcOffset = 0,
                                 VkDeviceSize dstOffset = 0);

        void uploadBufferDirect(
            VkDeviceMemory bufferMemory,
            const void *data,
            VkDevice device,
            VkDeviceSize size,
            VkDeviceSize dstOffset);
    }

}
