#pragma once

#include "BaseVk.h"
#include <optional>


/*
VK_QUEUE_GRAPHICS_BIT	Can handle graphics (rendering) commands.
VK_QUEUE_COMPUTE_BIT	Can handle compute shaders.
VK_QUEUE_TRANSFER_BIT	Can do memory transfers (copy, etc.).
VK_QUEUE_SPARSE_BINDING_BIT	Can bind sparse resources.
VK_QUEUE_PROTECTED_BIT	Can handle protected (e.g., DRM) comman
*/

//Todo:
//Currently we privilegiate not sharing compute and transfer family
//This was a naive design but this should be rethought through
//Perhaps with a way to know when we share or not a family
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> transferFamily;
    std::optional<uint32_t> presentFamily;

    int indicesCount;

    bool isComplete() {
        return graphicsFamily.has_value()
        && presentFamily.has_value();
    }
    bool isComplete(const DeviceSelectionCriteria& criteria) const {
        
        if (criteria.requireGraphics && !graphicsFamily.has_value()) {return false;};
        if (criteria.requireCompute && !computeFamily.has_value()) {return false;};
        if (criteria.requireTransferQueue && !transferFamily.has_value()){ return false;};
        if (criteria.requirePresent && !presentFamily.has_value()) return false;
        return true;
    }
};



///////////////////////////////////
//Queue 
///////////////////////////////////

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface, const DeviceSelectionCriteria& criteria);
/*
Why the different treamtent
Present support is platform-specific and surface-specific.
 A queue family might support graphics operations, 
 but not be able to present images to a given surface — especially on systems with multiple GPUs
Thus physical device propeties would not cut 
*/