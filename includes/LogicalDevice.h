#pragma once
#include "BaseVk.h"
#include "QueueFam.h"

///////////////////////////////////
//Physical device handling
///////////////////////////////////

/*

Modify QueueFamilyIndices and findQueueFamilies to explicitly look for a queue family with the VK_QUEUE_TRANSFER_BIT bit, but not the VK_QUEUE_GRAPHICS_BIT.
Modify createLogicalDevice to request a handle to the transfer queue
Create a second command pool for command buffers that are submitted on the transfer queue family
Change the sharingMode of resources to be VK_SHARING_MODE_CONCURRENT and specify both the graphics and transfer queue families
Submit any transfer commands like vkCmdCopyBuffer (which we'll be using in this chapter) to the transfer queue instead of the graphics queue
*/
class LogicalDeviceManager {
public:
    LogicalDeviceManager() = default;

    ~LogicalDeviceManager() = default;

    void createLogicalDevice(VkPhysicalDevice physicalDevice, QueueFamilyIndices indices);
    VkDevice getLogicalDevice() const;
    void DestroyDevice();
    VkQueue getGraphicsQueue() const { return mGraphicsQueue; }
    VkQueue getPresentQueue() const { return mPresentQueue; }
    
     VkQueue getComputeQueue() const{  return mComputeQueue;};
VkQueue getTransferQueue() const { return mTransferQueue;};

VkQueue getQueue(uint32_t familyIndex, uint32_t queueIndex = 0) const;
    VkResult submitToGraphicsQueue(
    VkCommandBuffer *,
    VkSemaphore ,
    VkSemaphore ,
    VkFence );

    VkResult presentImage(VkSwapchainKHR, VkSemaphore, uint32_t);
    
private:
    //Copy of instance handle should be fine
    VkDevice device;

//We drawing on the images in the swap chain from the graphics queue and then submitting them on the presentation queue. 
//The way it is handled depend of the sharing mode choosen in swapchain.h
    VkQueue mGraphicsQueue = VK_NULL_HANDLE;
    VkQueue mPresentQueue = VK_NULL_HANDLE ;

    //Dedicated Queue for asynchronicity if needed
    VkQueue mComputeQueue = VK_NULL_HANDLE;
    VkQueue mTransferQueue = VK_NULL_HANDLE;

    //User defined Queue ?

};

