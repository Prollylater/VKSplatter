#include "SyncObjects.h"

void FrameSyncObjects::createSyncObjects(VkDevice device)
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // Start signaled so we don't wait on first use
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create synchronization objects for a frame!");
    }
}

void FrameSyncObjects::destroy(VkDevice device)
{
    if (imageAvailableSemaphores != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(device, imageAvailableSemaphores, nullptr);
        imageAvailableSemaphores = VK_NULL_HANDLE;
    }
    if (renderFinishedSemaphores != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(device, renderFinishedSemaphores, nullptr);
        renderFinishedSemaphores = VK_NULL_HANDLE;
    }
    if (inFlightFences != VK_NULL_HANDLE)
    {
        vkDestroyFence(device, inFlightFences, nullptr);
        inFlightFences = VK_NULL_HANDLE;
    }
}

VkSemaphore FrameSyncObjects::getImageAvailableSemaphore() const
{
    return imageAvailableSemaphores;
}

VkSemaphore FrameSyncObjects::getRenderFinishedSemaphore() const
{
    return renderFinishedSemaphores;
}

VkFence FrameSyncObjects::getInFlightFence() const
{
    return inFlightFences;
}

void FrameSyncObjects::waitFenceSignal(VkDevice device) const
{
    uint64_t timeout = UINT64_MAX;
    VkResult result = vkWaitForFences(device, 1, &inFlightFences, VK_TRUE, timeout);
    if (VK_SUCCESS != result)
    {
        throw std::runtime_error("Waiting on fence failed!");
    }
}

void FrameSyncObjects::resetFence(VkDevice device) const
{
    VkResult result = vkResetFences(device, 1, &inFlightFences);
    if (VK_SUCCESS != result)
    {
        throw std::runtime_error("Reseting fence failed!");
    }
}


/*
syncObjects.waitForFence(currentFrame);
syncObjects.resetFence(currentFrame);

// Submit work to graphics queue using semaphores
VkSubmitInfo submitInfo = {};
submitInfo.waitSemaphoreCount = 1;
submitInfo.pWaitSemaphores = &syncObjects.getImageAvailableSemaphore(currentFrame);
// ...

vkQueueSubmit(graphicsQueue, 1, &submitInfo, syncObjects.getInFlightFence(currentFrame));

*/