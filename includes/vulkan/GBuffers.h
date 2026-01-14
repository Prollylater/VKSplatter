#pragma once

#include "Texture.h"
#include <vector>

class PhysicalDeviceManager;

class GBuffers
{
public:
    void init(VkExtent2D size)
    {
        gBuffers.clear();
        mSize = size;
    };
    void createGBuffers(
        const LogicalDeviceManager &logDevice,
        const PhysicalDeviceManager &physDevice,
        const std::vector<VkFormat> &formats,
        VmaAllocator allocator = VK_NULL_HANDLE);
    void createDepthBuffer(const LogicalDeviceManager &, const PhysicalDeviceManager &, VkFormat format, VmaAllocator allocator = VK_NULL_HANDLE);
    void destroy(VkDevice device, VmaAllocator allocator = VK_NULL_HANDLE);

    VkExtent2D getSize() const;
    std::vector<VkImageView> collectColorViews() const;
    std::vector<VkImageView> collectColorViews(const std::vector<uint8_t>& indices) const ;
    VkImage getColorImage(uint32_t) const;
    VkImage getDepthImage() const;
    VkImageView getColorImageView(uint32_t) const;
    VkImageView getDepthImageView() const;
    VkFormat getColorFormat(uint32_t) const;
    VkFormat getDepthFormat() const;
    VkDescriptorImageInfo getGBufferDescriptor(uint32_t) const;
    size_t colorBufferNb() const
    {
        return gBuffers.size();
    }

    std::vector<VkFormat> getAllFormats() const
    {
        std::vector<VkFormat> formats;

        // Add formats for gBuffers
        for (const auto &buffer : gBuffers)
        {
            formats.push_back(buffer.getFormat());
        }

        formats.push_back(gBufferDepth.getFormat());
        return formats;
    }


private:
    std::vector<Image> gBuffers{};
    Image gBufferDepth{};
    VkExtent2D mSize;
};

