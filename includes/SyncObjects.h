/*
SyncObjects

    Wraps synchronization primitives:

        Semaphores

        Fences

        Per-frame sync objects
Fence and Semaphore
Uo use semaphores for swapchain operations because they happen on the GPU.
For waiting on the previous frame to finish, we want to use fences.
This is so we don't draw more than one frame at a time. 
Because we re-record the command buffer every frame, we cannot record the next
 frame's work to the command buffer until the current frame has finished executing, 
 as we don't want to overwrite the current contents of the command buffer while the GPU is using it.
*/

#pragma once
#include "BaseVk.h"
class SyncObjects {
public:
    SyncObjects() = default;
    ~SyncObjects()= default;
    
    void createSyncObjects(VkDevice device, uint32_t maxFramesInFlight);
    void destroy(VkDevice, uint32_t maxFramesInFlight);
    // Called every frame to get the current sync primitives
    VkSemaphore getImageAvailableSemaphore(uint32_t frameIndex) const;
    VkSemaphore getRenderFinishedSemaphore(uint32_t frameIndex) const;
    VkFence getInFlightFence(uint32_t frameIndex) const;

    // Wait for current frame to finish
    void waitForFence(VkDevice, uint32_t frameIndex) const;

    // Reset fence before using it again
    void resetFence(VkDevice, uint32_t frameIndex) const;

private:
    VkDevice mDevice;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

 
};


