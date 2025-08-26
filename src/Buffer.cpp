#include "Buffer.h"

// Todo: Handle don't need to be passed as ref do they ?
///////////////////////////////////
// Buffer
///////////////////////////////////
// Start using https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
//   This is pretty bad      if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
// Check if you will use one buffer https://vulkan-tutorial.com/en/Vertex_buffers/Index_buffer for everything

void Buffer::createBuffer(VkDevice device,
                          VkPhysicalDevice physDevice,
                          VkDeviceSize dataSize,
                          VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties)
{

    mDevice = device;

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
    allocInfo.memoryTypeIndex = findMemoryType(physDevice, memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &mMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(device, mBuffer, mMemory, 0);
}

void Buffer::destroyBuffer()
{
    // Error check
    vkDestroyBuffer(mDevice, mBuffer, nullptr);
    vkFreeMemory(mDevice, mMemory, nullptr);
    mBuffer = VK_NULL_HANDLE;
    mMemory = VK_NULL_HANDLE;
}

// Separateing staging mapping and copying maybe ?
void Buffer::uploadBuffer(const void *data, VkDeviceSize dataSize, VkDeviceSize dstOffset,
                          VkPhysicalDevice physDevice,
                          const LogicalDeviceManager &logDev,
                          const QueueFamilyIndices &indices)
{

    const auto &device = logDev.getLogicalDevice();

    // Filling the vertex Buffer
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT imply using a heap equivalent
    Buffer stagingBuffer;
    // Todo: Do we need Allow flexibility on this ? Look into tutorial again
    // VK_BUFFER_USAGE_TRANSFER_SRC_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,

    stagingBuffer.createBuffer(device, physDevice, dataSize,
                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Create util
    void *databuff;
    // Make buffer and databuff host Mapped to allow a memcpy
    vkMapMemory(device, stagingBuffer.getMemory(), 0, dataSize, 0, &databuff);
    // Now stagingBufferMemory == dataBuff
    memcpy(databuff, data, dataSize);
    vkUnmapMemory(device, stagingBuffer.getMemory());

    copyBuffer(stagingBuffer.getBuffer(), mBuffer, dataSize, 0, dstOffset, logDev, indices);

    stagingBuffer.destroyBuffer();
};

// Should be usable by everything like create buffer
void Buffer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, const LogicalDeviceManager &deviceM,
                        const QueueFamilyIndices &indices)
{
    copyBuffer(srcBuffer, dstBuffer, size, 0, 0, deviceM, indices);
}

void Buffer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size,
                        VkDeviceSize srcOffset,
                        VkDeviceSize dstOffset,
                        const LogicalDeviceManager &deviceM,
                        const QueueFamilyIndices &indices)
{
    const auto &graphicsQueue = deviceM.getGraphicsQueue();
    const auto &device = deviceM.getLogicalDevice();

    CommandPoolManager cmdPoolM;
    // Todo: Should use a dedicated transfer queue ?
    cmdPoolM.createCommandPool(device, CommandPoolType::Transient, indices.graphicsFamily.value());
    VkCommandBuffer commandBuffer = cmdPoolM.beginSingleTime();
    // Recording
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = srcOffset; // Optional
    copyRegion.dstOffset = dstOffset; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    cmdPoolM.endSingleTime(commandBuffer, graphicsQueue);
    cmdPoolM.destroyCommandPool();
}

GPUBufferView Buffer::getView(VkDeviceSize offset, VkDeviceSize range)
{
    return GPUBufferView{mBuffer, offset, (range == VK_WHOLE_SIZE ? mSize : range)};
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

void Buffer::createVertexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh,
                                 const LogicalDeviceManager &deviceM, const QueueFamilyIndices indice)
{
    const VertexFormat &format = mesh.getFormat();

    VertexBufferData vbd = buildInterleavedVertexBuffer(mesh, format);
    mSize = mesh.vertexCount();

    const auto &data = vbd.mBuffers[0];

    createBuffer(device, physDevice, data.size(), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

                 
    uploadBuffer(data.data(), data.size(), 0, physDevice, deviceM, indice);
}

void Buffer::createIndexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh,
                                const LogicalDeviceManager &deviceM, const QueueFamilyIndices indice)

{
    const VertexFormat &format = mesh.getFormat();
    const auto &indices = mesh.indices;
    mSize = mesh.indices.size();

    VkDeviceSize bufferSize = sizeof(uint32_t) * indices.size();
    createBuffer(device, physDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    uploadBuffer(indices.data(), bufferSize, 0, physDevice, deviceM, indice);
}

uint32_t findMemoryType(const VkPhysicalDevice &physDevice, uint32_t memoryTypeBitsRequirement, const VkMemoryPropertyFlags &requiredProperties)
{

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

// size is  a mix and VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT to create an unique buffer
void Buffer::createVertexIndexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const std::vector<Mesh> &meshes,
                                      const LogicalDeviceManager &deviceM, const QueueFamilyIndices indice)
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



Perfect follow-up 👍 — because this is exactly where beginners overcomplicate, and pros keep things clean.

Let’s connect **Buffer / MeshBuffer / Texture** with **per-frame data (FrameResources)**.

---

## 🔹 1. What *is* frame data really?

Frame resources are just the **per-frame things that change**, because we can’t safely reuse them until the GPU finishes.
That usually means:

* Command buffers
* Per-frame semaphores/fences
* Per-frame descriptor sets (if data is different per frame)
* Per-frame UBO/SSBO buffers (dynamic scene constants, camera, lights, etc.)

---

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

### Global resources (live in VulkanContext / ResourceManagers)

```cpp
struct MeshBuffer {
    Buffer vertexBuffer;
    Buffer indexBuffer;
    uint32_t indexCount;
};

struct Texture {
    VkImage image;
    VkImageView view;
    VkSampler sampler;
};
```

These are **owned once**, uploaded once. No duplication per frame.

---

### FrameResources (rotated each frame)

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

✅ So:

* Mesh/Texture = *static, once per scene*
* FrameResources = *dynamic, per frame in flight*
* No need to duplicate meshes/textures in FrameResources

---

Would you like me to sketch a **class diagram** showing how `FrameResources`, `Scene` (meshes, textures), and `Uploader` tie into `Renderer`? That’d make the ownership/flow crystal clear.



*/