#pragma once
#include "BaseVk.h"
#include <glm/glm.hpp>
#include <array>
#include <unordered_map>
#include "VertexDescriptions.h"
#include "LogicalDevice.h"
#include "PhysicalDevice.h"

#include "CommandPool.h"

struct GPUBufferView
{
    VkBuffer mBuffer = VK_NULL_HANDLE;
    VkDeviceSize mOffset = 0;
    //This behave so far as a count
    VkDeviceSize mSize = 0;
};

struct AttributeStream
{
    GPUBufferView mView;
    uint32_t mStride = 0;
    // Todo:
    // Decide how to handle it in regard to the actual buffer
    // Specifally with the build interleaved and so on
    // This is only really fine due to how we construct our stuff but it render the whole MeshGPuRessources abstraction useless
    // Since the descriptor handle everything well enough
    // Using Attributes just assume everything is neatly deferred to the descriptor
    // Main use so far is for non interleaved and to allow us to drop the Buffer once created
    enum class Type
    {
        Attributes,/*Pos, Normal, UV, Color,*/ Index
    } mType;
};

class MeshGPUResources
{
public:
    void addStream(GPUBufferView view, uint32_t stride, AttributeStream::Type type)
    {
        mStreams.push_back({view, stride, type});
    };

    AttributeStream getStream(uint32_t index)
    {
        return mStreams[index];
    }

    VkBuffer getStreamBuffer(uint32_t index)
    {
        return mStreams[index].mView.mBuffer;
    }

private:
    std::vector<AttributeStream> mStreams;
};

///////////////////////////////////
// Buffer
///////////////////////////////////

uint32_t findMemoryType(const VkPhysicalDevice &device, uint32_t memoryTypeBitsRequirement, const VkMemoryPropertyFlags &requiredProperties);

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
                      VkMemoryPropertyFlags properties);
    void destroyBuffer();

    void uploadBuffer(const void *data, VkDeviceSize dataSize,
                      VkDeviceSize dstOffset,
                      VkPhysicalDevice physDevice,
                      const LogicalDeviceManager &physDev,
                      const QueueFamilyIndices &indices);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, const LogicalDeviceManager &deviceM, const QueueFamilyIndices &indices);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                    VkDeviceSize size, VkDeviceSize srcOffset,
                    VkDeviceSize dstOffset, const LogicalDeviceManager &deviceM, const QueueFamilyIndices &indices);

    // Todo: Should this be directly returned from an upload ?
    GPUBufferView getView(VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);

    VkBuffer getBuffer() const { return mBuffer; }
    VkDeviceMemory getMemory() const { return mMemory; }
    VkDeviceSize getSize() const { return mSize; }

private:
    VkDevice mDevice = VK_NULL_HANDLE;
    VkBuffer mBuffer = VK_NULL_HANDLE;
    VkDeviceMemory mMemory = VK_NULL_HANDLE;

    //Find a better way to set it
    VkDeviceSize mSize = 0;

public:
    void createVertexIndexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const std::vector<Mesh> &mesh,
                                  const LogicalDeviceManager &deviceM, const QueueFamilyIndices indice);

    void createVertexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh,
                             const LogicalDeviceManager &deviceM, const QueueFamilyIndices indice);
    void createIndexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh,
                            const LogicalDeviceManager &deviceM, const QueueFamilyIndices indice);
};

// TOOD: Change eother at VK_NULL_HANDLE
//  Destroy on not real object nor null handle can have weird conseuqeujces
