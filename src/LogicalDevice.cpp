#include "LogicalDevice.h"
#include <set>
void LogicalDeviceManager::createLogicalDevice(VkPhysicalDevice physicalDevice, QueueFamilyIndices indices)
{
    // Logical device manager get the name of it's corresponding physical device
    // Reverse shoudl also be true

    //You could use here the QueueFamilyIndices already existing or get a new one by passing the physmanager
 
    std::set<uint32_t> uniqueQueueFamilies;
    for (const auto& family : {
        indices.graphicsFamily,
        indices.presentFamily,
        indices.computeFamily,
        indices.transferFamily
    }) {
        if (family.has_value()) {
            uniqueQueueFamilies.insert(family.value());
        }
    }
    float queuePriority = 1.0f;

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        // Only a small number of queue by queue family and some may not support more
        // 1 is enough for now
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    //Todo: Explicit activation ? 
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = ContextVk::contextInfo.getDeviceSelector().requireSamplerAnisotropy;
   
    // Share idea of vk istance create info
    // Device is then created using physicail device features and queue create
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    //Data and count probbly work together
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount =  static_cast<uint32_t>(queueCreateInfos.size());;
    createInfo.pEnabledFeatures = &deviceFeatures;


    // Extension
    createInfo.enabledExtensionCount = static_cast<uint32_t>(ContextVk::contextInfo.getDeviceExtensions().size());
    createInfo.ppEnabledExtensionNames = ContextVk::contextInfo.getDeviceExtensions().data();

    // ValidationLayer
    // Those are actually shared with instance validation layers in up to date vulkan standard
    // It was not the case before and they needed to be set
    // Even if we set them chance are they are not used 
    if (ContextVk::contextInfo.enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(ContextVk::contextInfo.getValidationLayers().size());
        createInfo.ppEnabledLayerNames = ContextVk::contextInfo.getValidationLayers().data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }


    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create logical device!");
    }

    if( ContextVk::contextInfo.getDeviceSelector().requireGraphics){
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &mGraphicsQueue);

    }
    if( ContextVk::contextInfo.getDeviceSelector().requireTransferQueue){
    vkGetDeviceQueue(device, indices.transferFamily.value(), 0, &mTransferQueue);

    }
    if( ContextVk::contextInfo.getDeviceSelector().requirePresent){
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &mPresentQueue);

    }
    if( ContextVk::contextInfo.getDeviceSelector().requireCompute){
    vkGetDeviceQueue(device, indices.computeFamily.value(), 0, &mComputeQueue);
        
    }


}

VkDevice LogicalDeviceManager::getLogicalDevice() const
{
    return device;
}

VkQueue LogicalDeviceManager::getQueue(uint32_t familyIndex, uint32_t queueIndex )  const{
{
     VkQueue queueRef;
   // vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &queueRef);
    return queueRef;
}

}

void LogicalDeviceManager::DestroyDevice()
{
    if( device ) { 
  vkDestroyDevice( device, nullptr ); 
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