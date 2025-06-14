#include "BaseVk.h"
#include <glm/glm.hpp>
#include <array>
#include <unordered_map>
#include "VertexDescriptions.h"
#include "LogicalDevice.h"
///////////////////////////////////
// Buffer
///////////////////////////////////
// Temporary use
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/gtc/matrix_transform.hpp>
//Use alignas
#include <chrono>

//Not convice by the bunuds
/*
whjat's witj thje multiple uniform bueffer again ?
They areused in renderpass only right ?
*/
struct UniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class TextureManager;

class DescriptorManager
{
public:
    DescriptorManager() = default;
    ~DescriptorManager() = default;

    void createDescriptorPool(VkDevice device);

    void createDescriptorSets(VkDevice device, TextureManager& texutreM);

    void createDescriptorSetLayout(VkDevice device);
    void createUniformBuffers(VkDevice device, VkPhysicalDevice physDevice);
    void updateUniformBuffers(uint32_t currentImage, VkExtent2D swapChainExtent);

    void destroyDescriptorPools(VkDevice device)
    {
        vkDestroyDescriptorPool(device, mDescriptorPool, nullptr);
    }

    void destroyDescriptorLayout(VkDevice device);

    void destroyUniformBuffer(VkDevice device);

    const VkDescriptorSetLayout getDescriptorLat() const
    {

        return mDescriptorSetLayout;
    }

    const VkDescriptorSet& getSet(int index) const
    {

        return mDescriptorSets[index];
    }


private:
    VkDescriptorPool mDescriptorPool;
    VkDescriptorSetLayout mDescriptorSetLayout;

    // Buffer
    std::vector<VkDescriptorSet> mDescriptorSets;

    std::vector<VkBuffer> mUniformBuffers;
    std::vector<VkDeviceMemory> mUniformBuffersMemory;
    std::vector<void *> mUniformBuffersMapped;
};

// TOOD: Chang eother at VK_NULL_HANDLE
//  Destroy on not real object nor null handle can have weird conseuqeujces

/*

Multiple descriptor sets

As some of the structures and function calls hinted at, it is actually 
possible to bind multiple descriptor sets simultaneously. You need to 
specify a descriptor layout for each descriptor set when creating the 
pipeline layout. Shaders can then reference specific descriptor sets like 
this:

layout(set = 0, binding = 0) uniform UniformBufferObject { ... }


Also there is the array of 
*/