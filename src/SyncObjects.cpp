#include "SyncObjects.h"


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