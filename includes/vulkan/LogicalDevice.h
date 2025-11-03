#pragma once
#include "BaseVk.h"
#include "QueueFam.h"

#include <functional>
///////////////////////////////////
// Physical device handling
///////////////////////////////////
class CommandPoolManager;
/*

Modify QueueFamilyIndices and findQueueFamilies to explicitly look for a queue family with the VK_QUEUE_TRANSFER_BIT bit, but not the VK_QUEUE_GRAPHICS_BIT.
Modify createLogicalDevice to request a handle to the transfer queue
Create a second command pool for command buffers that are submitted on the transfer queue family
Change the sharingMode of resources to be VK_SHARING_MODE_CONCURRENT and specify both the graphics and transfer queue families
Submit any transfer commands like vkCmdCopyBuffer (which we'll be using in this chapter) to the transfer queue instead of the graphics queue
*/
class LogicalDeviceManager
{
public:
    LogicalDeviceManager() = default;

    ~LogicalDeviceManager() = default;

    void createLogicalDevice(VkPhysicalDevice physicalDevice, QueueFamilyIndices indices, const std::vector<const char *> &validationLayers, const DeviceSelectionCriteria &criteria);
    VkDevice getLogicalDevice() const;
    void createVmaAllocator(VkPhysicalDevice physicalDevice, VkInstance instance);
    VmaAllocator getVmaAllocator() const;
    void destroyVmaAllocator();

    void DestroyDevice();

    VkQueue getGraphicsQueue() const { return mGraphicsQueue; }
    VkQueue getPresentQueue() const { return mPresentQueue; }

    VkQueue getComputeQueue() const { return mComputeQueue; };
    VkQueue getTransferQueue() const { return mTransferQueue; };
    VkQueue getQueue(uint32_t familyIndex, uint32_t queueIndex = 0) const;

    // Todo: Move higher to renderer or so on
    VkResult immediateSubmit(CommandPoolManager &poolCmd,
                             std::function<void(VkCommandBuffer)> recordFunction,
                             VkQueue queue,
                             VkSemaphore waitSemaphore,
                             VkSemaphore signalSemaphore,
                             VkFence fence);

    VkResult submitFrameToGQueue(
        VkCommandBuffer cmdBuffers,
        VkSemaphore waitSemaphores,
        VkSemaphore signalSemaphores,
        VkFence fence);

    VkResult presentImage(VkSwapchainKHR, VkSemaphore, uint32_t);

    VkResult submitToQueue(
        VkQueue queue,
        const std::vector<VkCommandBuffer> &cmdBuffers,
        const std::vector<VkSemaphore> &waitSemaphores,
        const std::vector<VkPipelineStageFlags> &waitStages,
        const std::vector<VkSemaphore> &signalSemaphores,
        VkFence fence = VK_NULL_HANDLE);

    VkResult submit2ToQueue(
        VkQueue queue,
        const std::vector<VkCommandBuffer> &cmdBuffers,
        const std::vector<VkSemaphore> &waitSemaphores,
        const std::vector<VkPipelineStageFlags> &waitStages,
        const std::vector<VkSemaphore> &signalSemaphores,
        VkFence fence);
        
    VkResult waitForQueueIdle(VkQueue queue)
    {
        return vkQueueWaitIdle(queue);
    }

    VkResult waitIdle()
    {
        return vkDeviceWaitIdle(device);
    }

private:
    // Copy of instance handle should be fine
    VkDevice device;

    // We drawing on the images in the swap chain from the graphics queue and then submitting them on the presentation queue.
    // The way it is handled depend of the sharing mode choosen in swapchain.h
    VkQueue mGraphicsQueue = VK_NULL_HANDLE;
    VkQueue mPresentQueue = VK_NULL_HANDLE;

    // Dedicated Queue for asynchronicity if needed
    VkQueue mComputeQueue = VK_NULL_HANDLE;
    VkQueue mTransferQueue = VK_NULL_HANDLE;

    VmaAllocator mVmaAllocator;

    // User defined Queue ?
};
