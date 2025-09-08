
#include "QueueFam.h"
#include "ContextController.h"
#include "Descriptor.h"
#include "Texture.h"
void VulkanContext::initVulkanBase(GLFWwindow *window, ContextCreateInfo &createInfo)
{
    mInstanceM.createInstance(createInfo.getVersionMajor(), createInfo.getVersionMinor(),
                              createInfo.getValidationLayers(), createInfo.getInstanceExtensions());


    //Todo: Reread about the subject to understand why it was here
    //Instance =/= DebugMessenge
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

    //Initialize pipeline Cache
    mPipelineM.initialize(device, "");
}

//Todo: Depth Ressource  + Textjre
// Work on this
void VulkanContext::initRenderInfrastructure()
{
    std::cout << "initRenderInfrastructure" << std::endl;

    const VkDevice &device = mLogDeviceM.getLogicalDevice();
    const QueueFamilyIndices &indicesFamily = mPhysDeviceM.getIndices();
    mDepthRessources.createDepthBuffer(mLogDeviceM, mSwapChainM, mPhysDeviceM);

    // Defined the Render Pass Config
    RenderPassConfig defConfigRenderPass = vkUtils::RenderPass::makeDefaultConfig(mSwapChainM.getSwapChainImageFormat().format,
                                                                                  mDepthRessources.getFormat());
    mRenderPassM.createRenderPass(device, defConfigRenderPass);
    mSwapChainM.createFrameSwapChainRessources(device,{mDepthRessources.getView()},mRenderPassM.getRenderPass());
};

void VulkanContext::initPipelineAndDescriptors(const PipelineLayoutDescriptor& layoutConfig ,VertexFlags flag)
{
    std::cout << "initPipelineAndDescriptors" << std::endl;

    const VkDevice &device = mLogDeviceM.getLogicalDevice();

    //Todo: Push it upward ?
    PipelineBuilder builder;
    builder.setShaders({vertPath, fragPath})
    .setInputConfig({.vertexFormat = VertexFormatRegistry::getFormat(flag)}) 
    .setRenderPass(mRenderPassM.getRenderPass());
    //Todo:Not too sure of VertexFormatRegisty here
    //Todo:Not too sure of VertexFormatRegisty here
    //Todo:Not too sure of VertexFormatRegisty here
//
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

    mDepthRessources.destroyDepthBuffer(device);

    // Important
    // Todo: Better deletion of frames data
    for (int i = 0; i < mSwapChainM.GetSwapChainImageViews().size(); i++)
    {
        auto& frameData =  mSwapChainM.getCurrentFrameData();
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
