#pragma once
#include "BaseVk.h"
#include <glm/glm.hpp>
#include <array>
#include <unordered_map>
#include "VertexDescriptions.h"
#include "LogicalDevice.h"
#include "PhysicalDevice.h"

#include "CommandPool.h"
#include "RessourcesGPU.h"

// A class for holding a buffer (principally for Mesh)
class Buffer
{
public:
    Buffer() = default;
    ~Buffer() = default;

    void createBuffer(VkDevice device,
                      VkPhysicalDevice physDevice,
                      VkDeviceSize data,
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties,
                      VmaAllocator alloc = VK_NULL_HANDLE);

    void destroyBuffer(VkDevice device, VmaAllocator alloc = VK_NULL_HANDLE);

    void uploadBuffer(const void *data, VkDeviceSize dataSize,
                      VkDeviceSize dstOffset,
                      VkPhysicalDevice physDevice,
                      const LogicalDeviceManager &physDev,
                      uint32_t queueIndice, VmaAllocator allocator);

    void copyToBuffer(VkBuffer dstBuffer,
                      VkDeviceSize size, const LogicalDeviceManager &deviceM,
                      uint32_t indice,
                      VkDeviceSize srcOffset = 0,
                      VkDeviceSize dstOffset = 0);

    void copyFromBuffer(VkBuffer srcBuffer,
                        VkDeviceSize size, const LogicalDeviceManager &deviceM,
                        uint32_t indice,
                        VkDeviceSize srcOffset = 0,
                        VkDeviceSize dstOffset = 0);

    // Todo: Should this be directly returned from an upload ?
    // GPUBufferView getViewGpu(VkDevice device = VK_NULL_HANDLE ,VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);

    VkBufferView createBufferView(VkFormat format, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

    void *map(VmaAllocator allocator);
    void unmap(VmaAllocator allocator);

    VkBuffer getBuffer() const { return mBuffer; }
    VkDeviceMemory getMemory() const { return mMemory; }
    VmaAllocation getVMAMemory() const { return mBufferAllocation; }

    VkDeviceSize getSize() const { return mSize; }

    // Very sloppy for now shoudl a descriptor instead of M
    VkDescriptorBufferInfo getDescriptor() const
    {
        return {mBuffer, 0, mSize};
    }

    VkDeviceAddress getDeviceAdress(VkDevice device) const
    {
        VkBufferDeviceAddressInfo deviceAdressInfo{.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = mBuffer};
        return vkGetBufferDeviceAddress(device, &deviceAdressInfo);
    }

private:
    // Todo: remove mDevice
    VkDevice mDevice = VK_NULL_HANDLE;
    VkBuffer mBuffer = VK_NULL_HANDLE;
    VkDeviceMemory mMemory = VK_NULL_HANDLE;
    VmaAllocation mBufferAllocation = VK_NULL_HANDLE;
    bool useVma = false;
    // Find a better way to set it
    VkDeviceSize mSize = 0;
    // Stride + NB element ?
    void *mMapped = nullptr;

public:
    // Todo: Helper to move out
    void createVertexIndexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const std::vector<Mesh> &mesh,
                                  const LogicalDeviceManager &deviceM, uint32_t indice);

    //Todo: Reduce complexity by removing upload from those fucntion
    //Todo: Reduce complexity by removing upload from those fucntion
    //Todo: Reduce complexity by removing upload from those fucntion
    //Todo: Reduce complexity by removing upload from those fucntion
    void createVertexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh,
                             const LogicalDeviceManager &deviceM, uint32_t queueIndice, VmaAllocator alloc = VK_NULL_HANDLE, bool SSBO = false);
    void createIndexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh,
                            const LogicalDeviceManager &deviceM, uint32_t queueIndice, VmaAllocator alloc = VK_NULL_HANDLE);
};

// TOOD: Change eother at VK_NULL_HANDLE
//  Destroy on not real object nor null handle can have weird conseuqeujces
