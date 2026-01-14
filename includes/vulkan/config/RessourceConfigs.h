
#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

namespace vkUtils
{
    namespace Texture
    {
        struct ImageCreateConfig
        {
            VkDevice device = VK_NULL_HANDLE;
            VkPhysicalDevice physDevice = VK_NULL_HANDLE;
            // This may be dangerous
            VkDeviceMemory imageMemory = VK_NULL_HANDLE;
            uint32_t width = 1;
            uint32_t height = 1;
            uint32_t depth = 1;
            uint32_t mipLevels = 1;
            uint32_t arrayLayers = 1;
            VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
            VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
            VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
            VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            VkImageType imageType = VK_IMAGE_TYPE_2D;
            VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
            VkImageCreateFlags flags = 0;
            VmaAllocator allocator = VK_NULL_HANDLE;
            VmaAllocation allocation = VK_NULL_HANDLE;
        };

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

    }
} // namespace vkUtils::Texture



namespace vkUtils
{
    namespace BufferHelper
    {

        struct BufferTransition
        {
            VkBuffer buffer = VK_NULL_HANDLE;
            VkAccessFlags srcAccessMask = 0;
            VkAccessFlags dstAccessMask = 0;
            VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            VkDeviceSize offset = 0;
            VkDeviceSize size = VK_WHOLE_SIZE;
        };
    }

}
