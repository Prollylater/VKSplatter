#include "SwapChain.h"

#include <limits>
#include <algorithm>
#include "Texture.h"
#include "LogicalDevice.h"
#include "PhysicalDevice.h"
#include "CommandPool.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void SwapChainManager::createSurface(VkInstance instance, GLFWwindow *window)
{
  // Thank to glfw we avoid the hardway
  mInstance = instance;
  if (glfwCreateWindowSurface(mInstance, window, nullptr, &mSurface) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create window surface!");
  }
};

void SwapChainManager::DestroySurface()
{
  vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
};

VkSurfaceKHR SwapChainManager::GetSurface() const
{
  return mSurface;
};

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

  // PResent Mode: Around Line 7517 in Vulkan.h in an enum

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
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
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
  // Useful link describing the spec: https://registry.khronos.org/vulkan/specs/latest/man/html/VkSurfaceCapabilitiesKHR.html
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
      semaphore, //To signal
      VK_NULL_HANDLE,
      &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR)
  {
    // recreateSwapChain(); // must exist
    // Don't return directly since it don't have acces to the Framebuffers
    return false;
  }
  else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
  {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  return true;
}

void SwapChainManager::createSwapChain(VkPhysicalDevice physDevice, VkDevice logicalDevice, GLFWwindow *window, const SwapChainConfig &config)
{
  mFramesData.resize(ContextVk::contextInfo.MAX_FRAMES_IN_FLIGHT);
  mSupportDetails = querySwapChainSupport(physDevice);

  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(mSupportDetails.formats);
  VkPresentModeKHR presentMode = chooseSwapPresentMode(mSupportDetails.presentModes, config.preferredPresentModes);
  VkExtent2D extent = chooseSwapExtent(mSupportDetails.capabilities, window);
 
  mSwapChainExtent = extent;

  // Seem like being conservative is better as imageCount will be allocated  an deat vram
  // with image having their own tied element

  uint32_t imageCount = std::max(mSupportDetails.capabilities.minImageCount, config.preferredImageCount);

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

  QueueFamilyIndices indices = findQueueFamilies(physDevice, mSurface, ContextVk::contextInfo.getDeviceSelector());
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

  //Todo: Need to revisit this
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

  mSwapChainImageFormat = surfaceFormat.format;
}

void SwapChainManager::DestroySwapChain(VkDevice logicalDevice)
{
  vkDestroySwapchainKHR(logicalDevice, mSwapChain, nullptr);
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

    mSChainImageViews[i] = vkUtils::createImageView(device, mSwapChainImages[i],
                                                    mSwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
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

//Frame Data Ressources

  /*const*/ FrameResources &SwapChainManager::getCurrentFrameData() //const
    {
        return mFramesData[currentFrame];
    }

    const int SwapChainManager::getCurrentFrameIndex() const
    {
        return currentFrame;
    }

    void SwapChainManager::advanceFrame()
    {
        currentFrame++;
        if (currentFrame >= mFramesData.size())
        {
            currentFrame = 0;
        }
    }

    void SwapChainManager::createFrameData(VkDevice device, uint32_t queueIndice)
    {
        auto &frameData = mFramesData[currentFrame];
        frameData.mSyncObjects.createSyncObjects(device);
        frameData.mCommandPool.createCommandPool(device, CommandPoolType::Frame, queueIndice);
        frameData.mCommandPool.createCommandBuffers(1);
    };

    void SwapChainManager::destroyFrameData(VkDevice device)
    {
        auto &frameData = mFramesData[currentFrame];
        frameData.mSyncObjects.destroy(device);
        frameData.mCommandPool.destroyCommandPool();

    };

    void SwapChainManager::createFramesData(VkDevice device, uint32_t queueIndice)
    {
        currentFrame = 0;

        for (int i = 0; i < mFramesData.size(); i++)
        {
            createFrameData(device, queueIndice);
            advanceFrame();
        }
        currentFrame = 0;
    };
    
    void SwapChainManager::destroyFramesData(VkDevice device)
    {
        currentFrame = 0;
        for (int i = 0; i < mFramesData.size(); i++)
        {
            destroyFrameData(device);
            advanceFrame();
        }
        currentFrame = 0;
    };

//Passes Ressources

void SwapChainResources::createFramebuffers(VkDevice device, const SwapChainManager &swapChain, const DepthRessources &depthRess, VkRenderPass renderPass)
{
  const auto &imageViews = swapChain.GetSwapChainImageViews();
  VkExtent2D extent = swapChain.getSwapChainExtent();

  mFramebuffers.resize(imageViews.size());

  for (size_t i = 0; i < imageViews.size(); i++)
  {
    std::array<VkImageView, 2> attachments = {
        imageViews[i],
        depthRess.getView()};

    // Not sure of why sam depth Ressources, seem like a bad idea

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &mFramebuffers[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("failed to create framebuffer!");
    }
  }
}

void SwapChainResources::destroyFramebuffers(VkDevice device)
{
  for (auto framebuffer : mFramebuffers)
  {
    vkDestroyFramebuffer(device, framebuffer, nullptr);
  }
  mFramebuffers.clear();
}

void DepthRessources::createDepthBuffer(const LogicalDeviceManager &logDeviceM,
                                        const SwapChainManager &swapChain, const PhysicalDeviceManager &physDeviceM)
{
  mDepthFormat = physDeviceM.findSupportedFormat(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL,
      VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

  bool hasStencil = (mDepthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT || mDepthFormat == VK_FORMAT_D24_UNORM_S8_UINT);

  auto swapChainExtent = swapChain.getSwapChainExtent();
  mDepthImage = vkUtils::createImage(logDeviceM.getLogicalDevice(), physDeviceM.getPhysicalDevice(), mDepthMemory, swapChainExtent.width, swapChainExtent.height,
                                     mDepthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  mDepthView = vkUtils::createImageView(logDeviceM.getLogicalDevice(), mDepthImage, mDepthFormat,
                                        VK_IMAGE_ASPECT_DEPTH_BIT);

  const auto indices = physDeviceM.getIndices();
  vkUtils::transitionImageLayout(mDepthImage, mDepthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, logDeviceM, indices);
}

void DepthRessources::destroyDepthBuffer(VkDevice device)
{
  vkDestroyImageView(device, mDepthView, nullptr);
  vkDestroyImage(device, mDepthImage, nullptr);
  vkFreeMemory(device, mDepthMemory, nullptr);
}



















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
