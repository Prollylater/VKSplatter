
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


QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface, bool) {
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


//Expanded to take into account new version
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface, const DeviceSelectionCriteria& criteria) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        const auto& q = queueFamilies[i];

        if (criteria.requireGraphics && (q.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !indices.graphicsFamily) {
            indices.graphicsFamily = i;
        }

        if (criteria.requirePresent && !indices.presentFamily) {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) {
                indices.presentFamily = i;
            }
        }
        
        if (criteria.requireCompute && (q.queueFlags & VK_QUEUE_COMPUTE_BIT) && !indices.computeFamily) {
            // Prefer a dedicated compute queue
            if (!(q.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                indices.computeFamily = i;
            }
        }

        if (criteria.requireTransferQueue && (q.queueFlags & VK_QUEUE_TRANSFER_BIT) && !indices.transferFamily) {
            // Prefer a dedicated transfer queue
            if (!(q.queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(q.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                indices.transferFamily = i;
            }
        }

        // If not found a dedicated one, take any valid one
        if (criteria.requireCompute && !indices.computeFamily && (q.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            indices.computeFamily = i;
        }

        if (criteria.requireTransferQueue && !indices.transferFamily && (q.queueFlags & VK_QUEUE_TRANSFER_BIT)) {
            indices.transferFamily = i;
        }

        if (indices.isComplete(criteria)) {
            break;
        }
    }

    return indices;
}


