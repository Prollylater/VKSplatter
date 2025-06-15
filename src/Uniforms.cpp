
#include "Uniforms.h"
#include "Buffer.h"

///////////////////////////////////
// Buffer
///////////////////////////////////

constexpr uint32_t BINDING_UBO = 0;
constexpr uint32_t BINDING_SAMPLER = 1;

void DescriptorManager::createDescriptorPool(VkDevice device)
{
    // Bigger pool size,  a pool for each descriptor types
    // Careful with those
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(ContextVk::contextInfo.MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(ContextVk::contextInfo.MAX_FRAMES_IN_FLIGHT);

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
#include "Texture.h"

void DescriptorManager::createDescriptorSets(VkDevice device, TextureManager &texutreM)
{
    // Uniform Buffer that are not bound
    std::vector<VkDescriptorSetLayout> layouts(ContextVk::contextInfo.MAX_FRAMES_IN_FLIGHT, mDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = mDescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(ContextVk::contextInfo.MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    mDescriptorSets.resize(ContextVk::contextInfo.MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, mDescriptorSets.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < ContextVk::contextInfo.MAX_FRAMES_IN_FLIGHT; i++)
    {
        //Set UBO
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = mUniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

      
        //Set location bnding
        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = mDescriptorSets[i];
        descriptorWrites[0].dstBinding = BINDING_UBO;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        // Some way to directtly connect the binding
          VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = texutreM.getTextureView();
        imageInfo.sampler = texutreM.getSampler();

        //This should be somewher eelse ?
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = mDescriptorSets[i];
        descriptorWrites[1].dstBinding = BINDING_SAMPLER;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;
        //           descriptorWrite.pTexelBufferView = nullptr; // Optional
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
   
   
    }
};


//TODO
void DescriptorManager::createUniformBuffers(VkDevice device, VkPhysicalDevice physDevice)
{

    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    mUniformBuffers.resize(ContextVk::contextInfo.MAX_FRAMES_IN_FLIGHT);
    mUniformBuffersMemory.resize(ContextVk::contextInfo.MAX_FRAMES_IN_FLIGHT);
    mUniformBuffersMapped.resize(ContextVk::contextInfo.MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < ContextVk::contextInfo.MAX_FRAMES_IN_FLIGHT; i++)
    {
        createBuffer(mUniformBuffers[i], mUniformBuffersMemory[i], device, physDevice, bufferSize,
                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        // Persistent Mapping
        vkMapMemory(device, mUniformBuffersMemory[i], 0, bufferSize, 0, &mUniformBuffersMapped[i]);
    }
}

//Should be in scene i guess
/*
void DescriptorManager::updateUniformBuffers(uint32_t currentImage, VkExtent2D swapChainExtent)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);

    ubo.proj[1][1] *= -1;

    memcpy(mUniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
};
*/
void DescriptorManager::createDescriptorSetLayout(VkDevice device)
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &mDescriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void DescriptorManager::destroyDescriptorLayout(VkDevice device)
{

    vkDestroyDescriptorSetLayout(device, mDescriptorSetLayout, nullptr);
}

void DescriptorManager::destroyUniformBuffer(VkDevice device)
{

    for (size_t i = 0; i < ContextVk::contextInfo.MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyBuffer(device, mUniformBuffers[i], nullptr);
        vkFreeMemory(device, mUniformBuffersMemory[i], nullptr);
    }
}

// TOOD: Chang eother at VK_NULL_HANDLE
//  Destroy on not real object nor null handle can have weird conseuqeujces