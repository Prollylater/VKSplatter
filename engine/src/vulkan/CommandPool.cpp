#include "CommandPool.h"
#include <stdexcept>
void CommandPoolManager::createCommandPool(VkDevice device, CommandPoolType type, uint32_t familyIndex)
{
    mDevice = device;
    mType = type;

    VkCommandPoolCreateInfo info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    info.queueFamilyIndex = familyIndex;

    //https://docs.vulkan.org/refpages/latest/refpages/source/VkCommandPoolCreateFlagBits.html
    if (type == CommandPoolType::Transient)
    {
        info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    }
    else
    {
        // Allow individual reset of command buffer created (explictly?? and implicitly with begin recording)
        info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    }

    if (vkCreateCommandPool(mDevice, &info, nullptr, &mPool) != VK_SUCCESS)
        throw std::runtime_error("Failed to create command pool");
}

// Toodoo:: Add secondary
void CommandPoolManager::createCommandBuffers(size_t nbBuffers)
{
    // Allocate primary buffers if this is a frame pool or free
    if (mType == CommandPoolType::Frame || mType == CommandPoolType::Free)
    {
        mCmdBuffers.resize(nbBuffers);
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = mPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(mCmdBuffers.size());

        if (vkAllocateCommandBuffers(mDevice, &allocInfo, mCmdBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate primary command buffers");
    }
}

void CommandPoolManager::resetCommandBuffer(int index) const
{
    // VK_COMMAND_BUFFER_RESET_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
    // Explicit individual reset

    bool releaseRessources = false;
    VkCommandBufferResetFlags releaseFlag = VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT;

    // 0 let the command buffer keep the allocated ressources (drive ressources and cache stuff ?)
    // else, we need to reallocatememory. IT is different from free

    if (vkResetCommandBuffer(mCmdBuffers.at(index), releaseRessources ? releaseFlag : 0))
    {
        throw std::runtime_error("Failed to reset command buffer");
    }
}

void CommandPoolManager::resetCommandPool() const
{
    // Explicit reset
    bool releaseRessources = false;
    VkCommandPoolResetFlags releaseFlag = VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT;
    if (vkResetCommandPool(mDevice, mCmdPool, releaseRessources ? releaseFlag : 0))
    {
        throw std::runtime_error("Failed to reset pool");
    }
}

void CommandPoolManager::destroyCommandPool()
{
    // Should vkWaitOn the creation queue here ?
    if (!mCmdBuffers.empty())
    {
        vkFreeCommandBuffers(mDevice, mPool, static_cast<uint32_t>(mCmdBuffers.size()), mCmdBuffers.data());
    }

    if (mPool != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(mDevice, mPool, nullptr);
    }
}

// Notes Implictly reset command buffer
void CommandPoolManager::beginRecord(uint32_t index, VkCommandBufferUsageFlags flags,
                                     CmdBufferType bufferType,
                                     VkCommandBufferInheritanceInfo *inheritance)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = flags;

    // Todo: Look into secondary command buffer as nullptr lock it into onnly primary
    // Same thing for level at creation to make this more flexible
    // VkCommandBufferInheritanceInfo * secondaryCommandBufferInfo;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(mCmdBuffers[index], &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to begin recording command buffer!");
    }
}

void CommandPoolManager::endRecord(uint32_t index)
{
    if (vkEndCommandBuffer(mCmdBuffers[index]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to record command buffer!");
    }
}


VkCommandBufferSubmitInfo CommandPoolManager::getCmdSubmitInfo(int bufferIndex, uint32_t deviceMask)
{
    VkCommandBufferSubmitInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    info.pNext = nullptr;
    info.commandBuffer = mCmdBuffers[bufferIndex];
    info.deviceMask = deviceMask;

    return info;
}

// Transient helpers
VkCommandBuffer CommandPoolManager::beginSingleTime()
{
    if (mType != CommandPoolType::Transient && mType != CommandPoolType::Free)
    {
        throw std::runtime_error("beginSingleTime not allowed for Frame pool");
    };

    // Todo: WHy not just store it inside ?
    // This defeat the point of having it here
    VkCommandBuffer cmd = createTransientBuffer();
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);
    return cmd;
}

void CommandPoolManager::endSingleTime(VkCommandBuffer cmd, VkQueue queue)
{
    vkEndCommandBuffer(cmd);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(mDevice, mPool, 1, &cmd);
}

VkCommandBuffer CommandPoolManager::createTransientBuffer()
{
    VkCommandBuffer cmd;
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = mPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    if (vkAllocateCommandBuffers(mDevice, &allocInfo, &cmd) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate primary command buffers");
    }
    return cmd;
}

// This does not check if the buffeer was actually allocated hree
void CommandPoolManager::freeBuffer(VkCommandBuffer commandBuffer)
{
    vkFreeCommandBuffers(mDevice, mCmdPool, 1, &commandBuffer);
}

VkCommandBuffer CommandPoolManager::get(int index) const { return mCmdBuffers[index]; }
VkCommandBuffer *CommandPoolManager::getCmdBufferHandle(int index) { return &(mCmdBuffers[index]); }
