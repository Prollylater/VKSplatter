#pragma once
#include "BaseVk.h"


class PipelineManager {
public:
    PipelineManager() = default;
    //Tried actually using but apparently double destroy is a risk
    //Destroy might be followed by a setting object to VK_NULL_Handle
    ~PipelineManager() = default;

    VkPipeline getPipeline() const { return mGraphicsPipeline; }
    VkPipelineLayout getPipelineLayout() const { return mPipelineLayout; }

    bool initialize(VkDevice device, VkRenderPass renderPass);
    bool createGraphicsPipeline(const std::string &vertPath, const std::string &fragPath, const VkDescriptorSetLayout& descriportSetLayout);
    void destroy(VkDevice device);
    bool loadShaders(const std::string& vertPath, const std::string& fragPath);

private:
    VkDevice mDevice = VK_NULL_HANDLE;
    VkRenderPass mRenderPass = VK_NULL_HANDLE;

    VkPipeline mGraphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;

    //Could keep them to rebuild other pipelien ?
    //Not sure
    //VkShaderModule mVertShaderModule = VK_NULL_HANDLE;
    //VkShaderModule mFragShaderModule = VK_NULL_HANDLE;
    
    
    VkShaderModule createShaderModule(const std::vector<char>& code);
    std::vector<char> readShaderFile(const std::string& path);
    VkPipelineShaderStageCreateInfo createShaderStage(VkShaderStageFlagBits stage, VkShaderModule module);

};





//Compiling shader on the fly in Oopengl was the usual
//CHeck if equivalent is possible in vulkan