#include "LogicalDevice.h"
#include <set>

void LogicalDeviceManager::createLogicalDevice(VkPhysicalDevice physicalDevice, QueueFamilyIndices indices,
                                               const std::vector<const char *> &validationLayers, const DeviceSelectionCriteria &criteria)
{
    std::set<uint32_t> uniqueQueueFamilies;
    for (const auto &family : {
             indices.graphicsFamily,
             indices.presentFamily,
             indices.computeFamily,
             indices.transferFamily})
    {
        if (family.has_value())
        {
            uniqueQueueFamilies.insert(family.value());
        }
    }

    float queuePriority = 1.0f;

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queueFamily,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        };
        // Only a small number of queue by queue family and some may not support more
        // 1 is enough for now
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    VkPhysicalDeviceFeatures2 deviceFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &features13};
    // vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures);

    features13.dynamicRendering = criteria.requireDynamicRendering;
    features13.synchronization2 = criteria.requireSynchronization2;
    deviceFeatures.features.samplerAnisotropy = criteria.requireSamplerAnisotropy;

    VkDeviceCreateInfo createInfo
    {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &deviceFeatures,
            .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        //.pEnabledFeatures = &deviceFeatures,
            .enabledExtensionCount = static_cast<uint32_t>(criteria.deviceExtensions.size()),
        .ppEnabledExtensionNames = criteria.deviceExtensions.data()
    };

    // ValidationLayer
    // Those are actually shared with instance validation layers in up to date vulkan standard
    // It was not the case before and they needed to be set
    // Even if we set them chance are they are not used
    if (!validationLayers.empty())
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create logical device!");
    }

    if (criteria.requireGraphics)
    {
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &mGraphicsQueue);
    }
    if (criteria.requireTransferQueue)
    {
        vkGetDeviceQueue(device, indices.transferFamily.value(), 0, &mTransferQueue);
    }
    if (criteria.requirePresent)
    {
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &mPresentQueue);
    }
    if (criteria.requireCompute)
    {
        vkGetDeviceQueue(device, indices.computeFamily.value(), 0, &mComputeQueue);
    }
}

VkDevice LogicalDeviceManager::getLogicalDevice() const
{
    return device;
}

VkQueue LogicalDeviceManager::getQueue(uint32_t familyIndex, uint32_t queueIndex) const
{
    VkQueue queueRef;
    // vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &queueRef);
    return queueRef;
}

void LogicalDeviceManager::DestroyDevice()
{
    if (device)
    {
        vkDestroyDevice(device, nullptr);
        device = VK_NULL_HANDLE;
    }
}

/*

vkEnumeratePhysicalDevices() → get a list of physical devices and then
inspect each VkPhysicalDevice → Using vkGetPhysicalDeviceQueueFamilyProperties() to find which queue families support graphics, compute, transfer,bit and presentation, etc.
THen : VkDeviceQueueCreateInfo[] → Create info basically saying  qeueu xx shoudld be in
Passed above element in  VkDeviceCreateInfo → Along with optional features, layers, extensions this define what is needed of this device queue

vkCreateDevice() → Vulkan creates a logical device handle, giving you access to the queues you asked for.

Then during operation we have
vkGetDeviceQueue() → retrieve the actual VkQueue handles from the logical device.
*/

// Helper used for submiting to Frame GQueue
VkResult LogicalDeviceManager::submitFrameToGQueue(
    VkCommandBuffer cmdBuffer,
    VkSemaphore waitSemaphore,
    VkSemaphore signalSemaphore,
    VkFence fence)
{
    std::vector<VkCommandBuffer> cmdBuffers = {cmdBuffer};

    std::vector<VkSemaphore> waitSemaphores = {waitSemaphore};
    // Wait for semaphore "confirmation" during the color stage after vertexshader
    // Could be VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
    std::vector<VkPipelineStageFlags> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    std::vector<VkSemaphore> signalSemaphores = {signalSemaphore};

    return submitToQueue(mGraphicsQueue,
                         cmdBuffers,
                         waitSemaphores,
                         waitStages,
                         signalSemaphores,
                         fence);
}

VkResult LogicalDeviceManager::submitToQueue(
    VkQueue queue,
    const std::vector<VkCommandBuffer> &cmdBuffers,
    const std::vector<VkSemaphore> &waitSemaphores,
    const std::vector<VkPipelineStageFlags> &waitStages,
    const std::vector<VkSemaphore> &signalSemaphores,
    VkFence fence)
{
    // Todo:
    // Check if commmand buffer are used, from the correct type of queue
    //, or were recorded with a VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT flag
    // Could be done above this function
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = waitStages.data();

    submitInfo.commandBufferCount = static_cast<uint32_t>(cmdBuffers.size());
    submitInfo.pCommandBuffers = cmdBuffers.data();

    submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
    submitInfo.pSignalSemaphores = signalSemaphores.data();

    return vkQueueSubmit(queue, 1, &submitInfo, fence);
}

VkResult LogicalDeviceManager::presentImage(
    VkSwapchainKHR swapchain,
    VkSemaphore waitSemaphore,
    uint32_t imageIndex)
{
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &waitSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;
    return vkQueuePresentKHR(mPresentQueue, &presentInfo);
}
