#pragma once

#include "BaseVk.h"

// Querying details of swap chain support
// Structure used to query details of a swap chain support

class PhysicalDeviceManager;
class LogicalDeviceManager;
class CommandPoolManager;
class DepthRessources;
class SwapChainRessources;

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct SwapChainConfig {
    VkSurfaceFormatKHR preferredFormat = {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    std::vector<VkPresentModeKHR> preferredPresentModes = {
        VK_PRESENT_MODE_MAILBOX_KHR,
        VK_PRESENT_MODE_FIFO_KHR
    };
    // Triple buffering by default
    const uint32_t preferredImageCount = 3; 
    // 0 means "derive from window"
    VkExtent2D preferredExtent = {0, 0}; 
    bool allowExclusiveSharing = true;
};

class SwapChainManager
{
public:
 SwapChainManager() = default;
    ~SwapChainManager() = default;

    void CreateSurface(VkInstance instance, GLFWwindow *window);
    void DestroySurface();
    VkSurfaceKHR GetSurface() const;

    void createSwapChain(VkPhysicalDevice device, VkDevice logicalDevice, GLFWwindow *window,  const SwapChainConfig& config);
    void DestroySwapChain(VkDevice logicalDevice);

    bool aquireNextImage(VkDevice device, VkSemaphore semaphore, uint32_t& imageIndex);
    VkSwapchainKHR GetChain() const;

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) const;
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &, const std::vector<VkPresentModeKHR> &);

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, GLFWwindow *window);

    const std::vector<VkImage> &GetSwapChainImages() const
    {
        return mSwapChainImages;
    }


void createImageViews(VkDevice device);

void DestroyImageViews(VkDevice device);
  

  const std::vector<VkImageView> &GetSwapChainImageViews() const 
    {
        return mSChainImageViews;
    }
      const VkFormat getSwapChainImageFormat() const 
    {
        return mSwapChainImageFormat;
    }

      const VkExtent2D getSwapChainExtent() const 
    {
        return mSwapChainExtent;
    }

private:
    VkSurfaceKHR mSurface = VK_NULL_HANDLE;
    VkSwapchainKHR mSwapChain = VK_NULL_HANDLE;
    //Remove
    VkInstance mInstance;

    // For now we store it here could also return the std::vector
    SwapChainSupportDetails mSupportDetails;

    VkFormat mSwapChainImageFormat;
    VkExtent2D mSwapChainExtent;

    std::vector<VkImage> mSwapChainImages;
    std::vector<VkImageView> mSChainImageViews;
};



/*

[ Shadow Pass       ] → writes depth-only
       ↓
[ G-Buffer Pass     ] → writes to multiple attachments (albedo, normal, etc.)
       ↓
[ Lighting Pass     ] → reads G-buffer, outputs lit result
       ↓
[ Post-Processing   ] → bloom, tone mapping, etc.
       ↓
[ **Swapchain Pass** ] → Single Color Attachment


*/
class SwapChainResources {
public:
    void createFramebuffers(VkDevice device, const SwapChainManager &swapChain,const DepthRessources &depthRess, VkRenderPass renderPass);
    
    void destroyFramebuffers(VkDevice device);
    /*
    a framebuffer references image views that are to be used for color, depth and stencil targets. 
    */
    const std::vector<VkFramebuffer>& GetFramebuffers() const {
        return mFramebuffers;
    }

private:
    std::vector<VkFramebuffer> mFramebuffers;
};


class DepthRessources {
public:
    void createDepthBuffer(const LogicalDeviceManager&,
const SwapChainManager &swapChain, const PhysicalDeviceManager &,  const CommandPoolManager& cmdPoolM);
    void destroyDepthBuffer(VkDevice);
    VkImageView getView() const {
        return mDepthView;
    };
    VkFormat getFormat() const {
        return mDepthFormat;
    };
private:
    VkImage mDepthImage = VK_NULL_HANDLE;
    VkDeviceMemory mDepthMemory  = VK_NULL_HANDLE;
    VkImageView mDepthView = VK_NULL_HANDLE;
    VkFormat mDepthFormat ;
};



/*

VkInstance
   └── VkSurfaceKHR (created for a window, platform-specific)
        └── Used to query which VkPhysicalDevice supports presenting to it
            └── VkPhysicalDevice (GPU)
                └── Create VkDevice (logical device)
                    └── VkSwapchainKHR (created for the surface)
                         └── VkImage[] (owned by the device, created by Vulkan)
                              └── VkImageView (created by for rendering)

A VkImageView help defiend how the Image is interpretated and necessary to use it as framebuffer
With the Surface being a side concept that represent the windows area, and work for platform specific extension

VkInstance
  └── VkSurfaceKHR
       └── queried by VkPhysicalDevice (GPU) to check surface support
            └── VkDevice (logical device)
                 └── VkSwapchainKHR
                      ├── VkImage[]          ← owned by the swapchain
                      │     └── VkImageView[]  ← created by you
                      │           └── used in: VkFramebuffer[]
                      └── VkExtent2D         ← size of the surface (window)
                      
VkRenderPass
  └── Describes how framebuffer attachments are used
       └── used to create: VkFramebuffer[] ← one per VkImageView


*/