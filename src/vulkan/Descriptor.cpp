
#include "Descriptor.h"

///////////////////////////////////
// Descriptor
///////////////////////////////////

constexpr uint32_t MAX_SETS = 4;
constexpr uint32_t MAX_UNIFORM_BUFFERS = 32;
constexpr uint32_t MAX_SAMPLERS = 64;
constexpr uint32_t MAX_SSBO = 10;

//TODO:
//Separate this
//Ex: Material pool don't need SSBO currently
const std::array<VkDescriptorPoolSize, 3> DEFAULT_POOL_SIZES = {{
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_UNIFORM_BUFFERS},
    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_SAMPLERS},
    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_SSBO},
}};

// Manage memory for descriptor sets,
// In PoolSize size is tied to the type the number of sets/and descriptor
void DescriptorManager::createDescriptorPool(VkDevice device, uint32_t maxSets,
                                             const std::vector<VkDescriptorPoolSize> &poolSizes)
{
  // Bigger pool size,  a pool for each descriptor types
  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(DEFAULT_POOL_SIZES.size());
  poolInfo.pPoolSizes = DEFAULT_POOL_SIZES.data();
  poolInfo.maxSets = maxSets;

  if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &mDescriptorPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor pool!");
  }
};

// Todo. An APpend sets would some some problem
//  Describe the set, their binding and so on
//  Describe the type of binding each pool will be used to geenrate
void DescriptorManager::allocateDescriptorSets(VkDevice device)
{

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = mDescriptorPool;
  allocInfo.descriptorSetCount = mDescriptorSetLayouts.size();
  allocInfo.pSetLayouts = mDescriptorSetLayouts.data();

  mDescriptorSets.resize(mDescriptorSetLayouts.size());
  if (vkAllocateDescriptorSets(device, &allocInfo, mDescriptorSets.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate descriptor sets!");
  }
};


// Todo:
// In the end directly passing the material layout to use is a more reasonable solution
//
int DescriptorManager::allocateDescriptorSet(VkDevice device, int setLayout)
{
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = mDescriptorPool;
  allocInfo.pSetLayouts = &mDescriptorSetLayouts[setLayout];
  allocInfo.descriptorSetCount = 1;
  //Todo: Change the method
  int index = mDescriptorSets.size();
  mDescriptorSets.push_back(VkDescriptorSet());
  // Add the newly allocated descriptor set to the end of the vector
  if (vkAllocateDescriptorSets(device, &allocInfo, &mDescriptorSets.back()) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate descriptor set!");
  }

  return index;
}

void DescriptorManager::updateDescriptorSet(VkDevice device,
                                            const std::vector<VkWriteDescriptorSet> &writes)
{
  vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
};

// Create and pushbac, should be  append TODO: Poorlynamed
int DescriptorManager::getOrCreateSetLayout(
    VkDevice device,
    const std::vector<VkDescriptorSetLayoutBinding> &bindings)
{
  size_t hash = hashBindings(bindings);
  auto it = mLayoutHashToIndex.find(hash);
  if (it != mLayoutHashToIndex.end())
  {
    return it->second;
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();

  //Todo: Convert it using the new format since it throw at mistake anyway
  VkDescriptorSetLayout layout;
  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS){
    throw std::runtime_error("Failed to create descriptor set layout!");}

  int index = static_cast<uint32_t>(mDescriptorSetLayouts.size());
  mDescriptorSetLayouts.push_back(layout);
  mLayoutHashToIndex[hash] = index;
  return index;
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
  for (auto layout : mDescriptorSetLayouts)
  {
    if (layout != VK_NULL_HANDLE)
    {
      vkDestroyDescriptorSetLayout(device, layout, nullptr);
    }
  }

  mDescriptorSetLayouts.clear();
}

void DescriptorManager::freeDescriptorSet(VkDevice device, int index)
{
  VkResult result = vkFreeDescriptorSets(device, mDescriptorPool, 1, mDescriptorSets.data() + index);
}

void DescriptorManager::freeDescriptorSets(VkDevice device)
{
  VkResult result = vkFreeDescriptorSets(device, mDescriptorPool, mDescriptorSets.size(), mDescriptorSets.data());

  if (VK_SUCCESS != result)
  {
    throw std::runtime_error("failed to free descripto sets!");
  }

  mDescriptorSets.clear();
}

// Work witout VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
void DescriptorManager::resetDescriptorSets(VkDevice device)
{
  VkResult result = vkResetDescriptorPool(device, mDescriptorPool, 0);

  if (VK_SUCCESS != result)
  {
    throw std::runtime_error("Failed to reset descriptor pool!");
  }

  mDescriptorSets.clear();
}



  const VkDescriptorSetLayout DescriptorManager::getDescriptorLat(int index) const
    {

        return mDescriptorSetLayouts[index];
    }

    const VkDescriptorSet DescriptorManager::getSet(int index) const
    {
        return mDescriptorSets[index];
    }

    VkDescriptorSet &DescriptorManager::getSet(int index)
    {
        return mDescriptorSets[index];
    }

    const size_t DescriptorManager::getSetNb() const
    {
        return mDescriptorSets.size();
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



size_t hashBindings(const std::vector<VkDescriptorSetLayoutBinding>& bindings) {
    size_t h = 0;
    for (auto& b : bindings) {
        h ^= std::hash<uint32_t>()(b.binding)
          + std::hash<uint32_t>()(b.descriptorType) * 37
          + std::hash<uint32_t>()(b.stageFlags) * 101;
    }
    return h;
}
