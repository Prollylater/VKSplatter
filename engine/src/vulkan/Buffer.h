#pragma once
#include "BaseVk.h"
#include "LogicalDevice.h"

struct Mesh;
// A class for holding a buffer
// Notes: This class is fine to use to create a Buffer
// But so far it was dropped either for being not specific enough or too heavy
// Turn this into helper only

enum class BufferUpdatePolicy
{
    Immutable,  // device-local, uploaded once via staging then reject Update
    Dynamic,    // persistently mapped
    StagingOnly // Not CPU-visible, rarely bound directly
};

// Memory flag should be guaranteed by Immutable, Dynamic and Staging only to some degree
struct BufferDesc
{
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    BufferUpdatePolicy updatePolicy;
    VkMemoryPropertyFlags memoryFlags;
    std::string name; 
};

class Buffer
{
public:
    Buffer() = default;
    ~Buffer() = default;

    // Todo:: Default Variable order + The soup should be directly replaced by Buffer Desc
    void createBuffer(VkDevice device,
                      VkPhysicalDevice physDevice,
                      VkDeviceSize data,
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties, VmaAllocator alloc = VK_NULL_HANDLE,
                      BufferUpdatePolicy updatePolicy = BufferUpdatePolicy::StagingOnly);

    void destroyBuffer(VkDevice device, VmaAllocator allocator = VK_NULL_HANDLE);

    void uploadStaged(const void *data, VkDeviceSize dataSize,
                      VkDeviceSize dstOffset,
                      VkPhysicalDevice physDevice,
                      const LogicalDeviceManager &physDev,
                      uint32_t queueIndice, VmaAllocator allocator = VK_NULL_HANDLE);

    void copyToBuffer(VkBuffer dstBuffer,
                      VkDeviceSize size, const LogicalDeviceManager &deviceM,
                      uint32_t queuIndice,
                      VkDeviceSize srcOffset = 0,
                      VkDeviceSize dstOffset = 0);

    void copyFromBuffer(VkBuffer srcBuffer,
                        VkDeviceSize size, const LogicalDeviceManager &deviceM,
                        uint32_t queuIndice,
                        VkDeviceSize srcOffset = 0,
                        VkDeviceSize dstOffset = 0);

    // Todo: Should this be directly returned from an upload ?
    // GPUBufferView getViewGpu(VkDevice device = VK_NULL_HANDLE ,VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);

    VkBufferView createBufferView(VkFormat format, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);

    void *map(VmaAllocator allocator = VK_NULL_HANDLE);
    void unmap(VmaAllocator allocator = VK_NULL_HANDLE);

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
        // Would need to keep the usage flags which is arguably very useful
        return vkGetBufferDeviceAddress(device, &deviceAdressInfo);
    }

    BufferUpdatePolicy getUpdatePolicy() const { return mUpdatePolicy; }

private:
    // Todo: remove mDevice
    VkDevice mDevice = VK_NULL_HANDLE;
    VkBuffer mBuffer = VK_NULL_HANDLE;
    VkDeviceMemory mMemory = VK_NULL_HANDLE;
    VmaAllocation mBufferAllocation = VK_NULL_HANDLE;
    // Find a better way to set it
    VkDeviceSize mSize = 0;
    // Stride + NB element ?
    void *mMapped = nullptr;

    // Wrapper above this ?
    BufferUpdatePolicy mUpdatePolicy = BufferUpdatePolicy::Immutable;

};


/*    void createVertexIndexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const std::vector<Mesh> &mesh,
                                      const LogicalDeviceManager &deviceM, uint32_t indice);

        //Todo: Reduce complexity by removing upload from those fucntion
        void createVertexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh,
                                 const LogicalDeviceManager &deviceM, uint32_t queueIndice, VmaAllocator alloc = VK_NULL_HANDLE, bool SSBO = false);
        void createIndexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh,
                                const LogicalDeviceManager &deviceM, uint32_t queueIndice, VmaAllocator alloc = VK_NULL_HANDLE);

void Buffer::createVertexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh,
                                 const LogicalDeviceManager &deviceM, uint32_t indice, VmaAllocator allocator, bool SSBO)
{
    // const VertexFormat &format = mesh.getFormat();
    const VertexFormat &format = VertexFormatRegistry::getStandardFormat();

    VertexBufferData vbd = buildInterleavedVertexBuffer(mesh, format);

    const auto &data = vbd.mBuffers[0];
    // SSBO VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
    // Todo: better handling of
    if (SSBO)
    {
        createBuffer(device, physDevice, data.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, allocator);
    }
    else
    {
        createBuffer(device, physDevice, data.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, allocator);
    }

    uploadStaged(data.data(), data.size(), 0, physDevice, deviceM, indice, allocator);
}

void Buffer::createIndexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh,
                                const LogicalDeviceManager &deviceM, uint32_t indice, VmaAllocator allocator)
{
    // const VertexFormat &format = mesh.getFormat();
    const VertexFormat &format = VertexFormatRegistry::getStandardFormat();
    const auto &indices = mesh.indices;

    VkDeviceSize bufferSize = sizeof(uint32_t) * indices.size();
    createBuffer(device, physDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, allocator);
    uploadStaged(indices.data(), bufferSize, 0, physDevice, deviceM, indice, allocator);
}*/