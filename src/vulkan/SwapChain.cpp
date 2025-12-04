#include "SwapChain.h"

#include <limits>
#include <algorithm>
#include "Texture.h"
#include "LogicalDevice.h"
#include "PhysicalDevice.h"
#include "CommandPool.h"
#include "utils/RessourceHelper.h"

// Todo: Important consider manual transition of arrachement images when not Present and what it would mean
// Todo: Reconsider how are used all std::runtimeError

void SwapChainManager::createSurface(VkInstance instance, GLFWwindow *window)
{
  // Thank to glfw we avoid the hardway
  mInstance = instance;
  if (glfwCreateWindowSurface(mInstance, window, nullptr, &mSurface) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create window surface!");
  }
};

void SwapChainManager::destroySurface()
{
  vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
};

VkSurfaceKHR SwapChainManager::GetSurface() const
{
  return mSurface;
};

/////////////////////////////////
//////////////////////////SwapChain
/////////////////////////////////

SwapChainSupportDetails SwapChainManager::querySwapChainSupport(VkPhysicalDevice device) const
{
  SwapChainSupportDetails details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, mSurface, &details.capabilities);

  // Format capabilities which include vKFormat and VKColorSpace
  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatCount, nullptr);
  if (formatCount != 0)
  {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatCount, details.formats.data());
  }

  // Present Mode: Around Line 7517 in Vulkan.h in an enum

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, mSurface, &presentModeCount, nullptr);

  if (presentModeCount != 0)
  {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, mSurface, &presentModeCount, details.presentModes.data());
  }

  return details;
}

VkSurfaceFormatKHR SwapChainManager::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
  for (const auto &availableFormat : availableFormats)
  {
    if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return availableFormat;
    }
  }
  return availableFormats[0];
}

VkPresentModeKHR SwapChainManager::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablModes, const std::vector<VkPresentModeKHR> &preferredModes)
{
  for (VkPresentModeKHR preferred : preferredModes)
  {
    for (VkPresentModeKHR available : availablModes)
    {
      if (available == preferred)
      {
        return preferred;
      }
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D SwapChainManager::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, GLFWwindow *window)
{
  // Swap extent == resolution of swpa chain images in pixel coordiantes
  // And the limit
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
  {
    return capabilities.currentExtent;
  }
  else
  {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)};

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

bool SwapChainManager::aquireNextImage(VkDevice device, VkSemaphore semaphore, uint32_t &imageIndex)
{

  VkResult result = vkAcquireNextImageKHR(
      device,
      mSwapChain,
      UINT64_MAX,
      semaphore, // Trigger a  signal
      VK_NULL_HANDLE,
      &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR)
  {
    // Todo: Why is it treated outside again
    //  recreateSwapChain(); // must exist
    //  Don't return directly since it don't have acces to the Framebuffers
    return false;
  }
  else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
  {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  return true;
}

void SwapChainManager::createSwapChain(VkPhysicalDevice physDevice, VkDevice logicalDevice, GLFWwindow *window, const SwapChainConfig &config, const QueueFamilyIndices &indices)
{
  mSupportDetails = querySwapChainSupport(physDevice);
  mConfig = config;
  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(mSupportDetails.formats);
  VkPresentModeKHR presentMode = chooseSwapPresentMode(mSupportDetails.presentModes, mConfig.preferredPresentModes);
  VkExtent2D extent = chooseSwapExtent(mSupportDetails.capabilities, window);

  mSwapChainExtent = extent;

  // Seem like being conservative is better as imageCount will be allocated in  vram
  // with image having their own tied element

  uint32_t imageCount = std::max(mSupportDetails.capabilities.minImageCount, mConfig.preferredImageCount);

  if (mSupportDetails.capabilities.maxImageCount > 0 &&
      imageCount > mSupportDetails.capabilities.maxImageCount)
  {
    imageCount = mSupportDetails.capabilities.maxImageCount;
  }

  // Actual creation
  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = mSurface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1; // Not sure
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

  if (indices.graphicsFamily != indices.presentFamily)
  {
    // Image used by the two quue
    // Non explicit ownerships transfers of image between queue
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    // Specify yhe family sharing all
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  }
  else
  {
    // image owned by a single queue at the same time
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // Below are optional for this mode
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
  }

  // Todo: Need to revisit this
  createInfo.preTransform = mSupportDetails.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  // True will clip pixels obscured
  // Wouldn"t work with some offscreen rendering ?
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &mSwapChain) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create swap chain!");
  }

  vkGetSwapchainImagesKHR(logicalDevice, mSwapChain, &imageCount, nullptr);
  mSwapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(logicalDevice, mSwapChain, &imageCount, mSwapChainImages.data());

  mSwapChainImageFormat = surfaceFormat;
}

void SwapChainManager::destroySwapChain(VkDevice device)
{
  vkDestroySwapchainKHR(device, mSwapChain, nullptr);
  // Actual content is handled alongside instructions above
  mSwapChainImages.clear();
}

VkSwapchainKHR SwapChainManager::GetChain() const
{
  return mSwapChain;
}

// ImageViews

// Just for transition image currently

void SwapChainManager::createImageViews(VkDevice device)
{
  mSChainImageViews.resize(mSwapChainImages.size());

  for (size_t i = 0; i < mSwapChainImages.size(); i++)
  {

    mSChainImageViews[i] = vkUtils::Texture::createImageView(device, mSwapChainImages[i],
                                                             mSwapChainImageFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
  }
}

void SwapChainManager::DestroyImageViews(VkDevice device)
{

  for (auto imageView : mSChainImageViews)
  {
    vkDestroyImageView(device, imageView, nullptr);
  }
  mSChainImageViews.clear();
}

// Passes Ressources
// Todo::
void SwapChainResources::createFramebuffers(VkDevice device, VkExtent2D extent, const std::vector<VkImageView> &attachments, VkRenderPass renderPass)
{

  // Not sure of why sam depth Ressources, seem like a bad idea
  mFramebuffers.resize(1);
  VkFramebufferCreateInfo framebufferInfo{};
  framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  framebufferInfo.renderPass = renderPass;
  framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  framebufferInfo.pAttachments = attachments.data();
  framebufferInfo.width = extent.width;
  framebufferInfo.height = extent.height;
  framebufferInfo.layers = 1; // Array

  if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, mFramebuffers.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create framebuffer!");
  }
}

void SwapChainResources::destroyFramebuffers(VkDevice device)
{
  for (auto framebuffer : mFramebuffers)
  {
    if (framebuffer != VK_NULL_HANDLE)
    {
      vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
  }
  mFramebuffers.clear();
}

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

// Querying details of swap chain support

//"Hard way" is the usual method but still suing glfw
// Using a struct create Info
/*
VkWin32SurfaceCreateInfoKHR createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
createInfo.hwnd = glfwGetWin32Window(window);
createInfo.hinstance = GetModuleHandle(nullptr);

And without it we might have to directly pass an handle to the native element
xreated with x11 or other
VkXlibSurfaceCreateInfoKHR createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
createInfo.dpy = display;
createInfo.window = window;

*/

// PRESENT MODE

/*

Vertical blank : The moment the ,next frame start to be drawn (or just before)
Vsynched when the vertical bank is allowed to happen before fram sendinge
Only the VK_PRESENT_MODE_FIFO_KHR mode is guaranteed to be available, so we'll again have to write a function that looks for the best mode that is available:

VK_PRESENT_MODE_IMMEDIATE_KHR
Frames are sent to the screen immediately. May cause screen tearing. Lowest latency.

VK_PRESENT_MODE_MAILBOX_KHR
Triple buffering. Newer frames replace older ones if not yet shown. Prevents tearing and keeps latency low.
A bit like when using multiple frame buffer in gl

VK_PRESENT_MODE_FIFO_KHR
Like vertical sync (vsync). Frames queue and are shown during vertical blank. Always supported.

VK_PRESENT_MODE_FIFO_RELAXED_KHR
Similar to FIFO, but if the app misses the vertical blank, the next frame is shown immediately. May tear.


//Some mode are used for Shared surface ?
 */

//////////////////////
/*
That leaves one last field, oldSwapChain.
With Vulkan it's possible that your swap chain becomes invalid or unoptimized
while your application is running, for example because the window was resized. In that case the swap chain actually needs to be recreated from scratch and a reference to the old one must be specified in this field.
*/
