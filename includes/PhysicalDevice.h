#pragma once
#include "BaseVk.h"
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

    bool checkDeviceExtensionSupported(VkPhysicalDevice device);

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
    VkFormat findDepthFormat() const;
   
    //Indices
    QueueFamilyIndices getIndices() const{
      return mIndices;
    };

private:
    //Copy of instance handle should be fine
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    QueueFamilyIndices mIndices;
  
};
