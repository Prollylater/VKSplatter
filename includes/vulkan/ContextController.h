
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

// Todo: Which heap allocation are absolutely needed
class VulkanContext
{
public:
    // Core Vulkan handles, direct access
    VulkanInstanceManager mInstanceM;
    PhysicalDeviceManager mPhysDeviceM;
    LogicalDeviceManager mLogDeviceM;
    SwapChainManager mSwapChainM;

    /*
    Probably also in Renderer
    Once the archtiecture get clearer
    // Global
    vkUtils::DeletionQueue deletionQueue;

    deletionQueue.push([=]() {
        vkDestroyBuffer(device, buffer, nullptr);
    });
    deletionQueue.flush();
    */

    // Getters returning const references
    // const VulkanInstanceManager &getInstanceManager() const { return mInstanceM; }
    // VulkanInstanceManager &getInstanceManager() { return mInstanceM; }

    const PhysicalDeviceManager &getPhysicalDeviceManager() const { return mPhysDeviceM; }
    const LogicalDeviceManager &getLogicalDeviceManager() const { return mLogDeviceM; }

    const SwapChainManager &getSwapChainManager() const { return mSwapChainM; }

    PhysicalDeviceManager &getPhysicalDeviceManager() { return mPhysDeviceM; }
    LogicalDeviceManager &getLogicalDeviceManager() { return mLogDeviceM; }

    SwapChainManager &getSwapChainManager() { return mSwapChainM; }

    // Not too sure about what init  or not and in private or not

    void initVulkanBase(GLFWwindow *window, ContextCreateInfo &createInfo);

    void recreateSwapchain(GLFWwindow *window);

    void destroyAll();

private:
    // TODO: Memory allocator, debug messenger, etc.
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

