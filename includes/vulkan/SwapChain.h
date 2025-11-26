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

//Todo: Move it into it's own class
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
    VkImage getColorImage(uint32_t) const;
    VkImage getDepthImage() const;
    VkImageView getColorImageView(uint32_t) const;
    VkImageView getDepthImageView() const;
    VkFormat getColorFormat(uint32_t) const;
    VkFormat getDepthFormat() const;
    VkDescriptorImageInfo getGBufferDescriptor(uint32_t) const;
    size_t colorBufferNb()
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

    void destroyGBuffers(VkDevice);
    void destroyDepthBuffer(VkDevice);
};



struct DynamicPassInfo
{
    VkRenderingInfo info;
    std::vector<VkRenderingAttachmentInfo> colorAttachments;
    VkRenderingAttachmentInfo depthAttachment;
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

    // Dynamic Rendering Path
    DynamicPassInfo mDynamicPassInfo;
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

    void DestroyImageViews(VkDevice device);

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
    // Remove ?
    VkInstance mInstance;
    SwapChainSupportDetails mSupportDetails;
    VkSurfaceFormatKHR mSwapChainImageFormat;

    VkSurfaceKHR mSurface = VK_NULL_HANDLE;
    VkSwapchainKHR mSwapChain = VK_NULL_HANDLE;

    VkExtent2D mSwapChainExtent;
    std::vector<VkImage> mSwapChainImages;
    std::vector<VkImageView> mSChainImageViews;
/*
    uint32_t currentFrame = 0;
    std::vector<FrameResources> mFramesData;

    void createFrameData(VkDevice device, VkPhysicalDevice physDevice, uint32_t queueIndice);
    void destroyFrameData(VkDevice device);*/

    // Todo: Not sure, additionnal data for swapchain recreation
    SwapChainConfig mConfig;
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

/*
a framebuffer references image views that are to be used for color, depth and stencil targets.
*/

/*
VulkanContext
└── SwapchainManager
    ├── VkSwapchainKHR
    ├── swapchain images / views
    ├── FrameResources[MAX_FRAMES_IN_FLIGHT]
    └── DepthResources
        ├── VkImage mDepthImage
        ├── VkDeviceMemory mDepthMemory
        ├── VkImageView mDepthView
        └── VkFormat mDepthFormat

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