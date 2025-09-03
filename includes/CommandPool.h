// CommandPool.h

#pragma once

#include "BaseVk.h"

#include "QueueFam.h"

// Todo: Capacity to combine multiple buffer into one.
// Todo: Error checking
// Free should be a mode that allow creation of both Frame and Transient
enum class CommandPoolType
{
    Frame,
    Transient,
    Free
};

enum class CmdBufferType {
    Primary,
    Secondary
};

// Design = One Command Pool and Multiple Command Buffer vs Multiples
class CommandPoolManager
{
public:
    CommandPoolManager() = default;

    // QueueFamilyIndices queueFamilyIndices
    void createCommandPool(VkDevice device, CommandPoolType type, uint32_t familyIndex); 

    void createCommandBuffers(size_t nbBuffers);

    void resetCommandBuffer(int index = 0) const;

    void resetCommandPool() const;

    void destroyCommandPool();

    // Only available for transient

    void beginRecord( uint32_t index = 0,VkCommandBufferUsageFlags flags = 0,
        CmdBufferType bufferType = CmdBufferType::Primary,
        VkCommandBufferInheritanceInfo* inheritance = nullptr);

    void endRecord(uint32_t index = 0);

    // Transient helpers
    VkCommandBuffer beginSingleTime();

    void endSingleTime(VkCommandBuffer cmd, VkQueue queue);

    VkCommandBuffer createTransientBuffer();

    // This does not check if the buffeer was actually allocated hree
    void freeBuffer(VkCommandBuffer commandBuffer);
    VkCommandBuffer get(int index = 0) const ;
    VkCommandBuffer *getCmdBufferHandle(int index = 0) ;

private:
    VkDevice mDevice = VK_NULL_HANDLE;
    VkCommandPool mCmdPool = VK_NULL_HANDLE;
    CommandPoolType mType;
    VkCommandPool mPool;
    // Multiples buffer to accomadate multiples flames in flight.
    //  THe cpu prepare two command while the gpu is rendering
    std::vector<VkCommandBuffer> mCmdBuffers;
};

/*
Life cycle ?
vkAllocateCommandBuffers -> handle exists
vkBeginCommandBuffer -> recording allocates internal storage
... record commands ...
vkEndCommandBuffer -> recording finalized
vkQueueSubmit -> buffer executed
vkResetCommandBuffer(..., RELEASE_RESOURCES) -> handle valid, storage freed
vkBeginCommandBuffer -> storage reallocated internally, ready to record again (also stand for a reset but at 0)




//Record flags
  /*Flags
    VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT = 0x00000001,
    VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT = 0x00000002,
    VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT = 0x00000004,
    VK_COMMAND_BUFFER_USAGE_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF


    VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: The command buffer will be rerecorded right after executing it once.
    VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: This is a secondary command buffer that will be entirely within a single render pass.
    VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: The command buffer can be resubmitted while it is also already executed on a device (avoid)

*/