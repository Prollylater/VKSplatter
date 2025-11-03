#pragma once

#include "BaseVk.h"
#include <glm/glm.hpp>
#include <array>
#include <unordered_map>
#include "VertexDescriptions.h"
#include "LogicalDevice.h"
#include "Buffer.h"

#define GLM_FORCE_RADIANS
// Default aligned helper
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/gtc/matrix_transform.hpp>
// Use alignas
#include <chrono>

size_t hashBindings(const std::vector<VkDescriptorSetLayoutBinding> &bindings);

// REad on this VK_EXT_descriptor_indexing
class DescriptorManager
{
public:
    DescriptorManager() = default;
    ~DescriptorManager() = default;

    void createDescriptorPool(VkDevice device, uint32_t maxSets,
                              const std::vector<VkDescriptorPoolSize> &poolSizes);

    // Free only possible if Descriptor was created with
    // VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
    void freeDescriptorSet(VkDevice device, int index);
    void freeDescriptorSets(VkDevice device);
    // Work without VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
    void resetDescriptorSets(VkDevice device);

    void allocateDescriptorSets(VkDevice device);
    int allocateDescriptorSet(VkDevice device, int set);

    void updateDescriptorSet(VkDevice device, uint32_t setIndex,
                             const std::vector<VkWriteDescriptorSet> &writes);
    // TODO: To uin32_t .
    int getOrCreateSetLayout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding> &bindings);

    void destroyDescriptorPool(VkDevice device);
    void destroyDescriptorLayout(VkDevice device);

    const VkDescriptorSetLayout getDescriptorLat(int index) const;

    const VkDescriptorSet getSet(int index) const ;

    VkDescriptorSet &getSet(int index) ; 

    const size_t getSetNb() const ;

private:
    VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
    std::unordered_map<size_t, int> mLayoutHashToIndex;
    std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts;

    // std::vector<VkDescriptorSetLayout> mDescriptorSetLayouts;
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