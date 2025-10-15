#pragma once
#include "BaseVk.h"
#include <optional>
#include "VertexDescriptions.h" // Move this or Vertex Format ?

// Pipeline

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

struct PipelineRenderPassConfig
{
    VkRenderPass renderPass = VK_NULL_HANDLE;
    uint32_t subpass = 0;
    std::vector<VkFormat> colorAttachments;
    VkFormat depthAttachmentFormat;
    // VkFormat stencilAttachmentFormat;
    // uint32_t ViewMask;
};

struct PipelineConfig
{
    PipelineShaderConfig shaders;
    PipelineRasterConfig raster;
    PipelineBlendConfig blend;
    PipelineDepthConfig depth;
    PipelineVertexInputConfig input;
    PipelineLayoutConfig uniform;
    PipelineRenderPassConfig pass;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR};
};

// Descriptor
struct PipelineLayoutDescriptor
{
    std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayouts;
    std::vector<VkPushConstantRange> pushConstants;

    void addDescriptor(uint32_t location, VkDescriptorType type, VkShaderStageFlags stageFlag, uint32_t count = 1,
                       const VkSampler *sampler = nullptr)
    {
        descriptorSetLayouts.push_back({location, type, count, stageFlag, sampler});
    }

    void addPushConstant(VkShaderStageFlags stageFlag, uint32_t offset, uint32_t size)
    {
        pushConstants.push_back({stageFlag, offset, size});
    }
};

// Renderpass
enum class RenderPassType
{ // Default config ?
    Forward,
    GBuffer,
    Lighting,
    ShadowMap,
    PostProcess,
};

// Expand it for Subpasses and multiple attachements
// TOdo: Do i froget the {.membervariable}
struct AttachmentConfig
{
    VkAttachmentDescriptionFlags flags = 0;
    // Created from FrameRessources element/swapChainFromat ?
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT; // MSSA related, not introduce
    // Clear at render pass start
    VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    // Store after render pass
    VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // or SHADER_READ, etc.

    // VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    // VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    ////
};

template <typename Derived>
struct RenderTargetConfigCRTP
{
    std::vector<AttachmentConfig> attachments;

    bool enableDepth = true;
    bool enableMSAA = false;

    std::vector<VkFormat> getAttachementsFormat() const
    {
        std::vector<VkFormat> attachmentFormat;
        attachmentFormat.reserve(attachments.size());
        for (const auto &attachment : attachments)
        {
            attachmentFormat.push_back(attachment.format);
        }
        return attachmentFormat;
    }

    Derived &addAttachment(
        VkFormat format,
        VkImageLayout finalLayout,
        VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE)
    {
        attachments.push_back({.format = format,
                               .samples = VK_SAMPLE_COUNT_1_BIT,
                               .loadOp = loadOp,
                               .storeOp = storeOp,
                               .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                               .finalLayout = finalLayout});
        return static_cast<Derived &>(*this);
    }
};

struct RenderTargetConfig : RenderTargetConfigCRTP<RenderTargetConfig>
{
};

struct SubpassConfig
{
    struct AttachmentRef
    {
        uint32_t index; // index into RenderPassConfig attachments
        VkImageLayout layout;
    };

    std::vector<AttachmentRef> colorAttachments;
    std::optional<AttachmentRef> depthAttachment;
    std::vector<AttachmentRef> inputAttachments;
    // std::vector<AttachmentRef> resolveAttachments; // for MSAA resolve
};

struct RenderPassConfig : RenderTargetConfigCRTP<RenderPassConfig>
{
    std::vector<SubpassConfig> subpasses;
    std::vector<VkSubpassDependency> dependencies;

    RenderPassConfig &addSubpass()
    {
        subpasses.emplace_back();
        return *this;
    }

    RenderPassConfig &useColorAttachment(uint32_t index, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        subpasses.back().colorAttachments.push_back({index, layout});
        return *this;
    }

    RenderPassConfig &useDepthAttachment(uint32_t index, VkImageLayout layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        subpasses.back().depthAttachment = SubpassConfig::AttachmentRef{index, layout};
        return *this;
    }

    RenderPassConfig &addDependency(uint32_t srcSubpass, uint32_t dstSubpass,
                                    VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                                    VkAccessFlags srcAccess, VkAccessFlags dstAccess)
    {
        VkSubpassDependency dep{};
        dep.srcSubpass = srcSubpass;
        dep.dstSubpass = dstSubpass;
        dep.srcStageMask = srcStage;
        dep.dstStageMask = dstStage;
        dep.srcAccessMask = srcAccess;
        dep.dstAccessMask = dstAccess;
        dependencies.push_back(dep);
        return *this;
    }

    static RenderPassConfig defaultForward(VkFormat colorFormat, VkFormat depthFormat)
    {
        RenderPassConfig defConfigRenderPass;
        defConfigRenderPass.addAttachment(colorFormat, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE)
            .addAttachment(depthFormat, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE)
            .addSubpass()
            .useColorAttachment(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            .useDepthAttachment(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
            .addDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
        return defConfigRenderPass;
    }
};
