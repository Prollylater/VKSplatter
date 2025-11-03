#include "Buffer.h"
#include "utils/RessourceHelper.h"
#include "Mesh.h" //Scene

///////////////////////////////////
// Buffer
///////////////////////////////////
// Start using https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
//   This is pretty bad      if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
// Check if you will use one buffer https://vulkan-tutorial.com/en/Vertex_buffers/Index_buffer for everything
// Todo: Multithreading stagin stuff ?

void Buffer::createBuffer(VkDevice device,
                          VkPhysicalDevice physDevice,
                          VkDeviceSize dataSize,
                          VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties,
                          VmaAllocator allocator)
{
//Todo: ALlocation with vma mighht need to be separated to  make more granular cchange
    mDevice = device;
    mSize = dataSize;

    // In binary so the size is equal to the number of elements
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = dataSize;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (allocator != VK_NULL_HANDLE)
    {
        useVma = true;
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.requiredFlags = properties;
        // Todo: Better method to pass this
        allocInfo.flags  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        if (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            //VMA_ALLOCATION_CREATE_MAPPED_BIT automatically map a bit and hide it cleanly i guess
            if (properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
            {
                // Usually upload buffer (CPU → GPU)
                allocInfo.flags = allocInfo.flags | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            }
            else
            {
                //TOdo: Not sure
                // Usually readback buffer (GPU → CPU) ?
                allocInfo.flags = allocInfo.flags |VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            }

            //TOdo: This si usable only when
            //.bufferDeviceAddress is used which is default right now
        }
        if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &mBuffer, &mBufferAllocation, nullptr) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create buffer through VMA!");
        }
    }
    else
    {
        if (vkCreateBuffer(device, &bufferInfo, nullptr, &mBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create buffer!");
        }

        // Memory allocation
        // Todo: Take a look again at this
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, mBuffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        // Could be passed as uint32 directly
        allocInfo.memoryTypeIndex = findMemoryType(physDevice, memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &mMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate vertex buffer memory!");
        }

        if (vkBindBufferMemory(device, mBuffer, mMemory, 0))
        {
            throw std::runtime_error("failed to bind buffer memory!");
        }
    }
}

void Buffer::destroyBuffer(VkDevice device, VmaAllocator allocator)
{
    // Error check
    // Todo: Don't need to check VK_NULL_HANDLE
    if (mBuffer != VK_NULL_HANDLE)
    {
        if (useVma)
        {
            vmaDestroyBuffer(allocator, mBuffer, mBufferAllocation);
        }
        else
        {
            vkDestroyBuffer(mDevice, mBuffer, nullptr);
            vkFreeMemory(mDevice, mMemory, nullptr);
        }
        mBuffer = VK_NULL_HANDLE;
        mMemory = VK_NULL_HANDLE;
        mBufferAllocation = VK_NULL_HANDLE;
    }
}

// Separateing staging mapping and copying maybe ?
// This function create a staging buffer a command buffer and upload the buffer
void Buffer::uploadBuffer(const void *data, VkDeviceSize dataSize, VkDeviceSize dstOffset,
                          VkPhysicalDevice physDevice,
                          const LogicalDeviceManager &logDev,
                          uint32_t queueIndice, VmaAllocator allocator)
{
    const auto &device = logDev.getLogicalDevice();

    Buffer stagingBuffer;
    stagingBuffer.createBuffer(device, physDevice, dataSize,
                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, allocator);

    if (useVma && allocator && mBufferAllocation)
    {

        vkUtils::BufferHelper::uploadBufferVMA(allocator, stagingBuffer.getVMAMemory(), data, dataSize, dstOffset);
    }
    else
    {
        vkUtils::BufferHelper::uploadBufferDirect(device, stagingBuffer.getMemory(), data, dataSize, dstOffset);
    }

    copyFromBuffer(stagingBuffer.getBuffer(), dataSize, logDev, queueIndice, 0, dstOffset);
    stagingBuffer.destroyBuffer(device, allocator);
};

void Buffer::copyToBuffer(VkBuffer dstBuffer,
                          VkDeviceSize size,
                          const LogicalDeviceManager &deviceM,
                          uint32_t indice,
                          VkDeviceSize srcOffset,
                          VkDeviceSize dstOffset)
{
    vkUtils::BufferHelper::copyBufferTransient(mBuffer, dstBuffer, size, deviceM, indice, 0, 0);
}

void Buffer::copyFromBuffer(VkBuffer srcBuffer,
                            VkDeviceSize size,
                            const LogicalDeviceManager &deviceM,
                            uint32_t indice,
                            VkDeviceSize srcOffset,
                            VkDeviceSize dstOffset)
{

    vkUtils::BufferHelper::copyBufferTransient(srcBuffer, mBuffer, size, deviceM, indice, 0, 0);
}


VkBufferView Buffer::createBufferView(VkFormat format, VkDeviceSize offset, VkDeviceSize size)
{
    return vkUtils::BufferHelper::createBufferView(mDevice, mBuffer, format, offset, size);
}

void Buffer::createVertexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh,
                                 const LogicalDeviceManager &deviceM, uint32_t indice, VmaAllocator allocator, bool SSBO)
{
    const VertexFormat &format = mesh.getFormat();

    VertexBufferData vbd = buildInterleavedVertexBuffer(mesh, format);

    const auto &data = vbd.mBuffers[0];
    // SSBO VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
    //Todo: better handling of
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
    uploadBuffer(data.data(), data.size(), 0, physDevice, deviceM, indice, allocator);
}

void Buffer::createIndexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh,
                                const LogicalDeviceManager &deviceM, uint32_t indice, VmaAllocator allocator)

{
    const VertexFormat &format = mesh.getFormat();
    const auto &indices = mesh.indices;

    VkDeviceSize bufferSize = sizeof(uint32_t) * indices.size();
    createBuffer(device, physDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, allocator);
    uploadBuffer(indices.data(), bufferSize, 0, physDevice, deviceM, indice, allocator);
}

// This function will not check if the buffer is properly mapped
void *Buffer::map(VmaAllocator allocator)
{
    if (mMapped)
        return mMapped;
    // Could do something like
    // if (!(mMemoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
    //    throw std::runtime_error("Notmapped!");

    if (useVma)
    {
        if (vmaMapMemory(allocator, mBufferAllocation, &mMapped) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to map buffer memory via VMA!");
        }
    }
    else
    {

        if (vkMapMemory(mDevice, mMemory, 0, mSize, 0, &mMapped) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to map buffer memory!");
        }
    }

    return mMapped;
}
void Buffer::unmap(VmaAllocator allocator)
{
    if (!mMapped)
        return; // already unmapped

    if (useVma)
    {
        vmaUnmapMemory(allocator, mBufferAllocation);
    }
    else
    {
        vkUnmapMemory(mDevice, mMemory);
    }

    mMapped = nullptr;
}
