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

//So far, not too sure how to build this class
enum class enumQueueFamilyIndices{
    GRAPHICS
};

//Graphics and Compute Implictly handle transfer

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    
    //std::set
    int indicesCount;

    bool isComplete() {
        return graphicsFamily.has_value()
        && presentFamily.has_value();
    }
};


///////////////////////////////////
//Queue 
///////////////////////////////////

//Some doubt about the usage of this class
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, int dummy);

//Some doubt about the usage and definition of this class


QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);


/*
Why the different treamtent
Present support is platform-specific and surface-specific.
 A queue family might support graphics operations, 
 but not be able to present images to a given surface — especially on systems with multiple GPUs
Thus physical device propeties would not cut 
*/