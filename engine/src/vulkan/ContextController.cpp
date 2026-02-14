
#include "QueueFam.h"
#include "ContextController.h"

void VulkanContext::initVulkanBase(GLFWwindow *window, ContextCreateInfo &createInfo)
{
    mInstanceM.createInstance(createInfo.getVersionMajor(), createInfo.getVersionMinor(),
                              createInfo.getValidationLayers(), createInfo.getInstanceExtensions());

    // Todo: Reread about the subject to understand why it was here
    // Instance =/= DebugMessenge
    mInstanceM.setupDebugMessenger();

    // Surface
    // Todo: window abstraction
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
}

void VulkanContext::destroyAll()
{
    // Have destructor call those function
    VkDevice device = mLogDeviceM.getLogicalDevice();
    VmaAllocator allocator = mLogDeviceM.getVmaAllocator();

    // Important
    // Todo: Better deletion of frames data

    mSwapChainM.destroySwapChain(device);
    mLogDeviceM.destroyVmaAllocator();
    mLogDeviceM.DestroyDevice();
    mSwapChainM.destroySurface();
    mInstanceM.destroyInstance();
};

// This might be tied to an event
// This can more or less be activated already
void VulkanContext::recreateSwapchain(GLFWwindow *window)
{
    // Todo:
    // Indtroduce a way to upsacel actual gbuffers to swapcahin resolution
    //  Pause while Minimized
    // Todo remove
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

    mSwapChainM.destroySwapChain(device);
}

// Todo: Take care of this
#ifdef USE_VMA
// vmaCreateImage(...)
#else
// vkCreateImage(...)
#endif

Buffer VulkanContext::createBuffer(const BufferDesc &desc)
{
    Buffer buffer;

    VkBufferUsageFlags usageFlags = desc.usage;
    VkMemoryPropertyFlags memFlags = desc.memoryFlags;

    switch (desc.updatePolicy)
    {
    case BufferUpdatePolicy::Dynamic:
        memFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        break;
    case BufferUpdatePolicy::StagingOnly:
    case BufferUpdatePolicy::Immutable:
    default:
        // Device local only
        memFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        break;
    }

    buffer.createBuffer(
        mLogDeviceM.getLogicalDevice(),
        mPhysDeviceM.getPhysicalDevice(),
        desc.size,
        usageFlags,
        memFlags,
        mLogDeviceM.getVmaAllocator(),
        desc.updatePolicy);

    return buffer;
}

void VulkanContext::updateBuffer(Buffer &buffer, const void *data, VkDeviceSize size, VkDeviceSize offset)
{
    switch (buffer.getUpdatePolicy())
    {
    case BufferUpdatePolicy::Dynamic:
    {
        void *mapped = buffer.map(mLogDeviceM.getVmaAllocator());
        std::memcpy(static_cast<uint8_t *>(mapped) + offset, data, size);
        //buffer.unmap(); // if memory is coherent this may be a no-op
    }
    break;

    case BufferUpdatePolicy::StagingOnly:
        buffer.uploadStaged(data, size, offset, mPhysDeviceM.getPhysicalDevice(),
                            mLogDeviceM, mPhysDeviceM.getIndices().graphicsFamily.value(), mLogDeviceM.getVmaAllocator());
        break;
    case BufferUpdatePolicy::Immutable:
    default:
        // Do nothing: immutable buffers cannot be updated after creation
        break;
    }
}

Texture VulkanContext::createTexture(ImageData<stbi_uc> &cpuTexture)
{
    Texture tex;
    auto &deviceM = getLDevice();
    auto &physDevice = getPDeviceM();

    tex.createTextureImage(
        physDevice.getPhysicalDevice(),
        deviceM,
        cpuTexture,
        physDevice.getIndices().graphicsFamily.value(),
        deviceM.getVmaAllocator());
    tex.createTextureImageView(deviceM.getLogicalDevice());
    tex.createTextureSampler(deviceM.getLogicalDevice(), physDevice.getPhysicalDevice());

    return tex;
}
