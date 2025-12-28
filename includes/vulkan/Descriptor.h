#pragma once

#include "BaseVk.h"
#include <unordered_map>

size_t hashBindings(const std::vector<VkDescriptorSetLayoutBinding> &bindings);

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

    void updateDescriptorSet(VkDevice device, const std::vector<VkWriteDescriptorSet> &writes);
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
