#pragma once

#include "BaseVk.h"

// Todo: Look into those include and all the other

#include "SyncObjects.h"
#include "CommandPool.h"
#include "Buffer.h"
#include "Descriptor.h"

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

// Move it Away
#include "Texture.h"
class GBuffers
{
public:
    void init(VkExtent2D size) { mSize = size; };
    void createGBuffers(
        const LogicalDeviceManager &logDevice,
        const PhysicalDeviceManager &physDevice,
        const std::vector<VkFormat> &formats);
    void createDepthBuffer(const LogicalDeviceManager &, const PhysicalDeviceManager &, VkFormat format);
    void destroy(VkDevice device);

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

// Temp
struct UniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct DynamicPassInfo
{
    VkRenderingInfo info;
    std::vector<VkRenderingAttachmentInfo> colorAttachments;
    VkRenderingAttachmentInfo depthAttachment;
};

struct FrameResources
{
    CommandPoolManager mCommandPool;
    FrameSyncObjects mSyncObjects;
    DescriptorManager mDescriptor;
    Buffer mCameraBuffer;
    void *mCameraMapping;

    // Dynamic Rendering Path
    DynamicPassInfo mDynamicPassInfo;
    // Legacy
    SwapChainResources mFramebuffer;
};

/*
Swapchain images (implicitly destroyed by vkDestroySwapchainKHR)
Image views for swapchain images
Depth buffer image + image view (extent changes with the swapchain)
Framebuffers (point to swapchain + depth attachments, so must be rebuilt)
Render pass (if its attachments depend on swapchain format/extent — often yes)
Pipelines (if they use the swapchain extent for viewport/scissor baked into state)
*/
class SwapChainManager
{
public:
    SwapChainManager() = default;
    ~SwapChainManager() = default;

    void createSurface(VkInstance instance, GLFWwindow *window);
    void destroySurface();
    VkSurfaceKHR GetSurface() const;

    void createSwapChain(VkPhysicalDevice device, VkDevice logicalDevice, GLFWwindow *window, const SwapChainConfig &config, const QueueFamilyIndices &indices);
    void reCreateSwapChain(VkDevice device, VkPhysicalDevice physDevice, GLFWwindow *window, VkRenderPass renderPass, const GBuffers &depthRess, uint32_t indice);

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

    FrameResources &getCurrentFrameData();
    const int getCurrentFrameIndex() const;
    void advanceFrame();
    void createFramesData(VkDevice device, VkPhysicalDevice physDevice, uint32_t queueIndice, uint32_t framesInFlightCount);
    void createFramesSetLayout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding> &layouts);
    void createFramesDynamicRenderingInfo(const RenderTargetConfig &cfg,
                                          const std::vector<VkImageView> &colorViews,
                                          VkImageView depthView);
    // Pass the attachments and used them to create framebuffers
    void createFrameBuffers(VkDevice device, const std::vector<VkImageView> &attachments, VkRenderPass renderPass);

    // Misleading
    // This add before the framebuffer attachments images views of the swapchain then create framebuffers
    void completeFrameBuffers(VkDevice device, const std::vector<VkImageView> &attachments, VkRenderPass renderPass);

    void destroyFramesData(VkDevice device);

private:
    VkSurfaceKHR mSurface = VK_NULL_HANDLE;
    VkSwapchainKHR mSwapChain = VK_NULL_HANDLE;

    // Remove ?
    VkInstance mInstance;
    SwapChainSupportDetails mSupportDetails;
    VkSurfaceFormatKHR mSwapChainImageFormat;
    VkExtent2D mSwapChainExtent;

    std::vector<VkImage> mSwapChainImages;
    std::vector<VkImageView> mSChainImageViews;

    uint32_t currentFrame = 0;
    std::vector<FrameResources> mFramesData;

    void createFrameData(VkDevice device, VkPhysicalDevice physDevice, uint32_t queueIndice);
    void destroyFrameData(VkDevice device);
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