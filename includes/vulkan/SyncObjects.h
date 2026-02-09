
#pragma once
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan.h>


class FrameSyncObjects
{
public:
    FrameSyncObjects() = default;
    ~FrameSyncObjects() = default;

    void createSyncObjects(VkDevice device);
    void destroy(VkDevice);
    // Called every frame to get the current sync primitives
    VkSemaphore getImageAvailableSemaphore() const;
    VkSemaphore getRenderFinishedSemaphore() const;
    VkFence getInFlightFence() const;

    // Wait for current frame to finish
    void waitFenceSignal(VkDevice) const;

    // Reset fence to unsignaled state
    void resetFence(VkDevice) const;

private:
    VkDevice mDevice;
    VkSemaphore imageAvailableSemaphores;
    VkSemaphore renderFinishedSemaphores;
    VkFence inFlightFences;
};

namespace vkUtils
{
    namespace SyncObj
    {
        // Todo:https://vkguide.dev/docs/new_chapter_1/vulkan_mainloop_code/
        inline VkSemaphoreSubmitInfo getSumbitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore)
        {
            VkSemaphoreSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
            submitInfo.pNext = nullptr;
            submitInfo.semaphore = semaphore;
            submitInfo.stageMask = stageMask;
            submitInfo.deviceIndex = 0;
            submitInfo.value = 1;

            return submitInfo;
        }
    }
}

/*
SyncObjects

    Wraps synchronization primitives:

        Semaphores

        Fences

        Per-frame sync objects
Fence and Semaphore
We use semaphores for swapchain operations because they happen on the GPU.
For waiting on the previous frame to finish, we use fences.
This is so we don't draw more than one frame at a time.

Semaphores are used to synchronize submitted command buffers with each other.
Fences are used to synchronize an application with submitted commands.

 */
