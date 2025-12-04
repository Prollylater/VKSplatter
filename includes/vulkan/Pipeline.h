#pragma once
#include "BaseVk.h"

#include <array>
#include "VertexDescriptions.h"

class PipelineBuilder;

class PipelineManager
{
public:
    void initialize(VkDevice device, const std::string &cacheFile = "");
    void destroy(VkDevice device);
    void destroyPipeline(VkDevice device, uint32_t index);

    VkPipeline getPipeline(size_t index = 0) const { return mPipelines[index].pipeline; }
    VkPipelineLayout getPipelineLayout(size_t index = 0) const { return mPipelines[index].layout; }
    size_t getPipelineSize() { return mPipelines.size(); }

    std::vector<unsigned char> getCacheData(VkDevice device);

    int createPipelineWithBuilder(VkDevice device, const PipelineBuilder &builder);

    void reloadShaders(VkDevice device, int pipelineIndex, const PipelineBuilder builder);
    void dumpCacheToFile(const std::string &path, VkDevice mDevice);

private:
    VkDevice mDevice = VK_NULL_HANDLE;

    struct PipelineEntry
    {
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
        /*
        enum class PipelineType {
        GraphicsMaterial,  
        GraphicsNoMaterial, 
        Compute,
        RayTracing
    } type;
        */
    };

    std::unordered_map<size_t, int> mIndexByKey;
    std::vector<PipelineEntry> mPipelines;

    VkPipelineCache mPipelineCache = VK_NULL_HANDLE;
};
