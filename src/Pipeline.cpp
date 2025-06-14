#include "Pipeline.h"
#include <fstream>
#include <stdexcept>
#include "Buffer.h"

// COuld you rewrite it ? IF possible in a most flewible way ?
// For example separating shaderstage creatop,
// Makking create grpahicpipleines different ,
// Making read shader read at anothe stepr. ANything go
// Just that flexibilitty is the goal so a bigger division mayne
// NOhtin is set to stone and you're not here to say if it's good or ok enough  you want
// OPf course it is clear that create and creategrpahic are two beginning of the same function but this again may cahgen

namespace
{
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
        // VK_DYNAMIC_STATE_POLYGON_MODE
        // VK_DYNAMIC_STATE_DEPTH_BIAS
        // VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE
    };

    VkPipelineVertexInputStateCreateInfo createDefaultVertexInputState();
    VkPipelineInputAssemblyStateCreateInfo createDefaultInputAssemblyState();
    VkPipelineViewportStateCreateInfo createDefaultViewportState(VkViewport &viewport, VkRect2D &scissor, VkExtent2D &swapChainExtent);
    VkPipelineViewportStateCreateInfo createDynamicViewportState();

    VkPipelineDepthStencilStateCreateInfo createDefaultDepthStencilState();

    VkPipelineRasterizationStateCreateInfo createDefaultRasterizerState();
    VkPipelineMultisampleStateCreateInfo createDefaultMultisampleState();
    VkPipelineColorBlendStateCreateInfo createDefaultColorBlendState(VkPipelineColorBlendAttachmentState &);

}
// Pass important variable directly
bool PipelineManager::initialize(VkDevice device, VkRenderPass renderPass)
{
    mDevice = device;
    mRenderPass = renderPass;
    return true;
}

void PipelineManager::destroy(VkDevice device)
{
    if (mGraphicsPipeline)
    {
        vkDestroyPipeline(device, mGraphicsPipeline, nullptr);
    }
    if (mPipelineLayout)
    {
        vkDestroyPipelineLayout(device, mPipelineLayout, nullptr);
    }
}

bool PipelineManager::createGraphicsPipeline(const std::string &vertPath, const std::string &fragPath, const VkDescriptorSetLayout &descriportSetLayout)
{
    auto vertCode = readShaderFile(vertPath);
    auto fragCode = readShaderFile(fragPath);

    VkShaderModule vertModule = createShaderModule(vertCode);
    VkShaderModule fragModule = createShaderModule(fragCode);

    if (!vertModule || !fragModule)
    {
        return false;
    }

    auto vertStage = createShaderStage(VK_SHADER_STAGE_VERTEX_BIT, vertModule);
    auto fragStage = createShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, fragModule);
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {vertStage, fragStage};

    // Fixed-function configuration
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = createDefaultVertexInputState();
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = createDefaultInputAssemblyState();

    // Dynamic state, set the dynamic state available
    // Look into this and where it will be required to change them
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineViewportStateCreateInfo viewportState = createDynamicViewportState();
    VkPipelineRasterizationStateCreateInfo rasterizer = createDefaultRasterizerState();
    VkPipelineDepthStencilStateCreateInfo depthStencil = createDefaultDepthStencilState();
    VkPipelineMultisampleStateCreateInfo multisampling = createDefaultMultisampleState();
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    VkPipelineColorBlendStateCreateInfo colorBlending = createDefaultColorBlendState(colorBlendAttachment);

    // Uniform value are set here so it would be better to have multiple stufff here
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pSetLayouts = &descriportSetLayout;
    pipelineLayoutInfo.setLayoutCount = 1;

    // Push constant too ?
    if (vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, nullptr, &mPipelineLayout) != VK_SUCCESS)
        return false;

    // Push it to a function with a better way to set the count
    VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    // Vertex and Fragment
    pipelineInfo.stageCount = shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.layout = mPipelineLayout;
    pipelineInfo.renderPass = mRenderPass;
    pipelineInfo.subpass = 0;

    // Derivated Pipline cocnept nto used
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1;              // Optional

    if (vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mGraphicsPipeline) != VK_SUCCESS)
    {
        vkDestroyShaderModule(mDevice, vertModule, nullptr);
        vkDestroyShaderModule(mDevice, fragModule, nullptr);
        return false;
    }

    vkDestroyShaderModule(mDevice, vertModule, nullptr);
    vkDestroyShaderModule(mDevice, fragModule, nullptr);

    return true;
}

VkPipelineShaderStageCreateInfo PipelineManager::createShaderStage(VkShaderStageFlagBits stage, VkShaderModule module)
{
    VkPipelineShaderStageCreateInfo stageInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stageInfo.stage = stage;
    stageInfo.module = module;
    stageInfo.pName = "main";
    stageInfo.pSpecializationInfo = nullptr;

    return stageInfo;
}

VkShaderModule PipelineManager::createShaderModule(const std::vector<char> &code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    // Not make it a uint32 t from the beginning
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(mDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create shader module!");
    }

    return shaderModule;
}

std::vector<char> PipelineManager::readShaderFile(const std::string &filename)
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

namespace
{

    VkPipelineVertexInputStateCreateInfo createDefaultVertexInputState()
    {
        /*
    This describes the format of the vertex data that will be passed to the vertex shader.
    Bindings: spacing between data and whether the data is per-vertex or per-instance (see instancing)
    Attribute descriptions: type of the attributes passed to the vertex shader, which binding to load them from and at which offset
    */
        VertexFormatRegistry::initBase();
        return VertexFormatRegistry::getFormat("pos_color_interleaved").toCreateInfo();
    }

    VkPipelineInputAssemblyStateCreateInfo createDefaultInputAssemblyState()
    {
        /*
The VkPipelineInputAssemblyStateCreateInfo struct describes two things: what kind of geometry will be drawn from the vertices and if primitive restart should be enabled.
Normally, the vertices are loaded from the vertex buffer by index in sequential order, but with an element buffer you can specify the indices to use yourself. This allows you to perform optimizations like reusing vertices. If you set the primitiveRestartEnable member to VK_TRUE, then it's possible to break up lines and triangles in the _STRIP topology modes by using a special index of 0xFFFF or 0xFFFFFFFF.
*/

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        // Also Line and Point can be useful
        // Strip relevant struct
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        return inputAssembly;
    }

    VkPipelineViewportStateCreateInfo createDynamicViewportState()
    {

        VkPipelineViewportStateCreateInfo info{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        info.viewportCount = 1;
        info.scissorCount = 1;
        return info;
    }

    VkPipelineViewportStateCreateInfo createDefaultViewportState(VkViewport &viewport, VkRect2D &scissor, VkExtent2D &swapChainExtent)
    {
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;

        VkPipelineViewportStateCreateInfo info{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        info.viewportCount = 1;
        info.pViewports = &viewport;
        info.scissorCount = 1;
        info.pScissors = &scissor;
        return info;
    }

    VkPipelineDepthStencilStateCreateInfo createDefaultDepthStencilState()
    {
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;

        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        
        //Unecessary for now
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f;

        //Not for now
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {}; 
        depthStencil.back = {};   

        return depthStencil;
    }

    VkPipelineRasterizationStateCreateInfo createDefaultRasterizerState()
    {
        // Rastzerizer == Fragment Shader or right before fonctionality
        // Depth testing
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        // rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f;          // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f;    // Optional
        return rasterizer;
    }

    VkPipelineMultisampleStateCreateInfo createDefaultMultisampleState()
    {
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.sampleShadingEnable = VK_FALSE;
        return multisampling;
    }

    VkPipelineColorBlendStateCreateInfo createDefaultColorBlendState(VkPipelineColorBlendAttachmentState &colorBlendAttachment)
    {
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                              VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
        colorBlending.attachmentCount = 1;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.pAttachments = &colorBlendAttachment;
        return colorBlending;
    }

}