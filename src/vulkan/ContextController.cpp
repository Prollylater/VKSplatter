
#include "QueueFam.h"
#include "ContextController.h"
#include "Descriptor.h"
#include "Texture.h"
#include <span>
#include "utils/PipelineHelper.h"

void VulkanContext::initVulkanBase(GLFWwindow *window, ContextCreateInfo &createInfo)
{
    mInstanceM.createInstance(createInfo.getVersionMajor(), createInfo.getVersionMinor(),
                              createInfo.getValidationLayers(), createInfo.getInstanceExtensions());

    // Todo: Reread about the subject to understand why it was here
    // Instance =/= DebugMessenge
    mInstanceM.setupDebugMessenger();

    // Surface
    mSwapChainM.createSurface(mInstanceM.getInstance(), window);

    // Physical Device
    mPhysDeviceM.pickPhysicalDevice(mInstanceM.getInstance(), mSwapChainM, createInfo.getDeviceSelector());
    const VkPhysicalDevice &physDevice = mPhysDeviceM.getPhysicalDevice();
    const QueueFamilyIndices &indicesFamily = mPhysDeviceM.getIndices();

    // Logical Device
    mLogDeviceM.createLogicalDevice(physDevice, indicesFamily, createInfo.getValidationLayers(), createInfo.getDeviceSelector());
    const VkDevice &device = mLogDeviceM.getLogicalDevice();

    // SwapChain
    mSwapChainM.createSwapChain(physDevice, device, window, createInfo.getSwapChainConfig(), indicesFamily);
    mSwapChainM.createImageViews(device);

    // Frame Ressources
    mSwapChainM.createFramesData(device, physDevice, indicesFamily.graphicsFamily.value(), createInfo.MAX_FRAMES_IN_FLIGHT);

    // Initialize pipeline Cache
    mPipelineM.initialize(device, "");
}

static constexpr bool dynamic = true;
void VulkanContext::initRenderInfrastructure()
{
    std::cout << "Init Render Infrastructure" << std::endl;
    const VkDevice &device = mLogDeviceM.getLogicalDevice();
    const QueueFamilyIndices &indicesFamily = mPhysDeviceM.getIndices();
    mGBuffers.init(mSwapChainM.getSwapChainExtent());

    const VkFormat depthFormat = mPhysDeviceM.findDepthFormat();

    // Onward is just hardcoded stuff not meant to be dynamic yet
    if (dynamic)
    {
        RenderTargetConfig defRenderPass;
        defRenderPass.addAttachment(mSwapChainM.getSwapChainImageFormat().format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE)
            .addAttachment(depthFormat, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);

        // Create the actual offscreen G-buffers (excluding swapchain & depth)
        std::vector<VkFormat> gbufferFormats = defRenderPass.getAttachementsFormat();

        // Remove first (swapchain) and last (depth)
        if (gbufferFormats.size() > 2){
            gbufferFormats = {gbufferFormats.begin() + 1, gbufferFormats.end() - 1};}
        else{
            gbufferFormats.clear();}
        
        mGBuffers.createGBuffers(mLogDeviceM, mPhysDeviceM, gbufferFormats);
        mGBuffers.createDepthBuffer(mLogDeviceM, mPhysDeviceM, depthFormat);

        // 3Collect image views for dynamic rendering
        std::vector<VkImageView> colorViews;
        colorViews.reserve(mGBuffers.colorBufferNb());
        for (size_t index = 0; index < mGBuffers.colorBufferNb(); index++)
        {
            colorViews.push_back(mGBuffers.getColorImageView(index));
        }

        // Configure per-frame rendering info
        mSwapChainM.createFramesDynamicRenderingInfo(defRenderPass, colorViews, mGBuffers.getDepthImageView());
    }
    else
    {
        // Defining the Render Pass Config as the config can have use in Pipeline Description
        RenderPassConfig defConfigRenderPass = RenderPassConfig::defaultForward(mSwapChainM.getSwapChainImageFormat().format, depthFormat);
        mRenderPassM.initConfiguration(defConfigRenderPass);

        // Get the proper format which could be done before Re
        // Todo: std::span usage could make sense here and there
        // std::vector<VkFormat> attachementFormat = defConfigRenderPass.getAttachementsFormat();
        // mGBuffers.createGBuffers(mLogDeviceM, mPhysDeviceM, attachementFormat);
        mGBuffers.createDepthBuffer(mLogDeviceM, mPhysDeviceM, depthFormat);
        mRenderPassM.createRenderPass(device, defConfigRenderPass);

        mSwapChainM.completeFrameBuffers(device, {mGBuffers.getDepthImageView()}, mRenderPassM.getRenderPass());
    }
};

void VulkanContext::initPipelineAndDescriptors(const PipelineLayoutDescriptor &layoutConfig, VertexFlags flag)
{
    std::cout << "initPipelineAndDescriptors" << std::endl;

    const VkDevice &device = mLogDeviceM.getLogicalDevice();

    // For dynamic rendering
    // std::vector<VkFormat> slice;
    // We get SLice from the Gbuffer i guess
    // slice = std::vector<VkFormat>();
    // Slice asssuming  attachment last attachment is the depth

    PipelineBuilder builder;
    builder.setShaders({vertPath, fragPath})
        .setInputConfig({.vertexFormat = VertexFormatRegistry::getFormat(flag)});

    auto attachments = mGBuffers.getAllFormats();
    attachments.insert(attachments.begin(), mSwapChainM.getSwapChainImageFormat().format);
    std::vector<VkFormat> clrAttach(attachments.begin(), attachments.end() - 1);
    if (dynamic)
    {
        builder.setDynamicRenderPass(clrAttach, attachments.back());
    }
    else
    {
        builder.setRenderPass(mRenderPassM.getRenderPass());
    }

    // Todo:Not too sure of VertexFormatRegisty here
    // Todo:Not too sure of VertexFormatRegisty here
    // Todo:Not too sure of VertexFormatRegisty here
    // Todo:Not too sure of VertexFormatRegisty here
    std::vector<VkDescriptorSetLayoutBinding> layoutBindings = layoutConfig.descriptorSetLayouts;
    mSwapChainM.createFramesSetLayout(device, layoutBindings);

    builder.setUniform({{mSwapChainM.getCurrentFrameData().mDescriptor.getDescriptorLat()},
                        layoutConfig.pushConstants});

    mPipelineM.createPipelineWithBuilder(device, builder);
};

void VulkanContext::destroyAll()
{
    // Have destructor call those function
    VkDevice device = mLogDeviceM.getLogicalDevice();

    for (auto &buffer : mBufferM)
    {
        buffer.destroyBuffer();
    }

    mGBuffers.destroy(device);

    // Important
    // Todo: Better deletion of frames data
    for (int i = 0; i < mSwapChainM.GetSwapChainImageViews().size(); i++)
    {
        auto &frameData = mSwapChainM.getCurrentFrameData();
        frameData.mFramebuffer.destroyFramebuffers(device);
        frameData.mDescriptor.destroyDescriptorLayout(device);
        frameData.mDescriptor.destroyDescriptorPool(device);
        mSwapChainM.advanceFrame();
    }
    mSwapChainM.destroyFramesData(device);

    mRenderPassM.destroyRenderPass(device);
    mPipelineM.destroy(device);

    mSwapChainM.DestroyImageViews(device);
    mSwapChainM.destroySwapChain(device);

    mLogDeviceM.DestroyDevice();
    mSwapChainM.destroySurface();
    mInstanceM.destroyInstance();
};
