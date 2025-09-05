#include "Buffer.h"

// Todo: Handle don't need to be passed as ref do they ?
///////////////////////////////////
// Buffer
///////////////////////////////////
// Start using https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
//   This is pretty bad      if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
// Check if you will use one buffer https://vulkan-tutorial.com/en/Vertex_buffers/Index_buffer for everything

uint32_t findMemoryType(const VkPhysicalDevice &physDevice, uint32_t memoryTypeBitsRequirement, const VkMemoryPropertyFlags &requiredProperties)
{

    // Todo: Could we keep this around in a variable somewhere ? But where
    VkPhysicalDeviceMemoryProperties pMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physDevice, &pMemoryProperties);

    // Find a memory in `memoryTypeBitsRequirement` that includes all of `requiredProperties`
    // Tldr is, memoryTypeBits is a bit mask defined in hex so we check each property by shifting to it
    // An see if it is set with the &
    // Try debug to see if all value are always here
    const uint32_t memoryCount = pMemoryProperties.memoryTypeCount;
    for (uint32_t memoryIndex = 0; memoryIndex < memoryCount; ++memoryIndex)
    {
        const uint32_t memoryTypeBits = (1 << memoryIndex);
        // Check if the element bit is part of the requirement
        const bool isRequiredMemoryType = memoryTypeBitsRequirement & memoryTypeBits;

        // Then we check if the required property are here
        const VkMemoryPropertyFlags properties =
            pMemoryProperties.memoryTypes[memoryIndex].propertyFlags;
        const bool hasRequiredProperties =
            (properties & requiredProperties) == requiredProperties;

        if (isRequiredMemoryType && hasRequiredProperties)
            return static_cast<int32_t>(memoryIndex);
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void Buffer::createBuffer(VkDevice device,
                          VkPhysicalDevice physDevice,
                          VkDeviceSize dataSize,
                          VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties)
{

    mDevice = device;
    mSize = dataSize;

    // In binary so the size is equal to the number of elements
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = dataSize;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &mBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create vertex buffer!");
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

void Buffer::destroyBuffer()
{
    // Error check
    if (mBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(mDevice, mBuffer, nullptr);
        vkFreeMemory(mDevice, mMemory, nullptr);
        mBuffer = VK_NULL_HANDLE;
        mMemory = VK_NULL_HANDLE;
    }
}

// Separateing staging mapping and copying maybe ?
void Buffer::uploadBuffer(const void *data, VkDeviceSize dataSize, VkDeviceSize dstOffset,
                          VkPhysicalDevice physDevice,
                          const LogicalDeviceManager &logDev,
                          uint32_t queueIndice)
{

    const auto &device = logDev.getLogicalDevice();

    Buffer stagingBuffer;
    stagingBuffer.createBuffer(device, physDevice, dataSize,
                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkUtils::BufferHelper::uploadBufferDirect(stagingBuffer.getMemory(), data, device, dataSize, dstOffset);

    copyFromBuffer(stagingBuffer.getBuffer(), dataSize, logDev, queueIndice, 0, dstOffset);

    // copyBuffer(stagingBuffer.getBuffer(), mBuffer, dataSize, logDev, indices, 0, dstOffset);

    stagingBuffer.destroyBuffer();
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

GPUBufferView Buffer::getView(VkDeviceSize offset, VkDeviceSize range)
{
    return GPUBufferView{mBuffer, offset, (range == VK_WHOLE_SIZE ? mSize : range)};
}

VkBufferView Buffer::createBufferView(VkFormat format, VkDeviceSize offset, VkDeviceSize size)
{
    return vkUtils::BufferHelper::createBufferView(mDevice, mBuffer, format, offset, size);
}

void Buffer::createVertexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh,
                                 const LogicalDeviceManager &deviceM, uint32_t indice)
{
    const VertexFormat &format = mesh.getFormat();

    VertexBufferData vbd = buildInterleavedVertexBuffer(mesh, format);

    const auto &data = vbd.mBuffers[0];

    createBuffer(device, physDevice, data.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    uploadBuffer(data.data(), data.size(), 0, physDevice, deviceM, indice);
}

void Buffer::createIndexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh,
                                const LogicalDeviceManager &deviceM, uint32_t indice)

{
    const VertexFormat &format = mesh.getFormat();
    const auto &indices = mesh.indices;

    VkDeviceSize bufferSize = sizeof(uint32_t) * indices.size();
    createBuffer(device, physDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    uploadBuffer(indices.data(), bufferSize, 0, physDevice, deviceM, indice);
}

// size is  a mix and VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT to create an unique buffer
void Buffer::createVertexIndexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const std::vector<Mesh> &meshes,
                                      const LogicalDeviceManager &deviceM, uint32_t indice)
{
    /*
    const VertexFormat &format = meshes[0].getFormat();
    VertexBufferData vbd = buildInterleavedVertexBuffer(meshes, format);

    const auto &data = vbd.mBuffers[0];
    // INdices are used as is
    // const auto &indices = mesh.indices;

    std::vector<uint32_t> combinedIndices;
    size_t meshOffset = 0;
    combinedIndices.reserve(0);

    for (const auto &mesh : meshes)
    {
        combinedIndices.reserve(combinedIndices.size() + mesh.indices.size());

        for (uint32_t index : mesh.indices)
        {
            combinedIndices.push_back(index + static_cast<uint32_t>(meshOffset));
        }

        meshOffset += mesh.vertexCount();
    }

    VkDeviceSize vertexSize = data.size();
    VkDeviceSize indicesSize = sizeof(uint32_t) * combinedIndices.size();
    VkDeviceSize bufferSize = vertexSize + indicesSize;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(stagingBuffer, stagingBufferMemory, device, physDevice, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Filling the vertex Buffer
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT imply using a heap equivalent
    void *databuff;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &databuff);
    memcpy(databuff, data.data(), bufferSize);
    memcpy(static_cast<uint8_t *>(databuff) + data.size(), combinedIndices.data(), indicesSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(mVertexBuffer, mVertexBufferMemory, device, physDevice, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    copyBuffer(stagingBuffer, mVertexBuffer, bufferSize, deviceM, indice);

    // Destro temporary staginf buffer
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
    */
}

/*

Quick method: memory (CPU) -> staging buffer (bridge) -> vertex buffer (GPU).
Slow method: memory (CPU) -> vertex buffer (GPU).

vkMapMemory: Gives us a CPU-accessible pointer to the staging buffer's memoryin GPU.
memcpy: Copies the vertex data from CPU memory to GPU memory.
vkUnmapMemory: Basically confirm that the writing is done

VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT memory is not host-visible (CPU) so the CPu can directly access it

CPU writes data into a HOST_VISIBLE staging buffer.
Copies that data into a DEVICE_LOCAL GPU buffer using a command buffer.


//It allow to refill the vertix easily and to use it faster
*/

/*
Let’s connect **Buffer / MeshBuffer / Texture** with **per-frame data (FrameResources)**.

---

## 🔹 1. What *is* frame data really?



* Command buffers
* Per-frame semaphores/fences
* Per-frame descriptor sets (if data is different per frame)
* Per-frame UBO/SSBO buffers (dynamic scene constants, camera, lights, etc.)

## 🔹 2. Which buffers/textures go into frame data?

* **Static data** (Meshes, Textures):

  * Uploaded once → never changes every frame.
  * Lives in global GPU memory, *not* duplicated per frame.
  * Example: vertex/index buffers, albedo textures, static environment maps.
* **Dynamic data** (UBOs, SSBOs, transient staging buffers):

  * Changes per frame.
  * Must live in **FrameResources**, because you need 1 copy per frame in flight.

---

## 🔹 3. Practical structure


```cpp
struct FrameResources {
    VkCommandBuffer cmd;
    VkSemaphore imageAvailable;
    VkSemaphore renderFinished;
    VkFence inFlight;

    Buffer uniformBuffer;   // per-frame UBO
    VkDescriptorSet descriptorSet; // bound to this frame’s UBO
};
```

---

### Tying them together in a render loop

```cpp
FrameResources& frame = frameResources[currentFrame];

// Update per-frame uniform buffer
CameraUBO ubo = { ... };
void* data = frame.uniformBuffer.map();
memcpy(data, &ubo, sizeof(ubo));
frame.uniformBuffer.unmap();

// Record commands
vkResetCommandBuffer(frame.cmd, 0);
recordDraw(frame.cmd, globalMesh, globalTexture, frame.descriptorSet);

// Submit using this frame’s semaphores/fence
submit(frame);
```

---

## 🔹 4. Where this leaves your design

* **MeshBuffer & Texture** → live globally in your Scene or ResourceManager.
  (not per frame, because they are static GPU data)
* **FrameResources** → owns transient buffers (UBO/SSBO) + descriptors + sync objects.
  (per frame, because they change)
* **Uploader/ResourceManager** → does the copyBufferToImage / staging work during init or asset load.
  After upload, frame data just **uses** them.

---
*/

namespace vkUtils
{
    namespace BufferHelper
    {
        // Buffer View can be recreated even after Buffer has been deleted
        inline VkBufferView createBufferView(VkDevice device, VkBuffer buffer, VkFormat format, VkDeviceSize offset, VkDeviceSize size)
        {
            VkBufferViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
            viewInfo.buffer = buffer;
            viewInfo.offset = offset;
            viewInfo.range = size;
            viewInfo.format = format; // must match how the shader interprets it

            VkBufferView bufferView = VK_NULL_HANDLE;
            VkResult result = vkCreateBufferView(device, &viewInfo, nullptr, &bufferView);
            if (result != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create VkBufferView");
            }

            return bufferView;
        }

        ///////////////////////////////////////////////////////////////////
        ////////////////////////Creation utility///////////////////////////
        ///////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////
        ////////////////////////Memory Barrier utility///////////////////////////
        ///////////////////////////////////////////////////////////////////

        void transitionBuffer(
            const BufferTransition &transitionObject,
            const LogicalDeviceManager &deviceM,
            uint32_t indice)
        {
            const auto &graphicsQueue = deviceM.getGraphicsQueue();
            const auto &device = deviceM.getLogicalDevice();

            CommandPoolManager cmdPoolM;
            cmdPoolM.createCommandPool(device, CommandPoolType::Transient, indice);
            VkCommandBuffer commandBuffer = cmdPoolM.beginSingleTime();

            VkBufferMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.srcAccessMask = transitionObject.srcAccessMask;
            barrier.dstAccessMask = transitionObject.dstAccessMask;

            // Default choice for now
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.buffer = transitionObject.buffer;
            barrier.offset = transitionObject.offset;
            barrier.size = transitionObject.size;

            vkCmdPipelineBarrier(
                commandBuffer,
                transitionObject.srcStageMask, transitionObject.dstStageMask,
                0,
                0, nullptr,
                1, &barrier,
                0, nullptr);

            cmdPoolM.endSingleTime(commandBuffer, graphicsQueue);
            cmdPoolM.destroyCommandPool();
        }

        /////////////////////////////////////////////////
        /////////////////Recording Utility///////////////
        /////////////////////////////////////////////////

        inline void recordCopy(VkCommandBuffer cmdBuffer,
                               VkBuffer srcBuffer,
                               VkBuffer dstBuffer,
                               VkDeviceSize size,
                               VkDeviceSize srcOffset,
                               VkDeviceSize dstOffset)
        {
            // Recording
            VkBufferCopy copyRegion{};
            copyRegion.srcOffset = srcOffset; // Optional
            copyRegion.dstOffset = dstOffset; // Optional
            copyRegion.size = size;
            vkCmdCopyBuffer(cmdBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
            // Could use a Buffer Memory Barrier and transition here
            // Chapter Copy dataBetween Buffer
        }

        void copyBufferTransient(VkBuffer srcBuffer, VkBuffer dstBuffer,
                                 VkDeviceSize size,
                                 const LogicalDeviceManager &deviceM,
                                 uint32_t indice,
                                 VkDeviceSize srcOffset,
                                 VkDeviceSize dstOffset)
        {
            const auto &graphicsQueue = deviceM.getGraphicsQueue();
            const auto &device = deviceM.getLogicalDevice();

            CommandPoolManager cmdPoolM;
            cmdPoolM.createCommandPool(device, CommandPoolType::Transient, indice);
            VkCommandBuffer commandBuffer = cmdPoolM.beginSingleTime();
            vkUtils::BufferHelper::recordCopy(commandBuffer, srcBuffer, dstBuffer,
                                              size, srcOffset, dstOffset);
            cmdPoolM.endSingleTime(commandBuffer, graphicsQueue);
            cmdPoolM.destroyCommandPool();
        }

        void uploadBufferDirect(
            VkDeviceMemory bufferMemory,
            const void *data,
            VkDevice device,
            VkDeviceSize size,
            VkDeviceSize dstOffset)
        {
            void *mapped = nullptr;
            vkMapMemory(device, bufferMemory, dstOffset, size, 0, &mapped);
            // Now stagingBufferMemory == mapped
            memcpy(mapped, data, static_cast<size_t>(size));
            vkUnmapMemory(device, bufferMemory);
        }
    }

}
