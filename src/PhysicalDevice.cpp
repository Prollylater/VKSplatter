#include "PhysicalDevice.h"
#include "SwapChain.h"

#include <unordered_set>
void PhysicalDeviceManager::pickPhysicalDevice(VkInstance instance, const SwapChainManager &swapManager)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::cout << "Initialized " << deviceCount << " GPUs" << std::endl;

    // Actual selection among the suitable GPU

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // Multip map example could  be nice to have a second device but not too useful here

    int bestCandidateIndex = 0;
    int bestScore = 0;

    for (int index = 0; index < static_cast<int>(devices.size()); index++)
    {
        int score = rateDeviceSuitability(devices[index], swapManager);
        if (score > bestScore)
        {
            bestCandidateIndex = index;
            bestScore = score;
        }
    };

    // Check if the best candidate is suitable at all
    if (bestScore > 0)
    {
        physicalDevice = devices[bestCandidateIndex];

        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
        std::cout << "Selected Device: " << deviceProperties.deviceName << std::endl;
    }
    else
    {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

VkPhysicalDevice PhysicalDeviceManager::getPhysicalDevice() const
{
    return physicalDevice;
}

// Concern the suitability Indiices wise
bool PhysicalDeviceManager::isDeviceQueueSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    // add even if not suitable
    mIndices = findQueueFamilies(device, surface);
    return mIndices.isComplete();
}

bool PhysicalDeviceManager::checkDeviceExtensionSupported(VkPhysicalDevice device)
{
    // Again double enumerate in a std::vector data pattern
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::unordered_set<std::string> requiredExtensions;
    for (const auto &ext : availableExtensions)
    {
        requiredExtensions.insert(ext.extensionName);
    }

    // If one required is not found early exist
    for (const auto &required : ContextVk::contextInfo.deviceExtensions)
    {
        if (requiredExtensions.find(required) == requiredExtensions.end())
        {
            return false;
        }
    }

    return true;
}

int PhysicalDeviceManager::rateDeviceSuitability(VkPhysicalDevice device, const SwapChainManager &swapManager)
{
    int score = 0;

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    if (!deviceFeatures.geometryShader || !deviceFeatures.tessellationShader || !deviceFeatures.samplerAnisotropy)
    {
        return 0;
    }

    // deviceFeatures.samplerAnisotropy is optional so something should just carry the option so taht we don't use it later

    // Contorl how are supported some additonal features
    if (!checkDeviceExtensionSupported(device))
    {
        return 0;
    };

    if (!isDeviceQueueSuitable(device, swapManager.GetSurface()))
    {
        return 0;
    }

    // Swap chain suitability
    bool swapChainAdequate = false;
    SwapChainSupportDetails swapChainSupport = swapManager.QuerySwapChainSupport(device);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();

    if (!swapChainAdequate)
    {
        return 0;
    }

    // Rating

    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 1000;
    }

    score += deviceProperties.limits.maxImageDimension2D;

    return score;
}

VkFormat PhysicalDeviceManager::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
{

    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    //Change this
    throw std::runtime_error("Failed to find supported format!");
}


VkFormat PhysicalDeviceManager::findDepthFormat() const
{
   return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}


// So device may not support, queue we cant , extension we want, featyurse we want