#include "Pipeline.h"
#include <fstream>
#include <stdexcept>
#include "Buffer.h"

// COuld you rewrite it ? IF possible in a most flewible way ?
// For example separating shaderstage creation,
/*
//The graphic Pipeline can  be divided into
Shader stages: the shader modules that define the functionality of the programmable stages of the graphics pipeline
Fixed-function state: all of the structures that define the fixed-function stages of the pipeline, like input assembly, rasterizer, viewport and color blending
Pipeline layout: the uniform and push values referenced by the shader that can be updated at draw time
Render pass: the attachments referenced by the pipeline stages and their usage
*/

namespace
{

    VkPipelineVertexInputStateCreateInfo createDefaultVertexInputState();
    VkPipelineInputAssemblyStateCreateInfo createDefaultInputAssemblyState();
    VkPipelineViewportStateCreateInfo createDefaultViewportState(VkViewport &viewport, VkRect2D &scissor, VkExtent2D &swapChainExtent);
    VkPipelineViewportStateCreateInfo createDynamicViewportState();

    VkPipelineDepthStencilStateCreateInfo createDefaultDepthStencilState();
    VkPipelineDepthStencilStateCreateInfo createDisabledDepthStencilState();
    VkPipelineRasterizationStateCreateInfo createDefaultRasterizerState();
    VkPipelineMultisampleStateCreateInfo createDefaultMultisampleState();
    VkPipelineColorBlendStateCreateInfo createDefaultColorBlendState(VkPipelineColorBlendAttachmentState &);

}
// Pass important variable directly
bool PipelineManager::initialize(VkDevice device, VkRenderPass renderPass)
{
    // mDevice = device;
    // mRenderPass = renderPass;
    return true;
}

void PipelineManager::destroy(VkDevice device)
{
    if (mGraphicsPipeline[0])
    {
        vkDestroyPipeline(device, mGraphicsPipeline[0], nullptr);
    }
    if (mPipelineLayout)
    {
        vkDestroyPipelineLayout(device, mPipelineLayout, nullptr);
    }
}
bool PipelineManager::createGraphicsPipeline(VkDevice device, VkRenderPass renderPass, const PipelineConfig &config, const VkDescriptorSetLayout &descriportSetLayout,
                                             const VkPushConstantRange &pushCst)
{
    auto vertCode = readShaderFile(config.vertShaderPath);
    auto fragCode = readShaderFile(config.fragShaderPath);

    VkShaderModule vertModule = createShaderModule(device, vertCode);
    VkShaderModule fragModule = createShaderModule(device, fragCode);

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
    dynamicState.dynamicStateCount = static_cast<uint32_t>(config.dynamicStates.size());
    dynamicState.pDynamicStates = config.dynamicStates.data();

    VkPipelineViewportStateCreateInfo viewportState = createDynamicViewportState();
    VkPipelineRasterizationStateCreateInfo rasterizer = createDefaultRasterizerState();
    VkPipelineDepthStencilStateCreateInfo depthStencil = config.enableDepthTest ? createDefaultDepthStencilState() : createDisabledDepthStencilState();
    VkPipelineMultisampleStateCreateInfo multisampling = createDefaultMultisampleState();
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    VkPipelineColorBlendStateCreateInfo colorBlending = createDefaultColorBlendState(colorBlendAttachment);

    // Uniform value are set here so it would be better to have multiple stufff here
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pSetLayouts = &descriportSetLayout;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushCst;
    pipelineLayoutInfo.pushConstantRangeCount = 1;

    // Push constant too ?
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &mPipelineLayout) != VK_SUCCESS)
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
    // pipelineInfo.pTessellationState
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.layout = mPipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    // Derivated Pipline cocnept nto used
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1;              // Optional

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mGraphicsPipeline[0]) != VK_SUCCESS)
    {
        vkDestroyShaderModule(device, vertModule, nullptr);
        vkDestroyShaderModule(device, fragModule, nullptr);
        return false;
    }

    vkDestroyShaderModule(device, vertModule, nullptr);
    vkDestroyShaderModule(device, fragModule, nullptr);

    return true;
}

VkPipelineShaderStageCreateInfo PipelineManager::createShaderStage(VkShaderStageFlagBits stage, VkShaderModule module)
{
    VkPipelineShaderStageCreateInfo stageInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stageInfo.stage = stage;
    stageInfo.module = module;
    // Shader main function
    // Todo: But their may be some interesting doable in this
    // Unique shader with main function as difference for specific treatment ?
    stageInfo.pName = "main";
    stageInfo.pSpecializationInfo = nullptr;

    return stageInfo;
}

VkShaderModule PipelineManager::createShaderModule(VkDevice device, const std::vector<char> &code)
{
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

        // Todo: SHould be relevant to the type of mesh we will manage
        VertexFlags flag = static_cast<VertexFlags>(Vertex_Pos | Vertex_Normal | Vertex_UV | Vertex_Color);
        return VertexFormatRegistry::getFormat(flag).toCreateInfo();
    }

    VkPipelineInputAssemblyStateCreateInfo createDefaultInputAssemblyState()
    {
        // Only with geometry shader
        /*
The VkPipelineInputAssemblyStateCreateInfo struct describes two things:
what kind of geometry will be drawn from the vertices and if primitive restart should be enabled.
Normally, the vertices are loaded from the vertex buffer by index in sequential order,
but with an element buffer you can specify the indices to use yourself.
This allows you to perform optimizations like reusing vertices.
If you set the primitiveRestartEnable member to VK_TRUE, then it's possible to break up lines and triangles i
in the _STRIP topology modes by using a special index of 0xFFFF or 0xFFFFFFFF.
*/

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        // Listincompatible with Restart
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        // Also Line and Point can be useful
        // Strip relevant struct
        // Restart ?
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        return inputAssembly;
    }

    VkPipelineViewportStateCreateInfo createDynamicViewportState()
    {

        // Connected to confing dynamic state array ?
        VkPipelineViewportStateCreateInfo info{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
        info.viewportCount = 1;
        info.scissorCount = 1;
        // Could handle multiviewport
        // ToDo: Later later
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

        // Unecessary for now
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f; // Optional
        depthStencil.maxDepthBounds = 1.0f;

        // Not for now
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {};
        depthStencil.back = {};

        return depthStencil;
    }

    VkPipelineDepthStencilStateCreateInfo createDisabledDepthStencilState()
    {
        VkPipelineDepthStencilStateCreateInfo disabledDepthStencil{};
        disabledDepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        disabledDepthStencil.depthTestEnable = VK_FALSE;
        disabledDepthStencil.depthWriteEnable = VK_FALSE;
        disabledDepthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
        disabledDepthStencil.depthBoundsTestEnable = VK_FALSE;
        disabledDepthStencil.stencilTestEnable = VK_FALSE;
        return disabledDepthStencil;
    }

    VkPipelineRasterizationStateCreateInfo createDefaultRasterizerState()
    {
        // Rastzerizer == Fragment Shader or right before fonctionality
        // Depth testing
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        // Lines and Point with  fillModeNonSolid
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        // rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        // TOdo: This for example. Am i suppose to have different pipelines for various type of object ?
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
        multisampling.minSampleShading = 1.0f;          // Optional
        multisampling.pSampleMask = nullptr;            // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE;      // O
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