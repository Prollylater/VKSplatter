#pragma once
#include "BaseVk.h"

#include <array>
#include "VertexDescriptions.h"

enum class FragmentShaderType
{
    Basic,
    Blinn_Phong,
    PBR
};

struct PipelineShaderConfig
{
    std::string vertShaderPath;
    std::string fragShaderPath;
    std::string geomShaderPath;
    std::string computeShaderPath;
};

struct PipelineLayoutConfig
{
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    std::vector<VkPushConstantRange> pushConstants;
};

struct PipelineRasterConfig
{
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
    VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    // Lines and Point with fillModeNonSolid available with gpu features
    VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
};

struct PipelineBlendConfig
{
    bool enableAlphaBlend = false;
};

struct PipelineDepthConfig
{
    bool enableDetphTest = true;
};

struct PipelineVertexInputConfig
{
    VkPrimitiveTopology topolpgy = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;                   
    VkBool32 primitiveRestartEnable = VK_FALSE;
    VertexFormat vertexFormat;
};

struct PipelineConfig
{
    PipelineShaderConfig shaders;
    PipelineRasterConfig raster;
    PipelineBlendConfig blend;
    PipelineDepthConfig depth;
    PipelineVertexInputConfig input;
    PipelineLayoutConfig uniform;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    uint32_t subpass = 0;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR};
};

class PipelineBuilder
{
public:
    PipelineBuilder &setShaders(const PipelineShaderConfig &);
    PipelineBuilder &setDynamicStates(const std::vector<VkDynamicState> &states);
    PipelineBuilder &setRasterizer(const PipelineRasterConfig &);
    PipelineBuilder &setRenderPass(VkRenderPass renderPass, uint32_t subpass = 0);
    PipelineBuilder &setUniform(const PipelineLayoutConfig &uniform);
    PipelineBuilder &setBlend(const PipelineBlendConfig &);
    PipelineBuilder &setDepthConfig(const PipelineDepthConfig &);
    PipelineBuilder &setDepthTest(bool enable);
    PipelineBuilder &setInputConfig(const PipelineVertexInputConfig &);


    // Returns pipeline + layout
    std::pair<VkPipeline, VkPipelineLayout> build(VkDevice device, VkPipelineCache cache = VK_NULL_HANDLE) const;

private:
    PipelineConfig mConfig;
};

class PipelineManager
{
public:
    PipelineManager() = default;
    ~PipelineManager() = default;
    void initialize(VkDevice device, const std::string &cacheFile = "");
    void destroy(VkDevice device);
    void destroyPipeline(VkDevice device, uint32_t index);

    VkPipeline getPipeline(size_t index = 0) const { return mPipelines[index].pipeline; }
    VkPipelineLayout getPipelineLayout(size_t index = 0) const { return mPipelines[index].layout; }

    std::vector<unsigned char> getCacheData(VkDevice device);

    void createPipelineWithBuilder(VkDevice device ,const PipelineBuilder &builder);

    void reloadShaders(VkDevice device, int pipelineIndex, const PipelineBuilder builder);
    void dumpCacheToFile(const std::string &path, VkDevice mDevice);

private:
    VkDevice mDevice = VK_NULL_HANDLE;

    struct PipelineEntry
    {
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
    };
    std::vector<PipelineEntry> mPipelines;

    VkPipelineCache mPipelineCache = VK_NULL_HANDLE;
};


namespace vkUtils
{

    namespace Shaders
    {

        VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &code);
        std::vector<char> readShaderFile(const std::string &path);
        VkPipelineShaderStageCreateInfo createShaderStage(VkShaderStageFlagBits stage, VkShaderModule module);

    }

}

// Fixed-function configuration
////////////////////////
/*
+Multi thread creation of this
Cache creation and Compute Pipeliens
    VkComputePipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.layout = mPipelineLayout;


*/
/*
hreaded Pipeline Creation

Pipeline creation can be very slow (100s of ms per pipeline).

Many engines create them asynchronously in worker threads at load time.

Manager could expose async APIs:



Reflection-Based Descriptor Layouts

Use SPIR-V reflection to automatically build descriptor set layouts & push constant ranges.

That way, you don’t hardcode layouts in PipelineConfig.

Eases integration with shader authoring tools.

*/


//TODO: Multi Pipeline Creation + Multi Threated creation from cache

// Shader Module Abstraction ?
// ShaderModule → wraps VkShaderModule.

// Compiling shader on the fly in Oopengl was the usual
// CHeck if equivalent is possible in vulkan
