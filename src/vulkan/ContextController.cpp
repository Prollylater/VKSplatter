
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

    // TOdo: Create info being non const criteria used in phisycal device, coudl chande in logical device
    //  Physical Device
    mPhysDeviceM.pickPhysicalDevice(mInstanceM.getInstance(), mSwapChainM, createInfo.getDeviceSelector());
    const VkPhysicalDevice &physDevice = mPhysDeviceM.getPhysicalDevice();
    const QueueFamilyIndices &indicesFamily = mPhysDeviceM.getIndices();

    // Logical Device
    mLogDeviceM.createLogicalDevice(physDevice, indicesFamily, createInfo.getValidationLayers(), createInfo.getDeviceSelector());
    const VkDevice &device = mLogDeviceM.getLogicalDevice();

    mLogDeviceM.createVmaAllocator(physDevice, mInstanceM.getInstance());
    // SwapChain
    mSwapChainM.createSwapChain(physDevice, device, window, createInfo.getSwapChainConfig(), indicesFamily);
    mSwapChainM.createImageViews(device);

    // Frame Ressources
    mSwapChainM.createFramesData(device, physDevice, indicesFamily.graphicsFamily.value(), createInfo.MAX_FRAMES_IN_FLIGHT);


    // Initialize pipeline Cache
    mPipelineM.initialize(device, "");


    //Init Frame Descriptor set
    PipelineLayoutDescriptor sceneLayout;
    sceneLayout.addDescriptor(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
    sceneLayout.addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UniformBufferObject));
 
    std::vector<std::vector<VkDescriptorSetLayoutBinding>> layoutBindings = {sceneLayout.descriptorSetLayouts};
    mSwapChainM.createFramesDescriptorSet(device, layoutBindings);

}

static constexpr bool dynamic = true;
// TODO:
// Could be overloaded with  RenderTargetConfig that will mean dynamic
// Render Pass that will mean not dynamic render
void VulkanContext::initRenderInfrastructure()
{
    std::cout << "Init Render Infrastructure" << std::endl;
    const VkDevice &device = mLogDeviceM.getLogicalDevice();
    const QueueFamilyIndices &indicesFamily = mPhysDeviceM.getIndices();
    mGBuffers.init(mSwapChainM.getSwapChainExtent());

    const VkFormat depthFormat = mPhysDeviceM.findDepthFormat();

    // Onward is just hardcoded stuff not meant to be dynamic yet
    const VmaAllocator &allocator = mLogDeviceM.getVmaAllocator();

    if (dynamic)
    {
        RenderTargetConfig defRenderPass;
        defRenderPass.addAttachment(mSwapChainM.getSwapChainImageFormat().format, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE)
            .addAttachment(depthFormat, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE);

        // Create the actual offscreen G-buffers (excluding swapchain & depth)
        std::vector<VkFormat> gbufferFormats = defRenderPass.getAttachementsFormat();

        // Remove first (swapchain) and last (depth)
        if (gbufferFormats.size() > 2)
        {
            gbufferFormats = {gbufferFormats.begin() + 1, gbufferFormats.end() - 1};
        }
        else
        {
            gbufferFormats.clear();
        }

        mGBuffers.createGBuffers(mLogDeviceM, mPhysDeviceM, gbufferFormats, allocator);
        mGBuffers.createDepthBuffer(mLogDeviceM, mPhysDeviceM, depthFormat, allocator);

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
        mGBuffers.createDepthBuffer(mLogDeviceM, mPhysDeviceM, depthFormat, allocator);
        mRenderPassM.createRenderPass(device, defConfigRenderPass);

        mSwapChainM.completeFrameBuffers(device, {mGBuffers.getDepthImageView()}, mRenderPassM.getRenderPass());
    }
};

int VulkanContext::requestPipeline(VkDescriptorSetLayout materialLayout,
                                   const std::string &vertexPath,
                                   const std::string &fragmentPath,
                                   VertexFlags flags)
{
    const VkDevice &device = mLogDeviceM.getLogicalDevice();

    //Temp
 
    // For dynamic rendering
    // std::vector<VkFormat> slice;
    // We get SLice from the Gbuffer i guess
    // slice = std::vector<VkFormat>();
    // Slice asssuming  attachment last attachment is the depth

    PipelineBuilder builder;
    builder.setShaders({vertexPath, fragmentPath})
        .setInputConfig({.vertexFormat = VertexFormatRegistry::getFormat(flags)});
 
    std::cout<<flags<<std::endl;
    std::cout<<flags<<std::endl;
    std::cout<<flags<<std::endl;

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
    //Only UBO is added 
    PipelineLayoutConfig layoutConfig;
    layoutConfig.descriptorSetLayouts = {mSwapChainM.getCurrentFrameData().mDescriptor.getDescriptorLat(0), materialLayout};
    layoutConfig.pushConstants = mSceneLayout.pushConstants; 

    builder.setUniform(layoutConfig);

    return mPipelineM.createPipelineWithBuilder(device, builder);

};

void VulkanContext::initPipelineAndDescriptors(const PipelineLayoutDescriptor &layoutConfig, VertexFlags flag)
{
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

    //builder.setUniform({{mSwapChainM.getCurrentFrameData().mDescriptor.getDescriptorLat()},
      //q                  layoutConfig.pushConstants});

    mPipelineM.createPipelineWithBuilder(device, builder);
};

void VulkanContext::destroyAll()
{
    // Have destructor call those function
    VkDevice device = mLogDeviceM.getLogicalDevice();
    VmaAllocator allocator = mLogDeviceM.getVmaAllocator();

    mGBuffers.destroy(device, allocator);

    // Important
    // Todo: Better deletion of frames data
    for (int i = 0; i < mSwapChainM.GetSwapChainImageViews().size(); i++)
    {
        auto &frameData = mSwapChainM.getCurrentFrameData();
        frameData.mFramebuffer.destroyFramebuffers(device);
        mSwapChainM.advanceFrame();
    }
    mSwapChainM.destroyFramesData(device);
    mMaterialManager.destroyDescriptorLayout(device);
    mMaterialManager.destroyDescriptorPool(device);

    mRenderPassM.destroyRenderPass(device);
    mPipelineM.destroy(device);

    mSwapChainM.DestroyImageViews(device);
    mSwapChainM.destroySwapChain(device);
    mLogDeviceM.destroyVmaAllocator();
    mLogDeviceM.DestroyDevice();
    mSwapChainM.destroySurface();
    mInstanceM.destroyInstance();
};

void VulkanContext::recreateSwapchain(GLFWwindow *window)
{
    // Pause while Minimized
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    // Pause application until gpu is done before recreating everything
    mLogDeviceM.waitIdle();
    const auto &device = mLogDeviceM.getLogicalDevice();
    const auto &allocator = mLogDeviceM.getVmaAllocator();

    mSwapChainM.DestroyImageViews(device);
    mSwapChainM.destroySwapChain(device);

    // Destroy FrameBuffers/GBuffers/Modify render pass Info

    mGBuffers.destroy(device, allocator);

    for (int i = 0; i < mSwapChainM.GetSwapChainImageViews().size(); i++)
    {
        auto &frameData = mSwapChainM.getCurrentFrameData();
        frameData.mFramebuffer.destroyFramebuffers(device);
        mSwapChainM.advanceFrame();
    }

    // SwapChain
    mSwapChainM.createSwapChain(mPhysDeviceM.getPhysicalDevice(), device, window, mSwapChainM.getConfig(), mPhysDeviceM.getIndices());
    mSwapChainM.createImageViews(device);

    initRenderInfrastructure();
}

// Todo: Tale care of this
#ifdef USE_VMA
// vmaCreateImage(...)
#else
// vkCreateImage(...)
#endif