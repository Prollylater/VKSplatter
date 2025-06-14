#pragma once
#include "BaseVk.h"
#include <glm/glm.hpp>
#include <array>
#include <unordered_map>
#include "VertexDescriptions.h"
#include "LogicalDevice.h"
#include "CommandPool.h"
///////////////////////////////////
// Buffer
///////////////////////////////////
        void createBuffer(VkBuffer &buffer, VkDeviceMemory &bufferMemory,const VkDevice &device, const VkPhysicalDevice &physDevice, VkDeviceSize data,
                      VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

    uint32_t findMemoryType(const VkPhysicalDevice &device, uint32_t memoryTypeBitsRequirement, const VkMemoryPropertyFlags &requiredProperties);

class Buffer
{
public:
    Buffer() = default;
    ~Buffer() = default;

    // void createVertexBuffer();

    //  void createVertexBuffers(const VkDevice& device, const VkPhysicalDevice& physDevice,const Mesh& mesh, const VertexFormat& format);

    void createVertexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh, 
                             const LogicalDeviceManager &deviceM, const CommandPoolManager &cmdPoolM, const QueueFamilyIndices indice);

    void createVertexIndexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh,
    const LogicalDeviceManager& deviceM, const CommandPoolManager& cmdPoolM, const QueueFamilyIndices indice );

    void createIndexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh, 
                             const LogicalDeviceManager &deviceM, const CommandPoolManager &cmdPoolM, const QueueFamilyIndices indice);

    void destroyVertexBuffer(VkDevice device);
    void destroyIndexBuffer(VkDevice device);


    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, const LogicalDeviceManager &deviceM, const CommandPoolManager &cmdPoolM,
                    const QueueFamilyIndices &indices);

    const VkBuffer getVBuffer() const
    {
        return mVertexBuffer;
    }

    const VkDeviceMemory getVBufferMemory() const
    {
        return mVertexBufferMemory;
    }



    const VkBuffer getIDXBuffer() const
    {
        return mIndexBuffer;
    }

    const VkDeviceMemory getIDXBufferMemory() const
    {
        return mIndexBufferMemory;
    }


private:
    VkBuffer mVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory mVertexBufferMemory = VK_NULL_HANDLE;

    VkBuffer mIndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory mIndexBufferMemory = VK_NULL_HANDLE;
};

//TOOD: Chang eother at VK_NULL_HANDLE
// Destroy on not real object nor null handle can have weird conseuqeujces