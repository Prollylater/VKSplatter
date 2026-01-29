
#pragma once

// SOme need stuff is removable

#include "VulkanInstance.h"
#include "LogicalDevice.h"
#include "PhysicalDevice.h"
#include "SwapChain.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "CommandPool.h"

#include "SyncObjects.h"
#include "Buffer.h"
#include "Texture.h"

// Todo: Which heap allocation are absolutely needed
class VulkanContext
{
public:
    VulkanContext() = default;
    ~VulkanContext() { destroyAll(); }

    void initVulkanBase(GLFWwindow *window, ContextCreateInfo &createInfo);
    void recreateSwapchain(GLFWwindow *window);
    void destroyAll();

    const PhysicalDeviceManager &getPDeviceM() const { return mPhysDeviceM; }
    PhysicalDeviceManager &getPDeviceM() { return mPhysDeviceM; }

    const LogicalDeviceManager &getLDevice() const { return mLogDeviceM; }
    LogicalDeviceManager &getLDevice() { return mLogDeviceM; }

    const SwapChainManager &getSwapChainManager() const { return mSwapChainM; }
    SwapChainManager &getSwapChainManager() { return mSwapChainM; }

    //Most can be const
    Buffer createBuffer(const BufferDesc &desc);
    void updateBuffer(Buffer &buffer, const void *data, VkDeviceSize size, VkDeviceSize offset = 0);
    Texture createTexture( ImageData<stbi_uc> &cpuTexture);

    PhysicalDeviceManager mPhysDeviceM;
    LogicalDeviceManager mLogDeviceM;
    SwapChainManager mSwapChainM;
    VulkanInstanceManager mInstanceM;


private:
};

/*
A frame in flight refers to a rendering operation that
 has been submitted to the GPU but has not yet finished rendering in flight

    The CPU can prepare the next frame while (recordCommand Buffer)
    The GPU is still rendering the previous frame(s)
    So CPU could record Frame 1 and 2 while GPU is still on frame 0
    Cpu can then move on on more meaningful task if fence allow it

    Thingaffected:
    Frames, Uniform Buffer (Since we update and send it for each record)

    ThingUnaffected:
    OR not ? It's weird wait for the chapter
    Depth Buffer/Stencil Buffer (Only used during rendering. Sent for rendering, consumed there and  ignried)
*/
