#pragma once

#include "BaseVk.h"


class VulkanInstanceManager
{
public:

    VulkanInstanceManager() = default;
    ~VulkanInstanceManager() = default;

    // Vulkan instance
    void createInstance(uint32_t majorVer, uint32_t minorVer, const std::vector<const char *> & validationLayers, const std::vector<const char *> & instanceExt);
    void destroyInstance();

    VkInstance &getInstance();
    VkInstance *getInstancePtr();

    //Todo:
    void setupDebugMessenger();
    
private:
    std::vector<const char *> getRequiredInstanceExt();

    bool checkExtensionsSupport();

    //Validation Layer

    VkInstance mInstance = VK_NULL_HANDLE;
    bool mEnabledValidationLayer = true;


    // Debug Handling could be moved elewhere
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator);


};
