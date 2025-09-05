
#include "Uniforms.h"
#include "Buffer.h"

///////////////////////////////////
// Descriptor
///////////////////////////////////

/*
VkDescriptorPoolSize poolSizes[] = {
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 }, // up to 100 textures
    { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 50 },          // up to 50 UBOs
    { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 20 }           // up to 20 SSBOs
};
Type, descriptor count
VkDescriptorPoolCreateInfo poolInfo{};
poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
poolInfo.maxSets = 50;   // maximum number of *sets* allocated
*/

// Manage memory for descriptor sets,
// In PoolSize size is tied to the type the number of sets/and descriptor
void DescriptorManager::createDescriptorPool(VkDevice device, uint32_t maxSets,
                                             const std::vector<VkDescriptorPoolSize> &poolSizes)
{
  // Bigger pool size,  a pool for each descriptor types
  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = static_cast<uint32_t>(ContextVk::contextInfo.MAX_FRAMES_IN_FLIGHT);

  if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &mDescriptorPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor pool!");
  }
};

// Describe the set, their binding and so on
// Describe the type of binding each pool will be used to geenrate

void DescriptorManager::allocateDescriptorSets(VkDevice device, uint32_t setCount)
{
  // Use two mDescriptorSetLayout to create two descirptor set
  std::vector<VkDescriptorSetLayout> layouts(setCount, mDescriptorSetLayout);

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = mDescriptorPool;
  allocInfo.descriptorSetCount = layouts.size();
  allocInfo.pSetLayouts = layouts.data();

  mDescriptorSets.resize(setCount);
  // ContextVk::contextInfo.MAX_FRAMES_IN_FLIGHTs
  if (vkAllocateDescriptorSets(device, &allocInfo, mDescriptorSets.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }
};

void DescriptorManager::updateDescriptorSet(VkDevice device, uint32_t setIndex,
                                            const std::vector<VkWriteDescriptorSet> &writes)
{
  vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
};

void DescriptorManager::createDescriptorSetLayout(VkDevice device, std::vector<VkDescriptorSetLayoutBinding> bindings)
{
  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();
  layoutInfo.flags = 0;

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &mDescriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor set layout!");
  }
}

void DescriptorManager::destroyDescriptorPool(VkDevice device)
{
  if (mDescriptorPool != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorPool(device, mDescriptorPool, nullptr);
    mDescriptorPool = VK_NULL_HANDLE;
  }
}

void DescriptorManager::destroyDescriptorLayout(VkDevice device)
{
   if (mDescriptorSetLayout != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorSetLayout(device, mDescriptorSetLayout, nullptr);
    mDescriptorSetLayout = VK_NULL_HANDLE;
  }
}

/*

For  samplers and all kinds of image descriptors, an ImageDescriptorInfo type is used which has the following definition:


struct ImageDescriptorInfo {
  VkDescriptorSet                     TargetDescriptorSet;
  uint32_t                            TargetDescriptorBinding;
  uint32_t                            TargetArrayElement;
  VkDescriptorType                    TargetDescriptorType;
  std::vector<VkDescriptorImageInfo>  ImageInfos;
};

For uniform and storage buffers (and their dynamic variations), a BufferDescriptorInfo type is used. It has the following definition:


struct BufferDescriptorInfo {
  VkDescriptorSet                     TargetDescriptorSet;
  uint32_t                            TargetDescriptorBinding;
  uint32_t                            TargetArrayElement;
  VkDescriptorType                    TargetDescriptorType;
  std::vector<VkDescriptorBufferInfo> BufferInfos;
};
For uniform and storage texel buffers, a TexelBufferDescriptorInfo type is introduced with the following definition:


struct TexelBufferDescriptorInfo {
  VkDescriptorSet                     TargetDescriptorSet;
  uint32_t                            TargetDescriptorBinding;
  uint32_t                            TargetArrayElement;
  VkDescriptorType                    TargetDescriptorType;
  std::vector<VkBufferView>           TexelBufferViews;
};
The preceding structures are used when we want to update descriptor sets with handles of new descriptors (that haven't been bound yet).
It is also possible to copy descriptor data from other, already updated, sets. For this purpose, a CopyDescriptorInfo type is used that is defined like this:


struct CopyDescriptorInfo {
  VkDescriptorSet     TargetDescriptorSet;
  uint32_t            TargetDescriptorBinding;
  uint32_t            TargetArrayElement;
  VkDescriptorSet     SourceDescriptorSet;
  uint32_t            SourceDescriptorBinding;
  uint32_t            SourceArrayElement;
  uint32_t            DescriptorCount;
};
All the preceding structures define the handle of a descriptor set that should be updated,
an index of a descriptor within the given set, and an index into an array if we want to update descriptors accessed through arrays. The rest of the parameters are type-specific.

*/