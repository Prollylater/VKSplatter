
#include "QueueFam.h"



///////////////////////////////////
//Queue 
///////////////////////////////////

//Some doubt about the usage of this class
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;
    // Logic to find queue family indices to populate struct with

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (int i = 0; i < static_cast<int>(queueFamilies.size()); i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
            break;
        }
    }
    return indices;
}


//Some doubt about the usage and definition of this class


QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices;
    // Logic to find queue family indices to populate struct with

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    //Graphic, compute transfer bit
    for (int i = 0; i < static_cast<int>(queueFamilies.size()); i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }
    }
    return indices;
}




/*
Why the different treamtent
Present support is platform-specific and surface-specific.
 A queue family might support graphics operations, 
 but not be able to present images to a given surface — especially on systems with multiple GPUs
Thus physical device propeties would not cut 
*/