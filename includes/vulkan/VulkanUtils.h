#pragma once

#include "BaseVk.h"

// Validation Layer Control
bool checkValidationLayerSupport(const std::vector<const char *> validationsLayers)
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName : validationsLayers)
    {
        bool layerFound = std::any_of(availableLayers.begin(), availableLayers.end(),
                                      [layerName](const VkLayerProperties &props)
                                      {
                                          return strcmp(props.layerName, layerName) == 0;
                                      });

        if (!layerFound)
        {
            return false;
        }
    }
    return true;
}