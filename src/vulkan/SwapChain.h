#pragma once

#include "BaseVk.h"
#include "QueueFam.h"


struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class SwapChainManager
{
public:
    SwapChainManager() = default;
    ~SwapChainManager() = default;

    //Todo: MSurface don't need to be held in swapchain
    void createSurface(VkInstance instance, GLFWwindow *window);
    void destroySurface();
    VkSurfaceKHR GetSurface() const;

    void createSwapChain(VkPhysicalDevice device, VkDevice logicalDevice, GLFWwindow *window, const SwapChainConfig &config, const QueueFamilyIndices &indices);
 
    void destroySwapChain(VkDevice logicalDevice);

    bool aquireNextImage(VkDevice device, VkSemaphore semaphore, uint32_t &imageIndex);
    VkSwapchainKHR GetChain() const;

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &, const std::vector<VkPresentModeKHR> &);

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, GLFWwindow *window);

    const std::vector<VkImage> &GetSwapChainImages() const
    {
        return mSwapChainImages;
    }

    void createImageViews(VkDevice device);

    void destroyImageViews(VkDevice device);

    const std::vector<VkImageView> &GetSwapChainImageViews() const
    {
        return mSChainImageViews;
    }
    const VkSurfaceFormatKHR getSwapChainImageFormat() const
    {
        return mSwapChainImageFormat;
    }

    const VkExtent2D getSwapChainExtent() const
    {
        return mSwapChainExtent;
    }

    const SwapChainConfig getConfig() const
    {
        return mConfig;
    }

 

private:
  
    VkSurfaceFormatKHR mSwapChainImageFormat;

    // Remove ? Really exist due to Surface 
    VkInstance mInstance;
    //To Windows VK classs ?
    VkSurfaceKHR mSurface = VK_NULL_HANDLE;

    VkSwapchainKHR mSwapChain = VK_NULL_HANDLE;
    VkExtent2D mSwapChainExtent;
    std::vector<VkImage> mSwapChainImages;
    std::vector<VkImageView> mSChainImageViews;

    // Todo: Not sure, additionnal data for swapchain recreation
    SwapChainConfig mConfig;
    //Not really needed further for now
    SwapChainSupportDetails mSupportDetails;

};
