#pragma once

#include "BaseVk.h"

// Class built to create and Manage the VK Instance

class VulkanInstanceManager
{
public:

    VulkanInstanceManager() = default;
    //Recall destroy INstance here ?
    ~VulkanInstanceManager() = default;

    // Vulkan instance
    void createInstance();
    void destroyInstance();
    void setupDebugMessenger();

    VkInstance &getInstance();
    VkInstance *getInstancePtr();

private:
    std::vector<const char *> getRequiredExtensions();

    bool checkExtensionsSupport();

    //Validation Layer

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator);

    VkInstance instance = VK_NULL_HANDLE;
    // Debug Handling could be moved elewhere
    VkDebugUtilsMessengerEXT debugMessenger;
};
