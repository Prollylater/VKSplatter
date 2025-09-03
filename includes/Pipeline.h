#pragma once
#include "BaseVk.h"


#include <array>

enum class FragmentShaderType {
    Basic,
    Blinn_Phong,
    PBR
};


struct PipelineConfig {
    std::string vertShaderPath;
    std::string fragShaderPath;
    VkRenderPass renderPass;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    uint32_t subpass = 0;

    // Optional: Flags or enums for blending, culling, etc.
    bool enableDepthTest = true;
    bool enableAlphaBlend = false;

    // Optional: Dynamic states
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
};


class PipelineManager {
public:
    PipelineManager() = default;
    //Tried actually using but apparently double destroy is a risk
    //Destroy might be followed by a setting object to VK_NULL_Handle
    ~PipelineManager() = default;

    VkPipeline getPipeline() const { return mGraphicsPipeline[0]; }
    VkPipelineLayout getPipelineLayout() const { return mPipelineLayout; }

    bool initialize(VkDevice device, VkRenderPass renderPass);
    bool createGraphicsPipeline(VkDevice,VkRenderPass, const PipelineConfig&, const VkDescriptorSetLayout& );
    void destroy(VkDevice device);

private:
  //  VkDevice mDevice = VK_NULL_HANDLE;
    //VkRenderPass mRenderPass = VK_NULL_HANDLE;

    std::array<VkPipeline, 1> mGraphicsPipeline = {VK_NULL_HANDLE};

    //Still using a unique render pass
    VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;

    //Could keep them to rebuild other pipelien ?
    //Not sure
    //VkShaderModule mVertShaderModule = VK_NULL_HANDLE;
    //VkShaderModule mFragShaderModule = VK_NULL_HANDLE;
    
    
    VkShaderModule createShaderModule(VkDevice device,const std::vector<char>& code);
    std::vector<char> readShaderFile(const std::string& path);
    VkPipelineShaderStageCreateInfo createShaderStage(VkShaderStageFlagBits stage, VkShaderModule module);

};

//Shader Module Abstraction ?
//ShaderModule → wraps VkShaderModule.



//Compiling shader on the fly in Oopengl was the usual
//CHeck if equivalent is possible in vulkan