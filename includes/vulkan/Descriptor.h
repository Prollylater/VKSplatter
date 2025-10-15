#pragma once

#include "BaseVk.h"
#include <glm/glm.hpp>
#include <array>
#include <unordered_map>
#include "VertexDescriptions.h"
#include "LogicalDevice.h"
#include "Buffer.h"

#define GLM_FORCE_RADIANS
//Default aligned helper
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/gtc/matrix_transform.hpp>
// Use alignas
#include <chrono>


//REad on this VK_EXT_descriptor_indexing
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

    //Work witout VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
    void resetDescriptorSets(VkDevice device)
    {
        VkResult result = vkResetDescriptorPool(device, mDescriptorPool, 0);

        if (VK_SUCCESS != result)
        {
            throw std::runtime_error("Failed to reset descriptor pool!");
        }

        mDescriptorSets.clear();
    }

    void allocateDescriptorSets(VkDevice device, uint32_t setCount);
    void updateDescriptorSet(VkDevice device, uint32_t setIndex,
                             const std::vector<VkWriteDescriptorSet> &writes);
    void createDescriptorSetLayout(VkDevice device, std::vector<VkDescriptorSetLayoutBinding> bindings);

    void destroyDescriptorPool(VkDevice device);
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
    VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
    //TODO: Multiple mDescriptorSetLayout
    VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> mDescriptorSets;
};


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