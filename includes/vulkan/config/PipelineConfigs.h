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

// Renderpass
enum class RenderPassType : uint16_t
{
    Forward,
    GBuffer,
    Lighting,
    Shadow,
    PostProcess,
    UI,
    Count
};

struct PipelineShaderConfig
{
    std::string vertShaderPath;
    std::string fragShaderPath;
    std::string geomShaderPath;
    std::string computeShaderPath;

    size_t computeHash() const
    {
        auto hashCombine = [](size_t currhash, size_t value)
        {
            return currhash ^ (value + (currhash << 6));
        };

        size_t h = 0;
        h = hashCombine(h, std::hash<std::string>{}(vertShaderPath));
        h = hashCombine(h, std::hash<std::string>{}(fragShaderPath));
        h = hashCombine(h, std::hash<std::string>{}(geomShaderPath));
        h = hashCombine(h, std::hash<std::string>{}(computeShaderPath));
        return h;
    }
};

struct PipelineLayoutConfig
{
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    std::vector<VkPushConstantRange> pushConstants;

    size_t computeHash() const
    {
        auto hashCombine = [](size_t currhash, size_t value)
        {
            return currhash ^ (value + (currhash << 6));
        };
        size_t h = 0;
        for (auto layout : descriptorSetLayouts)
            h = hashCombine(h, reinterpret_cast<size_t>(layout)); // pointer-based hash
        for (auto &pc : pushConstants)
        {
            h = hashCombine(h, pc.stageFlags);
            h = hashCombine(h, pc.offset);
            h = hashCombine(h, pc.size);
        }
        return h;
    }
};

struct PipelineRasterConfig
{
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
    VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    // Lines and Point with fillModeNonSolid available with gpu features
    VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;

    size_t computeHash() const
    {
        size_t h = 0;
        h ^= std::hash<uint32_t>{}(cullMode) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<uint32_t>{}(frontFace) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<uint32_t>{}(polygonMode) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
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
    size_t computeHash() const
    {
        auto hashCombine = [](size_t currhash, size_t value)
        {
            return currhash ^ (value + (currhash << 6));
        };
        size_t h = 0;

        h = hashCombine(h, static_cast<size_t>(vertexFormat.mInterleaved));
        h = hashCombine(h, static_cast<size_t>(vertexFormat.mVertexFlags));

        for (auto &b : vertexFormat.bindings)
        {
            h = hashCombine(h, b.binding);
            h = hashCombine(h, b.stride);
            h = hashCombine(h, b.inputRate);
        }

        for (auto &a : vertexFormat.attributes)
        {
            h = hashCombine(h, a.location);
            h = hashCombine(h, a.binding);
            h = hashCombine(h, a.format);
            h = hashCombine(h, a.offset);
        }

        return h;
    }
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
    PipelineLayoutConfig uniform; // Todo: Uniform is not an appropraite name
    PipelineRenderPassConfig pass;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR};
};

// Descriptor
// This only describe a single set layout. also it is Poorly named
struct PipelineSetLayoutBuilder
{
    // Bad name
    std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutsBindings;
    std::vector<VkPushConstantRange> pushConstants;

    void addDescriptor(uint32_t location, VkDescriptorType type, VkShaderStageFlags stageFlag, uint32_t count = 1,
                       const VkSampler *sampler = nullptr)
    {
        descriptorSetLayoutsBindings.push_back({location, type, count, stageFlag, sampler});
    }

    void addPushConstant(VkShaderStageFlags stageFlag, uint32_t offset, uint32_t size)
    {
        pushConstants.push_back({stageFlag, offset, size});
    }
};

//Todo: SHould be directly in Material
//But hten Material would need to output it ?
// PipelineSetLayoutBuilder materialLayoutInfo;
// Introduce information too much tied to the Pipeline here
struct MaterialLayoutRegistry
{
    static const PipelineSetLayoutBuilder &Get(MaterialType type)
    {
        switch (type)
        {
        case MaterialType::None:

            static PipelineSetLayoutBuilder UnlitLayout = []
            {
                PipelineSetLayoutBuilder layout;
                layout.addDescriptor(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
                return layout;
            }();

            return UnlitLayout;
        case MaterialType::PBR:
        default:
            static PipelineSetLayoutBuilder PBRLayout = []
            {
                PipelineSetLayoutBuilder layout;
                layout.addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
                layout.addDescriptor(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // albedo
                layout.addDescriptor(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // normal
                layout.addDescriptor(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // metal
                layout.addDescriptor(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // rough
                layout.addDescriptor(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT); // emissive
                return layout;
            }();

            return PBRLayout;
        }
    }
};

// Expand it for Subpasses and multiple attachements
// TOdo: Do i froget the {.membervariable}
struct AttachmentConfig
{
    // Todo: See where this could be used
    // Name ?
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

    enum class Role : uint8_t
    {
        Other,
        Depth,
        Present
    } role;

    // Index of the attachment in The Gbuffer
    // Todo: Might interesting to find another way as for example Depth will need an ID but it won't really be used due to Depth Buffer be separated
    // An automatic way to assign using correct fromat ?
    // This also create a disconnect since we also jave VkFormat which is an information that should come from the attachment
    // The better choice would be to pass the Gbuffer directly in
    // Currently 255 is a stand in for invalid
    uint8_t realAttachmentID = 255;

    // VkAttachmentLoadOp stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    // VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
};

enum class RenderConfigType
{
    Dynamic,
    LegacyRenderPass
};
// Todo: Crtp might not really useful here
// Look for example where it is actually useful
// Static polymorphism is probably not needed
// NEtter in SIMD? Job scehduling ? ECS? Anything

template <typename Derived>
struct RenderTargetConfigCRTP
{
    // The entire Attachement COnfig i
    // Not fully useful for dynamic rendering
    // Should we change this ?
    std::vector<AttachmentConfig> attachments;

    RenderConfigType type = RenderConfigType::Dynamic;

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

    // Format of the actual offscreen G-buffers (excluding swapchain & depth)
    std::vector<VkFormat> extractGBufferFormats() const
    {
        std::vector<VkFormat> fmts;

        // Todo:
        for (const auto &att : attachments)
        {
            if (att.role == AttachmentConfig::Role::Other)
            {
                // Bad fail safe until i deal with this
                if (att.finalLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR || att.finalLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
                {
                    continue;
                    ;
                }
                fmts.push_back(att.format);
            }
        }
        return fmts;
    }

    // Todo:
    // This add Attachment without modifying the invalidID (Mostly present/depth right now)
    Derived &addAttachment(
        VkFormat format,
        VkImageLayout finalLayout,
        VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        AttachmentConfig::Role role = AttachmentConfig::Role::Other)
    {
        attachments.push_back({.format = format,
                               .samples = VK_SAMPLE_COUNT_1_BIT,
                               .loadOp = loadOp,
                               .storeOp = storeOp,
                               .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                               .finalLayout = finalLayout,
                               .role = role});
        return static_cast<Derived &>(*this);
    }

    Derived &addAttachment(
        uint8_t attachmentId,
        VkFormat format,
        VkImageLayout finalLayout,
        VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        AttachmentConfig::Role role = AttachmentConfig::Role::Other)
    {
        attachments.push_back({.format = format,
                               .samples = VK_SAMPLE_COUNT_1_BIT,
                               .loadOp = loadOp,
                               .storeOp = storeOp,
                               .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                               .finalLayout = finalLayout,
                               .role = role,
                               .realAttachmentID = attachmentId});
        return static_cast<Derived &>(*this);
    }

    const std::vector<uint8_t> getClrAttachmentsID() const
    {
        // Todo: Still on the subject of trying more of C++20 tools, this kind of situation ? filter, transform
        std::vector<uint8_t> attachementsUsed;
        attachementsUsed.reserve(attachments.size());
        for (const auto &attachment : attachments)
        {
            if (attachment.realAttachmentID != 255)
            {
                attachementsUsed.push_back(attachment.realAttachmentID);
            }
        }
        return attachementsUsed;
    }

    Derived &derived() { return static_cast<Derived &>(*this); }
    const Derived &derived() const { return static_cast<const Derived &>(*this); }
};

struct RenderTargetConfig : RenderTargetConfigCRTP<RenderTargetConfig>
{
    RenderTargetConfig() { type = RenderConfigType::Dynamic; }
    // Storethe Harrier?
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

    RenderPassConfig() { type = RenderConfigType::LegacyRenderPass; }

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
        // Todo: Could
        RenderPassConfig defConfigRenderPass;
        defConfigRenderPass.addAttachment(colorFormat, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, AttachmentConfig::Role::Present)
            .addAttachment(depthFormat, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE, AttachmentConfig::Role::Depth)
            .addSubpass()
            .useColorAttachment(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            .useDepthAttachment(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
            .addDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
        return defConfigRenderPass;
    }
};
