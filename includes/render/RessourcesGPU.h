#pragma once
#include "BaseVk.h"

struct Mesh;
struct Material;
class LogicalDeviceManager;
class DescriptorManager;
class AssetRegistry;

// Todo: Can this also be shared ? Depending on howw other color data are handled
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
        //Todo: uint32_t
        int descriptorLayoutIdx;
        int pipelineIndex;
    };
    
    static MaterialGPU createMaterialGPU(const AssetRegistry &registry, MaterialGPUCreateInfo info, const LogicalDeviceManager &deviceM, DescriptorManager &descriptor,  VkPhysicalDevice physDevice, uint32_t indice);

};

struct MeshGPU
{
    // TOdo: SHould i derectly use my Buffer class ?
    //  Cleanup object in case the mesh buffer is not kept
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VmaAllocation vertexAlloc = VK_NULL_HANDLE;
    VmaAllocation indexAlloc = VK_NULL_HANDLE;
    VkDeviceMemory vertexMem = VK_NULL_HANDLE;
    VkDeviceMemory indexMem = VK_NULL_HANDLE;

    VkDeviceAddress vertexAddress = 0;

    uint32_t vertexCount = 0; // Not too sure if this would be useful
    uint32_t indexCount = 0;
    uint32_t vertexStride = 0;

    static MeshGPU createMeshGPU(const Mesh &mesh, const LogicalDeviceManager &deviceM, const VkPhysicalDevice &physDevice, uint32_t indice, bool SSBO = false);
    // void bind()
    void destroy(VkDevice device, VmaAllocator alloc = VK_NULL_HANDLE);
};

struct InstanceData;
struct InstanceGPU
{
    VkBuffer instanceBuffer = VK_NULL_HANDLE;
    VmaAllocation instanceAlloc = VK_NULL_HANDLE;
    VkDeviceMemory instanceMem = VK_NULL_HANDLE;
    // VkDeviceAddress indexAddress = 0;

    uint32_t instanceCount;
    uint32_t instanceStride;

    static InstanceGPU createInstanceGPU(const std::vector<InstanceData> &mesh, const LogicalDeviceManager &deviceM, const VkPhysicalDevice &physDevice, uint32_t indice);
    // void bind()
    void destroy(VkDevice device, VmaAllocator alloc = VK_NULL_HANDLE);
};

struct GPUBufferView
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceAddress mVertexBufferAddress = 0;
};

/*

struct GPUBufferView
{
    VkBuffer mBuffer = VK_NULL_HANDLE;
    VkDeviceAddress mVertexBufferAddress = 0;
    // Goal would be to pass this to an Uniform Buffer
    VkDeviceSize mOffset = 0;
    VkDeviceSize mSize = 0;
};

struct AttributeStream // An elements ?
{
    GPUBufferView mView;
    uint32_t mStride = 0;
    // Todo:
    // Decide how to handle it in regard to the actual buffer
    // Specifally with the build interleaved and so on.
    // This is only really fine due to how we construct our stuff but it render the whole MeshGPuRessources abstraction useless
    // Since the descriptor handle everything well enough
    // Using Attributes just assume everything is neatly deferred to the descriptor
    // Main use so far is for non interleaved and to allow us to drop the Buffer once created
    enum class Type
    {
        Attributes,
        /*Pos, Normal, UV, Color,* / Index
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

*/