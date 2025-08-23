#pragma once
#include "BaseVk.h"
#include <glm/glm.hpp>
#include <array>
#include <unordered_map>
#include "VertexDescriptions.h"
#include "LogicalDevice.h"
#include "PhysicalDevice.h"

#include "CommandPool.h"


struct MeshBufferInfo {
    VkDeviceSize vertexOffset;  // in bytes
    VkDeviceSize indexOffset;   // in bytes
    uint32_t indexCount;        // for vkCmdDrawIndexed()
    uint32_t vertexCount;

    bool shared = false;
    bool interleavec = true;
    
};

///////////////////////////////////
// Buffer
///////////////////////////////////
        void createBuffer(VkBuffer &buffer, VkDeviceMemory &bufferMemory,const VkDevice &device, const VkPhysicalDevice &physDevice, VkDeviceSize data,
                      VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

    uint32_t findMemoryType(const VkPhysicalDevice &device, uint32_t memoryTypeBitsRequirement, const VkMemoryPropertyFlags &requiredProperties);


//A class for holding a buffer (principally for Mesh)
class Buffer
{
public:
    Buffer() = default;
    ~Buffer() = default;

    // void createVertexBuffer();

    //  void createVertexBuffers(const VkDevice& device, const VkPhysicalDevice& physDevice,const Mesh& mesh, const VertexFormat& format);

    
    void createVertexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh, 
                             const LogicalDeviceManager &deviceM,  const QueueFamilyIndices indice);

    void createVertexIndexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const std::vector<Mesh> &mesh,
    const LogicalDeviceManager& deviceM, const QueueFamilyIndices indice );

    void createIndexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh, 
                             const LogicalDeviceManager &deviceM, const QueueFamilyIndices indice);

    void destroyVertexBuffer(VkDevice device);
    void destroyIndexBuffer(VkDevice device);


    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, const LogicalDeviceManager &deviceM, const QueueFamilyIndices &indices);

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

    MeshBufferInfo mBufferInfo;



    //More highlevel method
    void uploadMesh(const Mesh& mesh, const LogicalDeviceManager &logDevM, const PhysicalDeviceManager &physDevM) {

    // Some way to know the buffer states
    const VkPhysicalDevice &physDevice = physDevM.getPhysicalDevice();

    const VkDevice &device = logDevM.getLogicalDevice();

     createVertexBuffers(device, physDevice, mesh, logDevM,  physDevM.getIndices());
     createIndexBuffers(device, physDevice, mesh, logDevM,  physDevM.getIndices());

    };

private:
    VkBuffer mVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory mVertexBufferMemory = VK_NULL_HANDLE;

    VkBuffer mIndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory mIndexBufferMemory = VK_NULL_HANDLE;
};

//TOOD: Change eother at VK_NULL_HANDLE
// Destroy on not real object nor null handle can have weird conseuqeujces


