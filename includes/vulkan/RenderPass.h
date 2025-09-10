

#pragma once
#include "BaseVk.h"
#include "CommandPool.h"
#include "SwapChain.h"
#include "CommandPool.h"

// Todo: Look into Dynamic Rendering
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

struct RenderPassConfig
{
    std::vector<AttachmentConfig> attachments;
    std::vector<SubpassConfig> subpasses;
    std::vector<VkSubpassDependency> dependencies;

    bool enableDepth = true;
    bool enableMSAA = false;

    std::vector<VkFormat> getAttachementsFormat()
    {
        std::vector<VkFormat> attachmentFormat(attachments.size());
        for (const auto &attachment : attachments)
        {
            attachmentFormat.push_back(attachment.format);
        }
        return attachmentFormat;
    }

    RenderPassConfig &addAttachment(
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
        return *this;
    }

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

/*
Create/render pass definitions through a config files
*/
class RenderPassManager
{
public:
    RenderPassManager() = default;
    ~RenderPassManager() = default;

    VkRenderPass getRenderPass() const { return mRenderPass; };
    void createRenderPass(VkDevice device, const RenderPassConfig &);

    // Move this to higher function
    void startPass(const VkCommandBuffer &command, const VkFramebuffer &frameBuffer, const VkExtent2D &extent);
    void endPass(const VkCommandBuffer &);

    void destroyRenderPass(VkDevice device);

    void initConfiguration(RenderPassConfig & config){
        mConfiguration = config;
    }
    const RenderPassConfig &getConfiguration() const
    {
        return mConfiguration;
    }

private:
    VkDevice mDevice = VK_NULL_HANDLE;
    VkRenderPass mRenderPass = VK_NULL_HANDLE;
    RenderPassConfig mConfiguration;
};

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
        }
    }
}

// Subpass Dependency quick guide:

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
