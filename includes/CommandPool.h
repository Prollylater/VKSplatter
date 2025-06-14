// CommandPool.h

#pragma once

#include "BaseVk.h"

class CommandPoolManager {
public:
    CommandPoolManager() = default;
    ~CommandPoolManager() = default;

    void create(VkDevice device, QueueFamilyIndices queueFamilyIndices );
    VkCommandPool createSubCmdPool(VkDevice device, QueueFamilyIndices queueFamilyIndex, VkCommandPoolCreateFlags flags) const;

    void destroy(VkDevice device);

    VkCommandPool get() const { return mCmdPool; }

    void reset(VkDevice) const;

private:
    VkDevice mDevice = VK_NULL_HANDLE;
    VkCommandPool mCmdPool = VK_NULL_HANDLE;
};



class CommandBuffer {
public:
    CommandBuffer() = default;
    ~CommandBuffer() = default;


    void createCommandBuffers(VkDevice device, VkCommandPool mCmdPool, int nbBuffers = MAX_FRAMES_IN_FLIGHT);
    
    VkCommandBuffer get(int index) const { return mCmdBuffer[index]; }
    VkCommandBuffer *getCmdBufferHandle(int index) { return &(mCmdBuffer[index]); }
    

    void beginRecord(VkCommandBufferUsageFlags flags ,uint32_t index);
    void endRecord(uint32_t index);

private:
    VkDevice device;
    VkCommandPool mCmdPool;
    //Multiples bugger to accomadate multiples flames in flight.
    // THe cpu prepare two command while the gpu is rendering
    std::vector<VkCommandBuffer> mCmdBuffer;
};


