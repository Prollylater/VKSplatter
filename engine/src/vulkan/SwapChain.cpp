#include "SwapChain.h"
#include "utils/RessourceHelper.h"
#include <limits>

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
  return availableFormats[1];
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
  destroyImageViews(device);
  vkDestroySwapchainKHR(device, mSwapChain, nullptr);
  // Actual content is handled alongside instructions above
  mSwapChainImages.clear();
}

VkSwapchainKHR SwapChainManager::GetChain() const
{
  return mSwapChain;
}

// ImageViews

void SwapChainManager::createImageViews(VkDevice device)
{
  mSChainImageViews.resize(mSwapChainImages.size());

  for (size_t i = 0; i < mSwapChainImages.size(); i++)
  {

    mSChainImageViews[i] = vkUtils::Texture::createImageView(device, mSwapChainImages[i],
                                                             mSwapChainImageFormat.format, VK_IMAGE_ASPECT_COLOR_BIT);
  }
}

void SwapChainManager::destroyImageViews(VkDevice device)
{

  for (auto imageView : mSChainImageViews)
  {
    vkDestroyImageView(device, imageView, nullptr);
  }
  mSChainImageViews.clear();
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
