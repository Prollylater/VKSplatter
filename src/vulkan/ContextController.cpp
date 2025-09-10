
#include "QueueFam.h"
#include "ContextController.h"
#include "Descriptor.h"
#include "Texture.h"
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

void VulkanContext::initRenderInfrastructure()
{
    std::cout << "initRenderInfrastructure" << std::endl;

    const VkDevice &device = mLogDeviceM.getLogicalDevice();
    const QueueFamilyIndices &indicesFamily = mPhysDeviceM.getIndices();
    mGBuffers.init(mSwapChainM.getSwapChainExtent());

    const VkFormat depthFormat = mPhysDeviceM.findDepthFormat();

    // Defining the Render Pass Config as the config can have use in Pipeline Description
    RenderPassConfig defConfigRenderPass = RenderPassConfig::defaultForward(mSwapChainM.getSwapChainImageFormat().format, depthFormat);
    mRenderPassM.initConfiguration(defConfigRenderPass);

    // Get the proper format
    // std::vector<VkFormat> attachementFormat = defConfigRenderPass.getAttachementsFormat();
    // mGBuffers.createGBuffers(mLogDeviceM, mPhysDeviceM, attachementFormat);
    mGBuffers.createDepthBuffer(mLogDeviceM, mPhysDeviceM, depthFormat);
    mRenderPassM.createRenderPass(device, defConfigRenderPass);

    // Store the Render Pass conifg ? It give access to multiple elment
    mSwapChainM.completeFrameBuffers(device, {mGBuffers.getDepthImageView()}, mRenderPassM.getRenderPass());
};

void VulkanContext::initPipelineAndDescriptors(const PipelineLayoutDescriptor &layoutConfig, VertexFlags flag)
{
    std::cout << "initPipelineAndDescriptors" << std::endl;

    const VkDevice &device = mLogDeviceM.getLogicalDevice();

    PipelineBuilder builder;
    builder.setShaders({vertPath, fragPath})
        .setInputConfig({.vertexFormat = VertexFormatRegistry::getFormat(flag)})
        .setRenderPass(mRenderPassM.getRenderPass());
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
    // Also why am i still passing device everywhere ?
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
