
#pragma once
#include "BaseVk.h"
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

/////Todo: For "adhoc semaphore", in passes and so on Fence are not added yet
/*
class SyncObjectManager
{
public:
    SyncObjectManager(VkDevice device);
    ~SyncObjectManager();

    VkSemaphore acquire();
    void release(VkSemaphore semaphore);

private:
    VkDevice mDevice;
    std::vector<VkSemaphore> mPool;  // available semaphores
    std::vector<VkSemaphore> mInUse; // for debugging/validation

    /*
    #ifndef NDEBUG
    std::vector<VkSemaphore> mInUse;
    #endif
    * /
};

SyncObjectManager::SyncObjectManager(VkDevice device)
    : mDevice(device) {}

SyncObjectManager::~SyncObjectManager()
{
    for (auto sem : mSemaphorePool)
    {
        vkDestroySemaphore(mDevice, sem, nullptr);
    }
    for (auto sem : mInUse)
    {
        vkDestroyFence(mDevice, sem, nullptr);
    }
}


VkSemaphore SyncObjectManager::acquire()
{
    VkSemaphore sem;

    if (!mPool.empty())
    {
        sem = mPool.back();
        mPool.pop_back();
    }
    else
    {
        // Create new one if none available
        VkSemaphoreCreateInfo info{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        if (vkCreateSemaphore(mDevice, &info, nullptr, &sem) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create semaphore");
        }
    }

    mInUse.push_back(sem);
    return sem;
}

void SyncObjectManager::release(VkSemaphore sem)
{
    /*auto it = std::find(mInUse.begin(), mInUse.end(), sem);
    if (it != mInUse.end())
    {
        mPool.push_back(sem);
        mInUse.erase(it);
    }* /
}


VkSemaphore FrameSyncObjects::acquireSemaphore(VkDevice device) {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
        }

}

VkFence FrameSyncObjects::acquireFence(VkDevice device, VkFenceCreateFlags flag = 0) {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = flag;
    //fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
        }

}



VkSemaphore FrameSyncObjects::acquireSemaphore(uint32_t index) const {
    return mSemaphorePool[frameIndex];
}

VkFence FrameSyncObjects::acquireFence(uint32_t frameIndex) const {
    return mFencePool[frameIndex];
}

void FrameSyncObjects::waitForFence(VkDevice device, uint32_t frameIndex) const {
    vkWaitForFences(device, 1, &mFencePool[frameIndex], VK_TRUE, UINT64_MAX);
}

void FrameSyncObjects::resetFence(VkDevice device, uint32_t frameIndex) const {
    vkResetFences(device, 1, &mFencePool[frameIndex]);
}

*/