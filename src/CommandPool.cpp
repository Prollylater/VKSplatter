#include "CommandPool.h"


//Todo: One Pool per family ?  Check in real usage
//Some suggest one Command Pool per frame in flight
/*void CommandPoolManager::createCommandPool(VkDevice device, QueueFamilyIndices queueFamilyIndex)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndex.graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &mCmdPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }

    mDevice = device;
}


VkCommandPool CommandPoolManager::createSubCmdPool(VkDevice device, QueueFamilyIndices queueFamilyIndex, VkCommandPoolCreateFlags flags)const
{
    if( device != VK_NULL_HANDLE){
    VkCommandPool cmdPool;
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = flags;
    poolInfo.queueFamilyIndex = queueFamilyIndex.graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &cmdPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }
    return  cmdPool;
    }
    return VK_NULL_HANDLE;

}

void CommandPoolManager::destroy(VkDevice device) {
    if (mCmdPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, mCmdPool, nullptr);
        //Also wipe the buffers
    }
}

void CommandPoolManager::reset(VkDevice device) const {
    vkResetCommandPool(device, mCmdPool, 0);
}



// CommandBuffer.cpp

void CommandBuffer::createCommandBuffers(VkDevice device, VkCommandPool commandPool, int nbBuffers)
{
    mCmdBuffer.resize(nbBuffers);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    //A short lived command Pool for this ?
    allocInfo.commandPool = commandPool;
    // Todo: Reread about it
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; 
    allocInfo.commandBufferCount = static_cast<uint32_t>(mCmdBuffer.size());

    if (vkAllocateCommandBuffers(device, &allocInfo, mCmdBuffer.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffer!");
    }

}

void CommandBuffer::beginRecord(VkCommandBufferUsageFlags flags = 0, uint32_t index = 0) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = flags;// SHould be flags but ignroed for now
    beginInfo.pInheritanceInfo = nullptr ;

    if (vkBeginCommandBuffer(mCmdBuffer[index], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }
}

void CommandBuffer::endRecord(uint32_t index) {
    if (vkEndCommandBuffer(mCmdBuffer[index]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to record command buffer!");
    }

    /*
    THis always precede this

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    * /
}
*/





/*


## 🔹 Option B — **One Command Pool per Frame-in-Flight**

* Each frame has its own pool.
* All command buffers used by that frame come from its pool.
* On frame recycle: just `vkResetCommandPool(poolForThisFrame)`.

**Pros:**

* Super clean lifecycle: when frame N is finished, reset its pool → all its command buffers are valid for reuse.
* No risk of accidentally resetting buffers still in use by GPU (since per-frame fences protect them).
* Cleaner when you add more per-frame data (descriptors, UBOs, etc.), since it all ties to frame index.
* Easier to isolate debug/profiling issues (per-frame state is self-contained).

**Cons:**

* Slightly more pools to manage (but negligible overhead).
* You need to track the pool associated with the current frame index.

---

```cpp
struct FrameResources {
    VkCommandPool cmdPool;
    VkCommandBuffer primaryCmd;
    std::vector<VkCommandBuffer> secondaryCmds;
    VkFence inFlightFence;
    VkSemaphore imageAvailable;
    VkSemaphore renderFinished;
    // maybe per-frame UBOs too
};

*/