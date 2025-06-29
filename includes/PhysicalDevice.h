#pragma once
#include "BaseVk.h"
#include "QueueFam.h"

///////////////////////////////////
//Physical device handling
///////////////////////////////////
class SwapChainManager;




class PhysicalDeviceManager {
public:
    PhysicalDeviceManager()= default;
   ~PhysicalDeviceManager()= default;

    void pickPhysicalDevice(VkInstance instance,const SwapChainManager& );
    VkPhysicalDevice getPhysicalDevice() const;

  //Should be static and deal with much more than Device handled
    bool isDeviceQueueSuitable(VkPhysicalDevice device,VkSurfaceKHR surface);
    int rateDeviceSuitability(VkPhysicalDevice device,const SwapChainManager& );

    bool areRequiredExtensionsSupported(VkPhysicalDevice );

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
    VkFormat findDepthFormat() const;
   
    VkSampleCountFlagBits getMaxUsableSampleCount();
    VkSampleCountFlagBits getMsaaSample() const {
      return mMsaaSamples;
    };

    //Indices
    QueueFamilyIndices getIndices() const{
      return mIndices;
    };

    void setSelectionCriteria(const DeviceSelectionCriteria& criteria);
private:
    //Copy of instance handle should be fine
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    QueueFamilyIndices mIndices;
    VkSampleCountFlagBits mMsaaSamples;
};


/*

DeviceSelectionCriteria criteria;
criteria.requireTessellationShader = false;
criteria.requiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

PhysicalDeviceManager pdm;
pdm.setSelectionCriteria(criteria);
pdm.pickPhysicalDevice(instance, swapManager);

*/