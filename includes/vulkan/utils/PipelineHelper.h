#pragma once

#include "config/PipelineConfigs.h"

// Pipeline
/*

struct PipelineKey {
    std::string vertShader;        
    std::string fragShader;
    uint32_t DescriptorIdentifie;  
};

Todo: Later Implement Hashing for creating Pipeline instead of handtracking it
*/
// TOdo: Not quite sure where the builder stay
/*
//Todo: Rethink this
struct PipelineKey {
    size_t shaderHash;
    size_t layoutHash;
    size_t rasterHash;
    size_t vertexFormatHash;
    bool dynamicRender;
    bool enableAlphaBlend;
    bool enableDepthTest;
};
*/
class PipelineBuilder
{
public:
    PipelineBuilder &setShaders(const PipelineShaderConfig &);
    PipelineBuilder &setDynamicStates(const std::vector<VkDynamicState> &states);
    PipelineBuilder &setRasterizer(const PipelineRasterConfig &);
    PipelineBuilder &setRenderPass(VkRenderPass renderPass, uint32_t subpass = 0);
    PipelineBuilder &setDynamicRenderPass(const std::vector<VkFormat> &color,
                                          VkFormat depth, VkFormat stencil = {}, uint32_t ViewMask = 0);

    PipelineBuilder &setUniform(const PipelineLayoutConfig &uniform);
    PipelineBuilder &setBlend(const PipelineBlendConfig &);
    PipelineBuilder &setDepthConfig(const PipelineDepthConfig &);
    PipelineBuilder &setDepthTest(bool enable);
    PipelineBuilder &setInputConfig(const PipelineVertexInputConfig &);
    size_t computehash() const {
        //BOol and 
        return  mConfig.input.computeHash() ^ mConfig.uniform.computeHash() ^ mConfig.raster.computeHash() ^  mConfig.shaders.computeHash();
    }
    // Returns pipeline + layout
    std::pair<VkPipeline, VkPipelineLayout> build(VkDevice device, VkPipelineCache cache = VK_NULL_HANDLE) const;

private:
    PipelineConfig mConfig;
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
/*Todo:
+Multi threaded Pipeline Creation + cahce usage

Pipeline creation can be very slow (100s of ms per pipeline).

Many engines create them asynchronously in worker threads at load time.

Manager could expose async APIs:



Reflection-Based Descriptor Layouts

Use SPIR-V reflection to automatically build descriptor set layouts & push constant ranges.

That way, you don’t hardcode layouts in PipelineConfig.

Eases integration with shader authoring tools.

*/

// TODO: Multi Pipeline Creation + Multi Threated creation from cache

// Shader Module Abstraction ?
// ShaderModule → wraps VkShaderModule.

// Compiling shader on the fly in Oopengl was the usual
// CHeck if equivalent is possible in vulkan

// TODO: Builder are currently in config

// Descriptor

namespace vkUtils
{

    namespace Descriptor
    {
        inline VkDescriptorSetLayoutBinding makeLayoutBinding(
            uint32_t binding, VkDescriptorType type, VkShaderStageFlags stages, uint32_t count = 1)
        {
            VkDescriptorSetLayoutBinding layout{};
            layout.binding = binding;
            layout.descriptorType = type;
            layout.descriptorCount = count;
            layout.stageFlags = stages;
            return layout;
        }

        inline VkPushConstantRange makePushConstantRange(
            VkShaderStageFlags flag, uint32_t offset, uint32_t size)
        {
            VkPushConstantRange pushConstantRange{};
            pushConstantRange.stageFlags = flag;
            pushConstantRange.offset = 0;
            pushConstantRange.size = size;
            return pushConstantRange;
        }

        inline VkWriteDescriptorSet makeWriteDescriptor(
            VkDescriptorSet dstSet, uint32_t binding, VkDescriptorType type,
            const VkDescriptorBufferInfo *bufferInfo = nullptr,
            const VkDescriptorImageInfo *imageInfo = nullptr)
        {
            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = dstSet;
            // Beginnig element in Array (We may want to update only part of an Array)
            // Not implemented yet
            write.dstArrayElement = 0;
            write.dstBinding = binding;
            // Number of elements to update, in an array
            write.descriptorCount = 1;
            write.descriptorType = type;
            write.pBufferInfo = bufferInfo;
            write.pImageInfo = imageInfo;
            write.pTexelBufferView = VK_NULL_HANDLE;

            return write;
        }

    }
};

// Render Pass
/*
On multiple render pass,
Should eventually have multiple of those
Typically for shadowPass where i guess only the depth from light view matter

Deferred Rendering with Gbuffer pass then coloring pass

Other stuff ? Each with their own shader

Maybe multi viewport from differnt pov


///////////////////////////////////////////////////////////////
The index of the attachment in this array is directly referenced from the fragment shader with the layout(location = 0) out vec4 outColor directive!
pInputAttachments: Attachments that are read from a shader
pDepthStencilAttachment: Attachment for depth and stencil data

//Not implemented yet
pPreserveAttachments: Attachments that are not used by this subpass, but for which the data must be preserved
pResolveAttachments: Attachments used for multisampling color attachments

*/

namespace vkUtils
{
    namespace RenderPass
    {
        /*
        inline RenderPassConfig makeDefaultConfig(VkFormat swpachChainFormat, VkFormat depthFormat, RenderPassType type = RenderPassType::Forward)
        {

            RenderPassConfig defConfigRenderPass;

            defConfigRenderPass.attachments.push_back({.format = swpachChainFormat,
                                                       .samples = VK_SAMPLE_COUNT_1_BIT,
                                                       .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                       .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                                                       .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                                       .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR});
            // Depth

            defConfigRenderPass.attachments.push_back({.format = depthFormat,
                                                       .samples = VK_SAMPLE_COUNT_1_BIT,
                                                       .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                       .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                                       .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                                       .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL});

            // Subpass  0 Parameter: uses color + depth
            std::optional<SubpassConfig::AttachmentRef> depthRef = SubpassConfig::AttachmentRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
            SubpassConfig subpassConfig = {.colorAttachments = {{.index = 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}},
                                           .depthAttachment = depthRef};

            defConfigRenderPass.subpasses.push_back(subpassConfig);

            // Dependency
            VkSubpassDependency subpDep{};
            subpDep.srcSubpass = VK_SUBPASS_EXTERNAL;
            subpDep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            subpDep.srcAccessMask = 0;

            // We are going to use the Attachment in the fragment shader stage to write on color attachment
            subpDep.dstSubpass = 0;
            subpDep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            subpDep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            // subpDep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            defConfigRenderPass.dependencies.push_back(subpDep);

            return defConfigRenderPass;
        }

        // A simple post Processing
        inline RenderPassConfig makeDefaultPostProConfig(VkFormat swpachChainFormat, VkFormat depthFormat, RenderPassType type = RenderPassType::Forward)
        {

            RenderPassConfig defConfigRenderPass;
            // Define the Render Pass Config

            defConfigRenderPass.attachments.push_back({.format = swpachChainFormat,
                                                       .samples = VK_SAMPLE_COUNT_1_BIT,
                                                       .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                       .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                                       .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                                       .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
            // Depth

            defConfigRenderPass.attachments.push_back({.format = depthFormat,
                                                       .samples = VK_SAMPLE_COUNT_1_BIT,
                                                       .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                       .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                                       .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                                       .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL});

            // Define the Render Pass Config
            // Image must have been created with INputAttachemtnBit
            defConfigRenderPass.attachments.push_back({.format = swpachChainFormat,
                                                       .samples = VK_SAMPLE_COUNT_1_BIT,
                                                       .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                       .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                                                       .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                                       .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR});

            // Subpass 0: uses color + depth
            std::optional<SubpassConfig::AttachmentRef> depthRef = SubpassConfig::AttachmentRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
            SubpassConfig subpassConfig = {.colorAttachments = {{.index = 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}},
                                           .depthAttachment = depthRef};

            SubpassConfig subpass2Config = {.colorAttachments = {{.index = 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}},
                                            .inputAttachments = {{.index = 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}}};

            defConfigRenderPass.subpasses.push_back(subpassConfig);

            // Dependency 1
            VkSubpassDependency subpDep{};
            subpDep.srcSubpass = VK_SUBPASS_EXTERNAL;
            subpDep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            subpDep.srcAccessMask = 0;
            subpDep.dstSubpass = 0;
            subpDep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            subpDep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            // Dependency 2
            VkSubpassDependency subp2Dep{};
            subp2Dep.srcSubpass = 0;
            subp2Dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            subp2Dep.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            subp2Dep.dstSubpass = 1;
            subp2Dep.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            subp2Dep.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            defConfigRenderPass.dependencies.push_back(subp2Dep);

            return defConfigRenderPass;
        }*/
    }
}

// Subpass Dependency :

/*
Notes: A dependency array don't map to the VkSubpassDescription

// Synchronization between subpasses (wait at stage)
// Decide when, Transition actually happen
srcSubpass : The index of a subpass whose operations should be finished before this operations (or a VK_SUBPASS_EXTERNAL value for commands before the render pass)
dstSubpass ! The index of a subpass whose operations depend on the previous set of commands (or a VK_SUBPASS_EXTERNAL value for operations after the render pass or our current subpass)
srcsubpass < dstsubpass. Don't matter for


srcStageMask: The set of pipeline stages which produce the result read by the "consuming" commands
srcAccessMask!The types of memory operations that occurred for the "producing" commands for

dstStageMask: The set of pipeline stages which depend on the data generated by the "producing" commands for
dstAccessMask:The types of memory operations that will be performed in "consuming" commands for

The mask are applied on all ressources on the subpass so we couold do like |
For dependencyFlags, use a VK_DEPENDENCY_BY_REGION_BIT value if the dependency is defined by region--
it means that operations generating data for a given memory region must finish before operations reading data from the same region can be executed;
 if this flag is not specified, dependency is global, which means that data for the whole image must be generated before "consuming" commands can be executed.


Todo: Look into dependency graph concept for Vulkan
Example
VkAttachmentDescription albedoAttachment{};
albedoAttachment.format = VK_FORMAT_R8G8B8A8_UNORM; // Standard 8-bit RGBA
albedoAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
albedoAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
albedoAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
albedoAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
albedoAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
albedoAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
albedoAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
*/
