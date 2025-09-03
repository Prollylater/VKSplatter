#pragma once
#include "BaseVk.h"
#include <glm/glm.hpp>
#include <array>
#include <unordered_map>
#include "VertexDescriptions.h"
#include "LogicalDevice.h"
#include "PhysicalDevice.h"

#include "CommandPool.h"

// Notes: Rework the class vision

struct GPUBufferView
{
    VkBuffer mBuffer = VK_NULL_HANDLE;
    VkDeviceSize mOffset = 0;
    // This behave so far as a count
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
        Attributes,
        /*Pos, Normal, UV, Color,*/ Index
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
                      uint32_t queueIndice);

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

    VkBuffer getBuffer() const { return mBuffer; }
    VkDeviceMemory getMemory() const { return mMemory; }
    VkDeviceSize getSize() const { return mSize; }

    // Todo: Should this be directly returned from an upload ?
    GPUBufferView getView(VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);
    VkBufferView createBufferView(VkFormat format, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

    // Very sloppy for now shoudl a descriptor instead of M
    VkDescriptorBufferInfo getDescriptor() const
    {
        return {mBuffer, 0, mSize};
    }

private:
    VkDevice mDevice = VK_NULL_HANDLE;
    VkBuffer mBuffer = VK_NULL_HANDLE;
    VkDeviceMemory mMemory = VK_NULL_HANDLE;

    // Find a better way to set it
    VkDeviceSize mSize = 0;

public:
    // Todo: Helper to move out
    void createVertexIndexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const std::vector<Mesh> &mesh,
                                  const LogicalDeviceManager &deviceM, uint32_t indice);

    void createVertexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh,
                             const LogicalDeviceManager &deviceM, uint32_t indice);
    void createIndexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh,
                            const LogicalDeviceManager &deviceM, uint32_t indice);
};

// TOOD: Change eother at VK_NULL_HANDLE
//  Destroy on not real object nor null handle can have weird conseuqeujces

// Additions
// Not sure how much use we have of this but
// But since we use exclusive sharing mode by default

namespace vkUtils
{
    namespace BufferHelper
    {
        // Buffer View can be recreated even after Buffer has been deleted
        inline VkBufferView createBufferView(VkDevice device, VkBuffer buffer, VkFormat format, VkDeviceSize offset, VkDeviceSize size = VK_WHOLE_SIZE);
        ///////////////////////////////////////////////////////////////////
        ////////////////////////Creation utility///////////////////////////
        ///////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////
        ////////////////////////Memory Barrier utility///////////////////////////
        ///////////////////////////////////////////////////////////////////

        struct BufferTransition
        {
            VkBuffer buffer = VK_NULL_HANDLE;
            VkAccessFlags srcAccessMask = 0;
            VkAccessFlags dstAccessMask = 0;
            VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            VkDeviceSize offset = 0;
            VkDeviceSize size = VK_WHOLE_SIZE;
        };

        // Rename to insert memory Barrier
        void transitionBuffer(
            const BufferTransition &transitionObject,
            const LogicalDeviceManager &deviceM,
            uint32_t indice);

        /////////////////////////////////////////////////
        /////////////////Recording Utility///////////////
        /////////////////////////////////////////////////

        inline void recordCopy(VkCommandBuffer cmdBuffer,
                               VkBuffer srcBuffer,
                               VkBuffer dstBuffer,
                               VkDeviceSize size,
                               VkDeviceSize srcOffset,
                               VkDeviceSize dstOffset);

        void copyBufferTransient(VkBuffer srcBuffer, VkBuffer dstBuffer,
                                 VkDeviceSize size,
                                 const LogicalDeviceManager &deviceM,
                                 uint32_t indice,
                                 VkDeviceSize srcOffset = 0,
                                 VkDeviceSize dstOffset = 0);

         void uploadBufferDirect(
            VkDeviceMemory bufferMemory,
            const void *data,
            VkDevice device,
            VkDeviceSize size,
            VkDeviceSize dstOffset);
    }

}
