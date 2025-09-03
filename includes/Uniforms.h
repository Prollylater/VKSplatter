#include "BaseVk.h"
#include <glm/glm.hpp>
#include <array>
#include <unordered_map>
#include "VertexDescriptions.h"
#include "LogicalDevice.h"
#include "Buffer.h"

// Todo: Definitly clean all the include
// Rename
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/gtc/matrix_transform.hpp>
// Use alignas
#include <chrono>

class DescriptorManager
{
public:
    DescriptorManager() = default;
    ~DescriptorManager() = default;

    void createDescriptorPool(VkDevice device, uint32_t maxSets,
                              const std::vector<VkDescriptorPoolSize> &poolSizes);

    // Free only possible if Descriptor was created with
    // VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT

    void freeDescriptorSet(VkDevice device, int index)
    {
        VkResult result = vkFreeDescriptorSets(device, mDescriptorPool, 1, mDescriptorSets.data() + index);
    }

    void freeDescriptorSets(VkDevice device)
    {
        VkResult result = vkFreeDescriptorSets(device, mDescriptorPool, mDescriptorSets.size(), mDescriptorSets.data());

        if (VK_SUCCESS != result)
        {
            throw std::runtime_error("failed to free descripto sets!");
        }

        mDescriptorSets.clear();
    }

    void allocateDescriptorSets(VkDevice device, uint32_t setCount);
    void updateDescriptorSet(VkDevice device, uint32_t setIndex,
                             const std::vector<VkWriteDescriptorSet> &writes);
    void createDescriptorSetLayout(VkDevice device, std::vector<VkDescriptorSetLayoutBinding> bindings);

    void destroyDescriptorPools(VkDevice device)
    {
        if (mDescriptorPool != VK_NULL_HANDLE)
        {
            freeDescriptorSets(device);
            vkDestroyDescriptorPool(device, mDescriptorPool, nullptr);
            mDescriptorPool = VK_NULL_HANDLE;
        }
    }

    void destroyDescriptorLayout(VkDevice device);

    const VkDescriptorSetLayout getDescriptorLat() const
    {

        return mDescriptorSetLayout;
    }

    const VkDescriptorSet &getSet(int index) const
    {

        return mDescriptorSets[index];
    }

private:
    VkDescriptorPool mDescriptorPool;
    VkDescriptorSetLayout mDescriptorSetLayout;
    std::vector<VkDescriptorSet> mDescriptorSets;
};

namespace vkUtils
{

    namespace Descriptor
    {
        inline VkDescriptorSetLayoutBinding makeLayoutBinding(
            uint32_t binding, VkDescriptorType type, VkShaderStageFlags stages, uint32_t count = 1)
        {
            VkDescriptorSetLayoutBinding layout{};
            layout.binding = binding;
            layout.descriptorType = type;
            layout.descriptorCount = count;
            layout.stageFlags = stages;
            return layout;
        }

        inline VkWriteDescriptorSet makeWriteDescriptor(
            VkDescriptorSet dstSet, uint32_t binding, VkDescriptorType type,
            const VkDescriptorBufferInfo *bufferInfo = nullptr,
            const VkDescriptorImageInfo *imageInfo = nullptr)
        {
            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = dstSet;
            write.dstArrayElement = 0;
            write.dstBinding = binding;
            write.descriptorCount = 1;
            write.descriptorType = type;
            write.pBufferInfo = bufferInfo;
            write.pImageInfo = imageInfo;
            write.pTexelBufferView = VK_NULL_HANDLE;

            return write;
        }

    }
}

/*
Multiple descriptor sets

As some of the structures and function calls hinted at, it is actually
possible to bind multiple descriptor sets simultaneously. You need to
specify a descriptor layout for each descriptor set when creating the
pipeline layout. Shaders can then reference specific descriptor sets like
this:
layout(set = 0, binding = 0) uniform UniformBufferObject { ... }

*/

/*
SSSBO
//Shoudl Material be
struct Material {
    vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;

    int baseColorTextureID;
    int normalTextureID;
    int metallicRoughnessTextureID;
    int emissiveTextureID;

    vec3 emissiveFactor;
    int alphaMode; // 0 = opaque, 1 = mask, 2 = blend
};

*/