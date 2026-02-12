#include <fstream>
#include "utils/PipelineHelper.h"
// Todo: Order of the category in the header is differnent
// Pipeline

VkPipelineShaderStageCreateInfo vkUtils::Shaders::createShaderStage(VkShaderStageFlagBits stage, VkShaderModule module)
{
    VkPipelineShaderStageCreateInfo stageInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stageInfo.stage = stage;
    stageInfo.module = module;
    // Notes: 
    // Unique shader with main function as difference for specific treatment ?
    stageInfo.pName = "main";
    stageInfo.pSpecializationInfo = nullptr;

    return stageInfo;
}

VkShaderModule vkUtils::Shaders::createShaderModule(VkDevice device, const std::vector<char> &code)
{
    if (code.empty())
    {
        return VK_NULL_HANDLE;
    };

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    // Not make it a uint32 t from the beginning
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());
    createInfo.flags = 0;

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module!");
    }

    return shaderModule;
}

std::vector<char> vkUtils::Shaders::readShaderFile(const std::string &filename)
{
    // Constructor to create streamer + open
    std::ifstream input(filename, std::ios::ate | std::ios::binary);
    if (!input.is_open())
    {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    // Allocate correct size y reading from the end
    size_t fileSize = (size_t)input.tellg();
    std::vector<char> buffer(fileSize);

    input.seekg(0);
    input.read(buffer.data(), fileSize);

    input.close();

    return buffer;
}

//Todo:
//Temp position 
PipelineBuilder &PipelineBuilder::setShaders(const PipelineShaderConfig &shaders)
{
    mConfig.shaders.vertShaderPath = shaders.vertShaderPath;
    mConfig.shaders.fragShaderPath = shaders.fragShaderPath;
    mConfig.shaders.geomShaderPath = shaders.geomShaderPath;
    mConfig.shaders.computeShaderPath = shaders.computeShaderPath;
    return *this;
}

PipelineBuilder &PipelineBuilder::setRenderPass(VkRenderPass renderPass, uint32_t subpass)
{
    mConfig.pass.renderPass = renderPass;
    mConfig.pass.subpass = subpass;
    return *this;
}

// Todo: element passed and how they are stored could be better
//Todo: Consider ViewMask and Subpass at some point
PipelineBuilder &PipelineBuilder::setDynamicRenderPass(const std::vector<VkFormat> &colors,
                                                       VkFormat depth, VkFormat stencil, uint32_t vMask)
{
    mConfig.pass.renderPass = VK_NULL_HANDLE;
    mConfig.pass.subpass = 0;

    mConfig.pass.colorAttachments = colors;
    mConfig.pass.depthAttachmentFormat = depth;
    // mConfig.pass.stencilAttachmentFormat = stencil;
    // mConfig.pass.ViewMask = vMask;
    return *this;
}

PipelineBuilder &PipelineBuilder::setUniform(const PipelineLayoutConfig &uniform)
{
    mConfig.uniform.descriptorSetLayouts = uniform.descriptorSetLayouts;
    mConfig.uniform.pushConstants = uniform.pushConstants;

    return *this;
}

PipelineBuilder &PipelineBuilder::setDynamicStates(const std::vector<VkDynamicState> &states)
{
    mConfig.dynamicStates = states;
    return *this;
}

PipelineBuilder &PipelineBuilder::setRasterizer(const PipelineRasterConfig &raster)
{
    mConfig.raster = raster;
    return *this;
}

PipelineBuilder &PipelineBuilder::setBlend(const PipelineBlendConfig &blend)
{
    mConfig.blend = blend;
    return *this;
}

PipelineBuilder &PipelineBuilder::setDepthConfig(const PipelineDepthConfig &depth)
{
    mConfig.depth = depth;
    return *this;
}

PipelineBuilder &PipelineBuilder::setInputConfig(const PipelineVertexInputConfig &input)
{
    mConfig.input = input;
    return *this;
}

PipelineBuilder &PipelineBuilder::setDepthTest(bool enable){
        mConfig.depth.enableDetphTest = enable;
    return *this;
};




// Descriptor

// RenderPass