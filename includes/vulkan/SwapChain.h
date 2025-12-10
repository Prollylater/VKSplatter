#pragma once

#include "BaseVk.h"

// Todo: Look into those include and all the other

#include "SyncObjects.h"
#include "CommandPool.h"
#include "Buffer.h"
#include "Descriptor.h"
// Move it Away
#include "Texture.h"
class RenderTargetConfig;

#include "RenderPass.h"

// Querying details of swap chain support
// Structure used to query details of a swap chain support

class PhysicalDeviceManager;
class LogicalDeviceManager;

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

//Might as well stay with frame data + new name
class SwapChainResources
{
public:
    void createFramebuffers(VkDevice device, VkExtent2D extent, const std::vector<VkImageView> &attachments, VkRenderPass renderPass);
    void destroyFramebuffers(VkDevice device);

    const std::vector<VkFramebuffer> &GetFramebuffers() const
    {
        return mFramebuffers;
    }

private:
    std::vector<VkFramebuffer> mFramebuffers;
};

//Todo: Move it into it's own class and remove PhysicalDeviceManger forward definition
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


//Toodo: This should belong to the render
struct FrameResources
{
    CommandPoolManager mCommandPool;
    FrameSyncObjects mSyncObjects;
    DescriptorManager mDescriptor;

    //Todo: Mapping is now included in Buffer
    Buffer mCameraBuffer;
    void *mCameraMapping;

    // Legacy
    SwapChainResources mFramebuffer;
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
    // void reCreateSwapChain(VkDevice device, VkPhysicalDevice physDevice, GLFWwindow *window, VkRenderPass renderPass, const GBuffers &depthRess, uint32_t indice);

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
