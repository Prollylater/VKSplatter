#include "GBuffers.h"
#include "utils/RessourceHelper.h"
#include "LogicalDevice.h"
#include "PhysicalDevice.h"

//Todo: The need of using the Manager class is not quit clear
//Revisit this
void GBuffers::createGBuffers(
    const LogicalDeviceManager &logDevice,
    const PhysicalDeviceManager &physDevice,
    const std::vector<VkFormat> &formats,
    VmaAllocator allocator)
{
  gBuffers.reserve(formats.size());

  for (auto format : formats)
  {
    vkUtils::Texture::ImageCreateConfig cfg{};
    cfg.device = logDevice.getLogicalDevice();
    cfg.physDevice = physDevice.getPhysicalDevice();
    cfg.height = mSize.height;
    cfg.width = mSize.width;
    cfg.format = format;
    cfg.allocator = allocator;

    // Notes that createImage has currently an hidden Transfer dst bit addition
    cfg.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
                VK_IMAGE_USAGE_STORAGE_BIT;
    // VK_IMAGE_USAGE_SAMPLED_BIT;

    gBuffers.push_back({});
    gBuffers.back().createImage(cfg);
    gBuffers.back().createImageView(cfg.device, VK_IMAGE_ASPECT_COLOR_BIT);
  }
  // Left by default as undefined, all transition are handled in shader
}

void GBuffers::createDepthBuffer(const LogicalDeviceManager &logDeviceM,
                                 const PhysicalDeviceManager &physDeviceM, VkFormat format, VmaAllocator allocator)
{
  // Todo: compare with creating sampled image
  // mDepthFormat = physDeviceM.findDepthFormat();
  bool hasStencil = (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT);

  vkUtils::Texture::ImageCreateConfig depthConfig;
  depthConfig.device = logDeviceM.getLogicalDevice();
  depthConfig.physDevice = physDeviceM.getPhysicalDevice();
  depthConfig.height = mSize.height;
  depthConfig.width = mSize.width;
  depthConfig.format = format;
  depthConfig.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  depthConfig.allocator = allocator;

  gBufferDepth.createImage(depthConfig);

  const auto indices = physDeviceM.getIndices();

  VkImageAspectFlags imgAspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
  if (hasStencil)
  {
    imgAspectFlag |= VK_IMAGE_ASPECT_STENCIL_BIT;
  }
  gBufferDepth.createImageView(depthConfig.device, imgAspectFlag);

  // Left by default as undefined, all transition are handled in shader
}
void GBuffers::destroy(VkDevice device, VmaAllocator allocator)
{
  for (auto &gBuffer : gBuffers)
  {
    gBuffer.destroyImage(device, allocator);
  };

  gBufferDepth.destroyImage(device, allocator);
}

VkExtent2D GBuffers::getSize() const
{
  return mSize;
};
VkImage GBuffers::getColorImage(uint32_t index) const
{
  return gBuffers[index].getImage();
};
VkImage GBuffers::getDepthImage() const
{
  return gBufferDepth.getImage();
};

std::vector<VkImageView> GBuffers::collectColorViews() const
{
  std::vector<VkImageView> colorViews;
  colorViews.reserve(colorBufferNb());
  for (size_t index = 0; index < colorBufferNb(); index++)
  {
    colorViews.push_back(getColorImageView(index));
  }
  return colorViews;
};

std::vector<VkImageView> GBuffers::collectColorViews(const std::vector<uint8_t> &indices) const
{
  std::vector<VkImageView> colorViewsSubset;
  colorViewsSubset.reserve(indices.size()); // Reserve enough space

  for (size_t index : indices)
  {
    if (index < colorBufferNb())
    { // Check for valid index
      colorViewsSubset.push_back(getColorImageView(index));
    }
  }

  return colorViewsSubset;
}

VkImageView GBuffers::getColorImageView(uint32_t index) const
{
  return gBuffers[index].getView();
};
VkImageView GBuffers::getDepthImageView() const
{
  return gBufferDepth.getView();
};
VkFormat GBuffers::getColorFormat(uint32_t index) const
{
  return gBuffers[index].getFormat();
};
VkFormat GBuffers::getDepthFormat() const
{
  return gBufferDepth.getFormat();
};
VkDescriptorImageInfo GBuffers::getGBufferDescriptor(uint32_t index) const
{
  return gBuffers[index].getDescriptor();
};

/*
 void GBuffers::destroyDepthBuffer(VkDevice device)
  {
    if (mDepthImage != VK_NULL_HANDLE)
    {
      vkDestroyImageView(device, mDepthView, nullptr);
      vkDestroyImage(device, mDepthImage, nullptr);
      vkFreeMemory(device, mDepthMemory, nullptr);
      mDepthView = VK_NULL_HANDLE;
      mDepthImage = VK_NULL_HANDLE;
      mDepthMemory = VK_NULL_HANDLE;
    }
  }
*/
