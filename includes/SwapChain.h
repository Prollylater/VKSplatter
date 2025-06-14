#pragma once

#include "BaseVk.h"

// Querying details of swap chain support
// Structure used to query details of a swap chain support
struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class PhysicalDeviceManager;
class LogicalDeviceManager;
class CommandPoolManager;
class DepthRessources;
class SwapChainRessources;

class SwapChainManager
{
public:
    void CreateSurface(VkInstance instance, GLFWwindow *window);
    void DestroySurface();
    VkSurfaceKHR GetSurface() const;

    void CreateSwapChain(VkPhysicalDevice device, VkDevice logicalDevice, GLFWwindow *window);
    void DestroySwapChain(VkDevice logicalDevice);

    bool aquireNextImage(VkDevice device, VkSemaphore semaphore, uint32_t& imageIndex) {

    VkResult result = vkAcquireNextImageKHR(
        device, 
        mSwapChain, 
        UINT64_MAX,
        semaphore,
        VK_NULL_HANDLE,
        &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        //recreateSwapChain(); // must exist
        //Don't return directly since it don't have acces to the Framebuffers
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    return true;
    }
    VkSwapchainKHR GetChain() const;

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) const;
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, GLFWwindow *window);

    const std::vector<VkImage> &GetSwapChainImages() const
    {
        return mSwapChainImages;
    }


void CreateImageViews(VkDevice device);

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
    VkSurfaceKHR mSurface;
    VkSwapchainKHR mSwapChain;
    // Handle to current
    // Validation function too ?
    VkInstance mInstance;

    // For now we store it here could also return the std::vector
    SwapChainSupportDetails mSupportDetails;

    VkFormat mSwapChainImageFormat;
    VkExtent2D mSwapChainExtent;

    std::vector<VkImage> mSwapChainImages;
    std::vector<VkImageView> mSChainImageViews;
};


class SwapChainResources {
public:
    //void createFramebuffers(VkDevice device, const SwapChainManager& swapChain, VkRenderPass renderPass);
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