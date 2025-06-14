#include "LogicalDevice.h"
#include <set>
void LogicalDeviceManager::createLogicalDevice(VkPhysicalDevice physicalDevice, QueueFamilyIndices indices)
{
    // Logical device manager get the name of it's corresponding physical device
    // Reverse shoudl also be true
    mPhysicalDevice = physicalDevice;

    std::set<uint32_t> uniqueQueueFamilies = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };

    float queuePriority = 1.0f;

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        // Only a small number of queue by queue family
        // 1 is enough
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    //OR directly request it
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    // Share idea of vk istance create info
    // Device is then created using physicail device features and queue create
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    //Data and count probbly work together
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount =  static_cast<uint32_t>(queueCreateInfos.size());;
    createInfo.pEnabledFeatures = &deviceFeatures;

    // Extension
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // ValidationLayer
    // Those are actually shared with  instance vlaidation layers hence ignored
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create logical device!");
    }

    getGraphicsQueue(indices, 0);
    getPresentQueue(indices, 0);


}

VkDevice LogicalDeviceManager::getLogicalDevice() const
{
    return device;
}

VkQueue LogicalDeviceManager::getGraphicsQueue(QueueFamilyIndices indices, int index)
{
    // Store the device queue in more accessible parameter
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), index, &mGraphicsQueue);
    return mGraphicsQueue;
}

VkQueue LogicalDeviceManager::getPresentQueue(QueueFamilyIndices indices, int index)
{
    // Store the device queue in more accessible parameter
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &mPresentQueue);
    return mPresentQueue;
}


void LogicalDeviceManager::DestroyDevice()
{
    vkDestroyDevice(device, nullptr);
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





VkResult LogicalDeviceManager::submitToGraphicsQueue(
    VkCommandBuffer * cmdBuffer,
    VkSemaphore waitSemaphore,
    VkSemaphore signalSemaphore,
    VkFence fence)
{
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &waitSemaphore;
    // Wait for semaphore "confirmation" during the color stage after vertexshader

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = cmdBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &signalSemaphore;
    
    // Specify the semapgore to signal
    return vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, fence);
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
