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

#pragma once
#include "BaseVk.h"
class FrameSyncObjects
{
public:
    FrameSyncObjects() = default;
    ~FrameSyncObjects() = default;

    //Todo: Important
    //MaxFrames iN Flight should disappear
    void createSyncObjects(VkDevice device);
    void destroy(VkDevice);
    // Called every frame to get the current sync primitives
    VkSemaphore getImageAvailableSemaphore() const;
    VkSemaphore getRenderFinishedSemaphore() const;
    VkFence getInFlightFence() const;

    // Wait for current frame to finish
    void waitForFence(VkDevice ) const;

    // Reset fence before using it again
    void resetFence(VkDevice) const;

private:
    VkDevice mDevice;
    VkSemaphore imageAvailableSemaphores;
    VkSemaphore renderFinishedSemaphores;
    VkFence inFlightFences;
};

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
    for (auto sem : mPool)
    {
        vkDestroySemaphore(mDevice, sem, nullptr);
    }
    for (auto sem : mInUse)
    {
        vkDestroySemaphore(mDevice, sem, nullptr);
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


void FrameSyncObjects::createSyncObjects(VkDevice device, uint32_t maxFramesInFlight) {
    imageAvailableSemaphores.resize(maxFramesInFlight);
    renderFinishedSemaphores.resize(maxFramesInFlight);
    inFlightFences.resize(maxFramesInFlight);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // Start signaled so we don't wait on first use
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; 

    for (size_t i = 0; i < maxFramesInFlight; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create synchronization objects for a frame!");
        } 
    }

}


void FrameSyncObjects::destroy(VkDevice device, uint32_t maxFramesInFlight) {
    for (size_t i = 0; i < maxFramesInFlight; i++) {
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }
    imageAvailableSemaphores.clear();
    renderFinishedSemaphores.clear();
    inFlightFences.clear();

}

VkSemaphore FrameSyncObjects::getImageAvailableSemaphore(uint32_t frameIndex) const {
    return imageAvailableSemaphores[frameIndex];
}

VkSemaphore FrameSyncObjects::getRenderFinishedSemaphore(uint32_t frameIndex) const {
    return renderFinishedSemaphores[frameIndex];
}

VkFence FrameSyncObjects::getInFlightFence(uint32_t frameIndex) const {
    return inFlightFences[frameIndex];
}

void FrameSyncObjects::waitForFence(VkDevice device, uint32_t frameIndex) const {
    vkWaitForFences(device, 1, &inFlightFences[frameIndex], VK_TRUE, UINT64_MAX);
}

void FrameSyncObjects::resetFence(VkDevice device, uint32_t frameIndex) const {
    vkResetFences(device, 1, &inFlightFences[frameIndex]);
}

*/