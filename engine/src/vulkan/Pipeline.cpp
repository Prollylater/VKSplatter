#include "Pipeline.h"
#include "utils/PipelineHelper.h"
#include <fstream>
#include <stdexcept>

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
    VkPipelineVertexInputStateCreateInfo createDefaultVertexInputState(const PipelineVertexInputConfig &);
    VkPipelineInputAssemblyStateCreateInfo createDefaultInputAssemblyState(const PipelineVertexInputConfig &);
    VkPipelineViewportStateCreateInfo createDynamicViewportState();
    VkPipelineViewportStateCreateInfo createDefaultViewportState(VkViewport &viewport, VkRect2D &scissor, VkExtent2D &swapChainExtent);
    VkPipelineDepthStencilStateCreateInfo createDefaultDepthStencilState(const PipelineDepthConfig &info);
    VkPipelineRasterizationStateCreateInfo createDefaultRasterizerState(const PipelineRasterConfig &info);
    VkPipelineMultisampleStateCreateInfo createDefaultMultisampleState();
    VkPipelineColorBlendStateCreateInfo createDefaultColorBlendState(const PipelineBlendConfig &info, VkPipelineColorBlendAttachmentState &);

}

void PipelineManager::initialize(VkDevice device, const std::string &cacheFile)
{
    mDevice = device;

    std::vector<char> cacheData;
    if (cacheFile.empty())
    {
        return;
    };

    std::ifstream in(cacheFile, std::ios::binary | std::ios::ate);
    if (in)
    {
        size_t size = (size_t)in.tellg();
        cacheData.resize(size);
        in.seekg(0);
        in.read(cacheData.data(), size);
    }

    VkPipelineCacheCreateInfo cacheInfo{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    cacheInfo.pInitialData = cacheData.empty() ? nullptr : cacheData.data();
    cacheInfo.initialDataSize = cacheData.size();

    // Add Manipulation like multi threading etc...
    if (vkCreatePipelineCache(device, &cacheInfo, nullptr, &mPipelineCache) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline cache!");
    }
}

// Get or create
int PipelineManager::createPipelineWithBuilder(VkDevice device, const PipelineBuilder &builder)
{
    size_t hash = builder.computehash();
    auto it = mIndexByKey.find(hash);
    if (it != mIndexByKey.end())
    {
        return it->second;
    }

    auto [pipeline, layout] = builder.build(mDevice, mPipelineCache);
    int index = mPipelines.size();
    mPipelines.push_back({pipeline, layout});
    return index;
}

void PipelineManager::destroyPipeline(VkDevice device, uint32_t index)
{
    if (mPipelines[index].pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device, mPipelines[index].pipeline, nullptr);
        mPipelines[index].pipeline = VK_NULL_HANDLE;
    }

    if (mPipelines[index].layout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device, mPipelines[index].layout, nullptr);
        mPipelines[index].layout = VK_NULL_HANDLE;
    }
}
void PipelineManager::destroy(VkDevice device)
{
    for (auto entry : mPipelines)
    {
        if (entry.pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(device, entry.pipeline, nullptr);
        }

        if (entry.layout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(device, entry.layout, nullptr);
        }
    }
    mPipelines.clear();

    if (mPipelineCache != VK_NULL_HANDLE)
    {
        vkDestroyPipelineCache(device, mPipelineCache, nullptr);
        mPipelineCache = VK_NULL_HANDLE;
    }
}

// Todo: bounds check ?
void PipelineManager::reloadShaders(VkDevice device, int pipelineIndex, const PipelineBuilder builder)
{
    destroy(device);
    createPipelineWithBuilder(device, builder);
}

// Pipeline Cache then ?

std::vector<unsigned char> PipelineManager::getCacheData(VkDevice device)
{

    size_t cacheSize = 0;
    if (vkGetPipelineCacheData(device, mPipelineCache, &cacheSize, nullptr) != VK_SUCCESS || cacheSize == 0)
    {
        return std::vector<unsigned char>();
    }
    std::vector<unsigned char> cachedData(cacheSize);

    vkGetPipelineCacheData(device, mPipelineCache, &cacheSize, cachedData.data()) != VK_SUCCESS;

    return cachedData;
}

void PipelineManager::dumpCacheToFile(const std::string &path, VkDevice device)
{
    size_t size = 0;
    // Double call
    VkResult result = vkGetPipelineCacheData(device, mPipelineCache, &size, nullptr);
    if (result != VK_SUCCESS || size == 0)
    {
        std::cerr << "Failed to get pipeline cache data size: " << result << std::endl;
        return;
    }

    std::vector<char> cachedData(size);
    if (vkGetPipelineCacheData(device, mPipelineCache, &size, cachedData.data()) != VK_SUCCESS)
    {
        std::cerr << "Failed to get pipeline cache data: " << result << std::endl;
        return;
    }

    // Write the data to the file
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open())
    {
        std::cerr << "Failed to open file for writing: " << path << std::endl;
        return;
    }

    out.write(cachedData.data(), size);
    if (!out)
    {
        std::cerr << "Failed to write data to file: " << path << std::endl;
    }
    else
    {
        std::cout << "Pipeline cache data successfully written to " << path << std::endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////
// BUILDER
///////////////////////////////////////////////////////////////////////////////////

// --------------------- Build Method ------- -------------

std::pair<VkPipeline, VkPipelineLayout> PipelineBuilder::build(VkDevice device, VkPipelineCache cache) const
{
    const bool hasCompute = !mConfig.shaders.computeShaderPath.empty();

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;

    // Branch out if a compute Shadder is present
    if (hasCompute)
    {
        VkPipeline pipeline;
        VkPipelineLayout pipelineLayout;

        // Create compute shader module
        VkShaderModule computeModule = vkUtils::Shaders::createShaderModule(device, vkUtils::Shaders::readShaderFile(mConfig.shaders.computeShaderPath));

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.pSetLayouts = mConfig.uniform.descriptorSetLayouts.data();
        pipelineLayoutInfo.setLayoutCount = mConfig.uniform.descriptorSetLayouts.size();
        pipelineLayoutInfo.pPushConstantRanges = mConfig.uniform.pushConstants.data();
        pipelineLayoutInfo.pushConstantRangeCount = mConfig.uniform.pushConstants.size();

        // Push constant too ?
        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        {
        }

        // Build VkComputePipelineCreateInfo with just pipelineLayout and computeModule
        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.stage = vkUtils::Shaders::createShaderStage(VK_SHADER_STAGE_COMPUTE_BIT, computeModule);

        vkCreateComputePipelines(device, cache, 1, &pipelineInfo, nullptr, &pipeline);

        vkDestroyShaderModule(device, computeModule, nullptr);
        return {pipeline, pipelineLayout};
    }

    const bool hasVertex = !mConfig.shaders.vertShaderPath.empty();
    const bool hasFragment = !mConfig.shaders.fragShaderPath.empty();

    auto readShaderLambda = [](const std::string &filePath) -> std::vector<char>
    {
        return filePath.empty() ? std::vector<char>() : std::move(vkUtils::Shaders::readShaderFile(filePath));
    };

    std::vector<char> vertCode = readShaderLambda(mConfig.shaders.vertShaderPath);
    std::vector<char> fragCode = readShaderLambda(mConfig.shaders.fragShaderPath);
    std::vector<char> geomCode = readShaderLambda(mConfig.shaders.geomShaderPath);

    VkShaderModule vertModule = vkUtils::Shaders::createShaderModule(device, vertCode);
    VkShaderModule fragModule = vkUtils::Shaders::createShaderModule(device, fragCode);
    VkShaderModule geomModule = vkUtils::Shaders::createShaderModule(device, geomCode);

    if (vertModule == VK_NULL_HANDLE && fragModule == VK_NULL_HANDLE && geomModule == VK_NULL_HANDLE)
    {
        throw std::runtime_error("No shaders provided for pipeline.");
    }

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    if (vertModule != VK_NULL_HANDLE)
    {
        shaderStages.push_back(vkUtils::Shaders::createShaderStage(VK_SHADER_STAGE_VERTEX_BIT, vertModule));
    }
    if (fragModule != VK_NULL_HANDLE)
    {
        shaderStages.push_back(vkUtils::Shaders::createShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, fragModule));
    }
    if (geomModule != VK_NULL_HANDLE)
    {
        shaderStages.push_back(vkUtils::Shaders::createShaderStage(VK_SHADER_STAGE_GEOMETRY_BIT, geomModule));
    }

    // Dynamic state, set the dynamic state available
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(mConfig.dynamicStates.size());
    dynamicState.pDynamicStates = mConfig.dynamicStates.data();

    // Fixed-function configuration
    // Relevant only when we have Vertex Shader
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = createDefaultVertexInputState(mConfig.input);
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = createDefaultInputAssemblyState(mConfig.input);
    // Fixed-function configuration

    // Most are only relevant with Fragment Shader
    VkPipelineViewportStateCreateInfo viewportState = createDynamicViewportState();
    VkPipelineRasterizationStateCreateInfo rasterizer = createDefaultRasterizerState(mConfig.raster);
    VkPipelineDepthStencilStateCreateInfo depthStencil = createDefaultDepthStencilState(mConfig.depth);
    VkPipelineMultisampleStateCreateInfo multisampling = createDefaultMultisampleState();
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    VkPipelineColorBlendStateCreateInfo colorBlending = createDefaultColorBlendState(mConfig.blend, colorBlendAttachment);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pSetLayouts = mConfig.uniform.descriptorSetLayouts.data();
    pipelineLayoutInfo.setLayoutCount = mConfig.uniform.descriptorSetLayouts.size();
    pipelineLayoutInfo.pPushConstantRanges = mConfig.uniform.pushConstants.data();
    pipelineLayoutInfo.pushConstantRangeCount = mConfig.uniform.pushConstants.size();

    // Push constant too ?
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {

        vkDestroyShaderModule(device, vertModule, nullptr);
        vkDestroyShaderModule(device, fragModule, nullptr);

        return {VK_NULL_HANDLE, VK_NULL_HANDLE};
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    // Vertex and Fragment
    pipelineInfo.stageCount = shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pDynamicState = &dynamicState;
    // pipelineInfo.flags = 0;

    // Tesselation
    //  pipelineInfo.pTessellationState

    // Rasterization
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;

    // Rasterization + Depth Stencil
    pipelineInfo.pDepthStencilState = &depthStencil;

    pipelineInfo.layout = pipelineLayout;
    VkPipelineRenderingCreateInfo pipelineRender;
    if (mConfig.pass.renderPass != VK_NULL_HANDLE)
    {
        pipelineInfo.renderPass = mConfig.pass.renderPass;
        pipelineInfo.subpass = mConfig.pass.subpass;
    }
    else // Dynamic rendering first introduction
    {
        pipelineRender = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
                          .colorAttachmentCount = static_cast<uint32_t>(mConfig.pass.colorAttachments.size()),
                          .pColorAttachmentFormats = mConfig.pass.colorAttachments.data(),
                          .depthAttachmentFormat = mConfig.pass.depthAttachmentFormat};
        pipelineInfo.pNext = &pipelineRender;
        pipelineInfo.renderPass = nullptr;
    }

    // Derivated Pipline cocnept not used
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(device, cache, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
    {
        vkDestroyShaderModule(device, vertModule, nullptr);
        vkDestroyShaderModule(device, fragModule, nullptr);
        return {VK_NULL_HANDLE, VK_NULL_HANDLE};
    }

    vkDestroyShaderModule(device, vertModule, nullptr);
    vkDestroyShaderModule(device, fragModule, nullptr);
    return {pipeline, pipelineLayout};
}

namespace
{

    VkPipelineVertexInputStateCreateInfo createDefaultVertexInputState(const PipelineVertexInputConfig &info)
    {
        /*
        This describes the format of the vertex data that will be passed to the vertex shader.
        Bindings: spacing between data and whether the data is per-vertex or per-instance (see instancing)
        Attribute descriptions: type of the attributes passed to the vertex shader, which binding to load them from and at which offset
        */
        return info.vertexFormat.toCreateInfo();
    }

    VkPipelineInputAssemblyStateCreateInfo createDefaultInputAssemblyState(const PipelineVertexInputConfig &info)
    {
        /*
        Normally, the vertices are loaded from the vertex buffer by index in sequential order,
        but with an element buffer you can specify the indices to use yourself.
        This allows you to perform optimizations like reusing vertices.
        If you set the primitiveRestartEnable member to VK_TRUE, then it's possible to break up lines and triangles i
        in the _STRIP topology modes by using a special index of 0xFFFF or 0xFFFFFFFF.
        */

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
        // Listincompatible with Restart
        inputAssembly.topology = info.topolpgy;
        inputAssembly.primitiveRestartEnable = info.primitiveRestartEnable;
        return inputAssembly;
    }

    VkPipelineViewportStateCreateInfo createDynamicViewportState()
    {
        // Could handle multiviewport
        // ToDo: Later later
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

    VkPipelineDepthStencilStateCreateInfo createDefaultDepthStencilState(const PipelineDepthConfig &info)
    {
        VkPipelineDepthStencilStateCreateInfo depthStencil{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
        depthStencil.depthTestEnable = info.enableDetphTest ? VK_TRUE : VK_FALSE;
        depthStencil.depthWriteEnable = info.enableDetphTest ? VK_TRUE : VK_FALSE;
        depthStencil.depthCompareOp = info.enableDetphTest ? VK_COMPARE_OP_LESS : VK_COMPARE_OP_ALWAYS;
        depthStencil.minDepthBounds = 0.0f;
        depthStencil.maxDepthBounds = 1.0f;
        depthStencil.front = {};
        depthStencil.back = {};

        return depthStencil;
    }

    VkPipelineRasterizationStateCreateInfo createDefaultRasterizerState(const PipelineRasterConfig &info)
    {
        // Rastzerizer == Fragment Shader or right before fonctionality
        // Depth testing
        VkPipelineRasterizationStateCreateInfo rasterizer{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
        rasterizer.polygonMode = info.polygonMode;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = info.cullMode;
        rasterizer.frontFace = info.frontFace;

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

    VkPipelineColorBlendStateCreateInfo createDefaultColorBlendState(const PipelineBlendConfig &info, VkPipelineColorBlendAttachmentState &colorBlendAttachment)
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