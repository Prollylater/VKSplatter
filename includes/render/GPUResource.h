#pragma once
#include "AssetTypes.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

struct Material;

struct MaterialGPU
{
    int pipelineEntryIndex = -1;
    int descriptorIndex = -1;

    // Not allocated
    VkBuffer uniformBuffer = VK_NULL_HANDLE;
    VmaAllocation uniformBufferAlloc = VK_NULL_HANDLE; // Memory handle
    VkDeviceMemory uniformBufferMem = VK_NULL_HANDLE;

    void destroy(VkDevice device, VmaAllocator alloc = VK_NULL_HANDLE);

    struct MaterialGPUCreateInfo
    {
        AssetID<Material> cpuMaterial;
        int descriptorLayoutIdx;
        int pipelineIndex;
    };
};

struct MeshGPU
{
    // Todo: GPUBufferView or BUffer itself would simplify this
    //  Cleanup object in case the mesh buffer is not kept
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VmaAllocation vertexAlloc = VK_NULL_HANDLE;
    VmaAllocation indexAlloc = VK_NULL_HANDLE;
    VkDeviceMemory vertexMem = VK_NULL_HANDLE;
    VkDeviceMemory indexMem = VK_NULL_HANDLE;

    VkDeviceAddress vertexAddress = 0;

    uint32_t indexBufferOffset = 0;
    uint32_t vertexBufferOffset = 0;
    void destroy(VkDevice device, VmaAllocator alloc = VK_NULL_HANDLE);
};

struct InstanceGPU
{
    VkBuffer instanceBuffer = VK_NULL_HANDLE;
    VmaAllocation instanceAlloc = VK_NULL_HANDLE;
    VkDeviceMemory instanceMem = VK_NULL_HANDLE;
    // VkDeviceAddress indexAddress = 0;

    //data used to update InstanceGPU
    uint32_t stride;
    uint32_t capacity;      
    uint32_t count;
    
    void* mapped; //Mainly for test purpose
    
    void destroy(VkDevice device, VmaAllocator alloc = VK_NULL_HANDLE);
};

struct GPUBufferView
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceAddress mVertexBufferAddress = 0;
};