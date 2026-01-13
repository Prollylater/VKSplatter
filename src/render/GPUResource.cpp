
#include "GPUResource.h"

void MaterialGPU::destroy(VkDevice device, VmaAllocator allocator)
{
    if (allocator)
    {
        vmaDestroyBuffer(allocator, uniformBuffer, uniformBufferAlloc);
    }
    else
    {
        vkFreeMemory(device, uniformBufferMem, nullptr);
        vkDestroyBuffer(device, uniformBuffer, nullptr);
    }
    uniformBufferAlloc = VK_NULL_HANDLE;
    uniformBuffer = VK_NULL_HANDLE;
    uniformBufferMem = VK_NULL_HANDLE;
}

void MeshGPU::destroy(VkDevice device, VmaAllocator allocator)
{
    if (allocator)
    {
        vmaDestroyBuffer(allocator, vertexBuffer, vertexAlloc);
        vmaDestroyBuffer(allocator, indexBuffer, indexAlloc);
    }
    else
    {
        vkFreeMemory(device, vertexMem, nullptr);
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, indexMem, nullptr);
        vkDestroyBuffer(device, indexBuffer, nullptr);
    }
    vertexBuffer = VK_NULL_HANDLE;
    indexBuffer = VK_NULL_HANDLE;
    vertexMem = VK_NULL_HANDLE;
    indexMem = VK_NULL_HANDLE;
    vertexAlloc = VK_NULL_HANDLE;
    indexAlloc = VK_NULL_HANDLE;
}

// The who is allowed to destroy Mesh GPU Ressources is still a pending question
// Deletion QUEUE neeeded ?
void InstanceGPU::destroy(VkDevice device, VmaAllocator allocator)
{
    if (allocator)
    {
        vmaDestroyBuffer(allocator, instanceBuffer, instanceAlloc);
    }
    else
    {
        vkFreeMemory(device, instanceMem, nullptr);
        vkDestroyBuffer(device, instanceBuffer, nullptr);
    }
    instanceBuffer = VK_NULL_HANDLE;
    instanceAlloc = VK_NULL_HANDLE;
    instanceMem = VK_NULL_HANDLE;
    mapped = nullptr;
}
