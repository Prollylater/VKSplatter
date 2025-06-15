#include "VulkanInstance.h"
#include "VulkanUtils.h"


//Vulkan instance
void VulkanInstanceManager::createInstance(){


    if (ContextVk::contextInfo.enableValidationLayers && !checkValidationLayerSupport())
    {
        throw std::runtime_error("validation layers requested, but not available!");
    }
    // Optional but potentially useful
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Important
    // Tell Vulkan driver which extensions and validation layers to use
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Extensions
    //  fill extensioncount with the number of extensions required to interface with the window
    // Fill glfwExtensions with them
    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();
    
    // Additional debug utils for creation and destroy
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

    // Global validation Layers
    if (ContextVk::contextInfo.enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(ContextVk::contextInfo.validationLayers.size());
        createInfo.ppEnabledLayerNames = ContextVk::contextInfo.validationLayers.data();
        populateDebugMessengerCreateInfo(debugCreateInfo);
        // Create a separate  messenger that can handle create and failure
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

    if (result != VK_SUCCESS)
    {
           std::cerr << "vkCreateInstance failed with error code: " << result << std::endl;
             throw std::runtime_error("failed to create instance!");
    }


};


void VulkanInstanceManager::destroyInstance(){

    if (ContextVk::contextInfo.enableValidationLayers)
    {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
    vkDestroyInstance(instance, nullptr);
};

VkInstance& VulkanInstanceManager::getInstance(){
    return instance;
};
VkInstance* VulkanInstanceManager::getInstancePtr(){
    return &instance;
};

// Extensions
std::vector<const char *> VulkanInstanceManager::getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // Pointer to array of pointer
    // Pointer are ultimately C-Style string
    // Similar to const char*[] and const char**
    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (ContextVk::contextInfo.enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

bool VulkanInstanceManager::checkExtensionsSupport()
{
    // Additonal test
    // Can't get both elements at the same time
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    std::cout << "available extensions: " << extensionCount << " \n ";

    for (const auto &extension : extensions)
    {
        std::cout << '\t' << extension.extensionName << '\n';
    }
    return false;
}

////////////////////////////////////////
//Validation Layer set up
////////////////////////////////////////



// Debug Call nacl
/*
//messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: Diagnostic message
VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: Informational message like the creation of a resource
VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: Message about behavior that is not necessarily an error, but very likely a bug in your application
VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: Message about behavior that is invalid and may cause crashes


//
messageType parameter can have the following values:
VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: Some event has happened that is unrelated to the specification or performance
VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: Something has happened that violates the specification or indicates a possible mistake
VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: Potential non-optimal use of Vulkans

//VkDebugUtilsMessengerCallbackDataEXT parameter
pMessage: The debug message as a null-terminated string
pObjects: Array of Vulkan object handles related to the message
objectCount: Number of objects in array
*/

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData)
{

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    // Return true == Abort call that triggered this
    return VK_FALSE;
}

// Fetching an extension function

void VulkanInstanceManager::setupDebugMessenger()
{
    if (!ContextVk::contextInfo.enableValidationLayers)
        return;

    // Used to captrue events occuring while creating and destroying an instance of a applicatrion
    // So with or debuger not yet created
    // passed to the instance
    // Create info
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void VulkanInstanceManager::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
{
    // Setting all element handled by  the utils

    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    // pointerFunction custom call back
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr; // Optionals
}


// Object creation  with pointer to object debug function, no allcoator
// Should not be owned

VkResult VulkanInstanceManager::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void VulkanInstanceManager::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}


/////////////////////////////
//Callback
////////////////////////