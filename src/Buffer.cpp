#include "Buffer.h"

///////////////////////////////////
// Buffer
///////////////////////////////////
// Start using https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
//   This is pretty bad      if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
// Check if you will use one buffer https://vulkan-tutorial.com/en/Vertex_buffers/Index_buffer for veeryhting
void Buffer::destroyVertexBuffer(VkDevice device)
{
    vkDestroyBuffer(device, mVertexBuffer, nullptr);
    vkFreeMemory(device, mVertexBufferMemory, nullptr);
}

void Buffer::destroyIndexBuffer(VkDevice device)
{
    vkDestroyBuffer(device, mIndexBuffer, nullptr);
    vkFreeMemory(device, mIndexBufferMemory, nullptr);
}

// Very long
void createBuffer(VkBuffer &buffer, VkDeviceMemory &bufferMemory, const VkDevice &device, const VkPhysicalDevice &physDevice, VkDeviceSize dataSize,
                          VkBufferUsageFlags usage, VkMemoryPropertyFlags flagsProp)
{

    // VertexBufferData vbd= buildSeparatedVertexBuffers(mesh, format);

    // Only one vertex is expected currently
    {
        // In binary so the size is equal to the number of elements
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = dataSize;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create vertex buffer!");
        }

        // MEmory allocation
        // Todo: Take a look again at this

        // This function could be better written
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(physDevice, memRequirements.memoryTypeBits, flagsProp);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate vertex buffer memory!");
        }

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }
}

/*

vkCmdBindVertexBuffers(command, 0,  1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(command, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    //vkCmdDraw(command, static_cast<uint32_t>(mesh.vertexCount()), 1, 0, 0);
    vkCmdDrawIndexed(command, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
    //vkCmdBindIndexBuffer(command, vertexBuffers, indexOffset, VK_INDEX_TYPE_UINT32);


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
// Those function are riddled with compromise mainly on the function paramter

void Buffer::createVertexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh,
                                 const LogicalDeviceManager &deviceM, const CommandPoolManager &cmdPoolM, const QueueFamilyIndices indice)

{
    const VertexFormat &format = mesh.getFormat();
    for (size_t i = 0; i < 1 /*vbd.mBuffers.size()*/; ++i)
    {
        VertexBufferData vbd = buildInterleavedVertexBuffer(mesh, format);
        const auto &data = vbd.mBuffers[0];

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(stagingBuffer, stagingBufferMemory, device, physDevice, data.size(),
                     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        // Filling the vertex Buffer
        // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT imply using a heap equivalent
        void *databuff;
        vkMapMemory(device, stagingBufferMemory, 0, data.size(), 0, &databuff);
        memcpy(databuff, data.data(), data.size());
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(mVertexBuffer, mVertexBufferMemory, device, physDevice, data.size(),
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        copyBuffer(stagingBuffer, mVertexBuffer, data.size(), deviceM, cmdPoolM, indice);

        // Destro temporary staginf buffer
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }
}


void Buffer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, const LogicalDeviceManager &deviceM, const CommandPoolManager &cmdPoolM,
                        const QueueFamilyIndices &indices)
{
    const auto &graphicsQueue = deviceM.getGraphicsQueue();
    const auto &device = deviceM.getLogicalDevice();
    const auto &commandPool = cmdPoolM.createSubCmdPool(device, indices, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
   
    CommandBuffer cmdBuffer;
    int uniqueIndex = 0;
    cmdBuffer.createCommandBuffers(deviceM.getLogicalDevice(), commandPool, 1);
    VkCommandBuffer commandBuffer = cmdBuffer.get(uniqueIndex); 
    cmdBuffer.beginRecord(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, uniqueIndex); 
    // Recording

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    cmdBuffer.endRecord(uniqueIndex);

    
    //Lot of thing to rethink here
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

    vkDestroyCommandPool(device, commandPool, nullptr);
}

/*void Buffer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, const LogicalDeviceManager &deviceM, const CommandPoolManager &cmdPoolM,
                        const QueueFamilyIndices &indices)
{
    const auto &graphicsQueue = deviceM.getGraphicsQueue();
    const auto &device = deviceM.getLogicalDevice();
    const auto &commandPool = cmdPoolM.createSubCmdPool(device, indices, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(deviceM.getLogicalDevice(), &allocInfo, &commandBuffer);

    // Recording
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

    vkDestroyCommandPool(device, commandPool, nullptr);
}
*/
// size is  a mix and VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT to create an unique buffer
void Buffer::createVertexIndexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh,
                                      const LogicalDeviceManager &deviceM, const CommandPoolManager &cmdPoolM, const QueueFamilyIndices indice)
{
    const VertexFormat &format = mesh.getFormat();
    VertexBufferData vbd = buildInterleavedVertexBuffer(mesh, format);
    const auto &data = vbd.mBuffers[0];
    const auto &indices = mesh.indices;

    VkDeviceSize bufferSize = sizeof(uint32_t) * mesh.indices.size() + data.size();
    VkDeviceSize indicesSize = sizeof(uint32_t) * mesh.indices.size();

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
    //memcpy(databuff + data.size(), indices.data(), indicesSize);
    memcpy(static_cast<uint8_t*>(databuff) + data.size(), indices.data(), indicesSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(mVertexBuffer, mVertexBufferMemory, device, physDevice, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    copyBuffer(stagingBuffer, mVertexBuffer, bufferSize, deviceM, cmdPoolM, indice);

    // Destro temporary staginf buffer
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Buffer::createIndexBuffers(const VkDevice &device, const VkPhysicalDevice &physDevice, const Mesh &mesh,
                                const LogicalDeviceManager &deviceM, const CommandPoolManager &cmdPoolM, const QueueFamilyIndices indice)

{
    const VertexFormat &format = mesh.getFormat();
    const auto &indices = mesh.indices;
    VkDeviceSize bufferSize = sizeof(uint32_t) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(stagingBuffer, stagingBufferMemory, device, physDevice, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // Filling the vertex Buffer
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT imply using a heap equivalent
    void *databuff;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &databuff);
    memcpy(databuff, indices.data(), bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(mIndexBuffer, mIndexBufferMemory, device, physDevice, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    copyBuffer(stagingBuffer, mIndexBuffer, bufferSize, deviceM, cmdPoolM, indice);

    // Destro temporary staginf buffer
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
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