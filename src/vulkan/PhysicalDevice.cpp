#include "PhysicalDevice.h"
#include "SwapChain.h"

#include <unordered_set>

void PhysicalDeviceManager::pickPhysicalDevice(VkInstance instance, const SwapChainManager &swapManager, const DeviceSelectionCriteria& criteria)
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
        int score = rateDeviceSuitability(devices[index], swapManager, criteria);
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
        mMsaaSamples = getMaxUsableSampleCount();
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
bool PhysicalDeviceManager::isDeviceQueueSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, const DeviceSelectionCriteria& criteria)
{
    mIndices = findQueueFamilies(device, surface, criteria);
    return mIndices.isComplete(criteria);
}


bool PhysicalDeviceManager::areRequiredExtensionsSupported(VkPhysicalDevice device, const std::vector<const char *> & deviceExtensions)
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
    for (const auto &required : deviceExtensions)
    {
        if (requiredExtensions.find(required) == requiredExtensions.end())
        {
            return false;
        }
    }

    return true;
}

int PhysicalDeviceManager::rateDeviceSuitability(VkPhysicalDevice device, const SwapChainManager &swapManager, const DeviceSelectionCriteria& criteria)
{
    int score = 0;

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    /*
    if (!deviceFeatures.geometryShader || !deviceFeatures.tessellationShader || !deviceFeatures.samplerAnisotropy)
    {
        return 0;
    }*/

    // Must-have features
    if (criteria.requireGeometryShader && !deviceFeatures.geometryShader)
    {
        return 0;
    }

    //Other features
    if (criteria.requireTessellationShader && !deviceFeatures.tessellationShader)
    {
        return 0;
    }
    if (criteria.requireSamplerAnisotropy && !deviceFeatures.samplerAnisotropy)
    {
        return 0;
    }

    // Required extensions
    if (!areRequiredExtensionsSupported(device, criteria.deviceExtensions))
    {
        return 0;
    };

    // Queue family support
    if (!isDeviceQueueSuitable(device, swapManager.GetSurface(), criteria))
    {
        return 0;
    }

    // Swap chain suitability
    bool swapChainAdequate = false;
    SwapChainSupportDetails swapChainSupport = swapManager.querySwapChainSupport(device);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();

    if (!swapChainAdequate)
    {
        return 0;
    }

    // Rating

    if (deviceProperties.deviceType == criteria.preferredType)
    {
        score += 1000;
    }

    score += deviceProperties.limits.maxImageDimension2D;

    return score;
}

// TODO: Must be adapted to handlded specific buffer + texture
VkFormat PhysicalDeviceManager::findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
{
    return vkUtils::Format::findSupportedFormat(physicalDevice, candidates, tiling, features);
}

VkFormat PhysicalDeviceManager::findDepthFormat() const
{
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkSampleCountFlagBits PhysicalDeviceManager::getMaxUsableSampleCount()
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
                                physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT)
    {
        return VK_SAMPLE_COUNT_64_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_32_BIT)
    {
        return VK_SAMPLE_COUNT_32_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_16_BIT)
    {
        return VK_SAMPLE_COUNT_16_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_8_BIT)
    {
        return VK_SAMPLE_COUNT_8_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_4_BIT)
    {
        return VK_SAMPLE_COUNT_4_BIT;
    }
    if (counts & VK_SAMPLE_COUNT_2_BIT)
    {
        return VK_SAMPLE_COUNT_2_BIT;
    }

    return VK_SAMPLE_COUNT_1_BIT;
}

// So device may not support, queue we cant , extension we want, featyurse we want

namespace vkUtils
{
    namespace Format
    {

        VkFormat findSupportedFormat(VkPhysicalDevice physDevice, const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) 
        {
            // Used to create Sampled  Image
            for (VkFormat format : candidates)
            {
                VkFormatProperties props;
                vkGetPhysicalDeviceFormatProperties(physDevice, format, &props);

                if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) //!=0
                {
                    return format;
                }
                else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
                {
                    return format;
                }
            }
           return VK_FORMAT_UNDEFINED;
        }

    }
}
