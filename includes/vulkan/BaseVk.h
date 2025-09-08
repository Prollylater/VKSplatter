#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <vector>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

// TODO: Important
const std::string vertPath = "./ressources/shaders/vert.spv";
const std::string fragPath = "./ressources/shaders/frag.spv";
const std::string MODEL_PATH = "./ressources/models/hearthspring.obj";
const std::string TEXTURE_PATH = "./ressources/models/hearthspring.png";

// Also repass on fucntion while looking into the structure

struct DeviceSelectionCriteria
{
    // Somehow stand for queue and Shader
    bool requireGraphics = true;
    bool requirePresent = true;
    bool requireCompute = false;
    bool requireTransferQueue = false; // Not implemented yet

    bool requireGeometryShader = false;
    bool requireTessellationShader = false;
    bool requireSamplerAnisotropy = true;

    // depth Clamp/vias/multiple viewports
    VkPhysicalDeviceType preferredType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

    std::vector<const char *> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    // Todo: Implement builder here too
};

struct SwapChainConfig
{
    VkSurfaceFormatKHR preferredFormat = {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    std::vector<VkPresentModeKHR> preferredPresentModes = {
        VK_PRESENT_MODE_FIFO_KHR,
        VK_PRESENT_MODE_MAILBOX_KHR};
    // Triple buffering by default
    uint32_t preferredImageCount = 3;
    VkExtent2D preferredExtent = {0, 0}; // 0 means "derive size from window"
    bool allowExclusiveSharing = true;

    static SwapChainConfig Default() { return SwapChainConfig(); }

    // Setters
    SwapChainConfig &setPreferredFormat(VkSurfaceFormatKHR format)
    {
        preferredFormat = format;
        return *this;
    }

    SwapChainConfig &addPreferredPresentMode(VkPresentModeKHR mode)
    {
        preferredPresentModes.push_back(mode);
        return *this;
    }

    SwapChainConfig &clearPreferredPresentMode(VkPresentModeKHR mode)
    {
        preferredPresentModes.clear();
        return *this;
    }

    SwapChainConfig &setPreferredImageCount(uint32_t count)
    {
        preferredImageCount = count;
        return *this;
    }

    SwapChainConfig &setPreferredExtent(uint32_t width, uint32_t height)
    {
        preferredExtent = {width, height};
        return *this;
    }

    SwapChainConfig &allowSharing(bool allow)
    {
        allowExclusiveSharing = allow;
        return *this;
    }
};

class VulkanContext;

struct ContextCreateInfo
{
public:
    // Non const ?
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

    // friend VulkanContext;
    uint32_t versionMajor = 1;
    uint32_t versionMinor = 2;

    // Configuration
    std::vector<const char *> validationLayers = {
        "VK_LAYER_KHRONOS_validation"};

    std::vector<const char *> instanceExtensions;
    

    bool enableValidationLayers =
#ifndef NDEBUG
        true;
#else
        false;
#endif

    DeviceSelectionCriteria selectionCriteria;
    SwapChainConfig swapchainConfig;
    // Getters
    uint32_t getVersionMajor() const { return versionMajor; }
    uint32_t getVersionMinor() const { return versionMinor; }

    const std::vector<const char *> &getValidationLayers() const
    {
        static const std::vector<const char *> dummy;
        if (enableValidationLayers)
        {
            return validationLayers;
        }
        return dummy;
    };

    static ContextCreateInfo Default() { return ContextCreateInfo(); }
    const std::vector<const char *> &getInstanceExtensions() const { return instanceExtensions; }
    const std::vector<const char *> &getDeviceExtensions() const { return selectionCriteria.deviceExtensions; }
    const DeviceSelectionCriteria &getDeviceSelector() const { return selectionCriteria; }
    SwapChainConfig &getSwapChainConfig() { return swapchainConfig; }

    bool isValidationLayersEnabled() const { return enableValidationLayers; }

private:
    // Setters
    ContextCreateInfo &setApiVersion(uint32_t major, uint32_t minor)
    {
        versionMajor = major;
        versionMinor = minor;
        return *this;
    };

    ContextCreateInfo &setEnableValidationLayers(bool enable)
    {
        enableValidationLayers = enable;
        return *this;
    };

    ContextCreateInfo &setValidationLayers(const std::vector<const char *> &layers)
    {
        validationLayers = layers;
        return *this;
    };

    ContextCreateInfo &addValidationLayer(const char *layer)
    {
        validationLayers.push_back(layer);
        return *this;
    };

    ContextCreateInfo &setInstanceExtensions(const std::vector<const char *> &exts)
    {
        instanceExtensions = exts;
        return *this;
    };

    ContextCreateInfo &addInstanceExtension(const char *name)
    {
        instanceExtensions.push_back(name);
        return *this;
    };

    ContextCreateInfo &setDeviceExtensions(const std::vector<const char *> &exts)
    {
        selectionCriteria.deviceExtensions = exts;
        return *this;
    };

    ContextCreateInfo &addDeviceExtension(const char *name)
    {
        selectionCriteria.deviceExtensions.push_back(name);
        return *this;
    };

    ContextCreateInfo &setDeviceSelectionCriteria(const DeviceSelectionCriteria &criteria)
    {
        selectionCriteria = criteria;
        return *this;
    };
};

// Todo; Also Spir V reflect seem to the best way to go about this
// Todo; Not sure about the position
struct PipelineLayoutDescriptor
{
    std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayouts;
    std::vector<VkPushConstantRange> pushConstants;

    void addDescriptor(uint32_t location, VkDescriptorType type, VkShaderStageFlags stageFlag,  uint32_t count =1,
                                 const VkSampler *sampler = nullptr)
    {
        descriptorSetLayouts.push_back({location, type, count, stageFlag, sampler});
    }

    void addPushConstant(VkShaderStageFlags stageFlag, uint32_t offset, uint32_t size)
    {
        pushConstants.push_back({stageFlag, offset, size});
    }

    
};

// Or have the setter being private only accessible

// Should end up being a Variable passed into The context

/*
A frame in flight refers to a rendering operation that
 has been submitted to the GPU but has not yet finished rendering in flight

    The CPU can prepare the next frame while (recordCommand Buffer)
    The GPU is still rendering the previous frame(s)
    So CPU could record Frame 1 and 2 while GPU is still on frame 0
    Cpu can then move on on more meaningful task if fence allow it

    Thingaffected:
    Frames, Uniform Buffer (Since we update and send it for each record)

    ThingUnaffected:
    OR not ? It's weird wait for the chapter
    Depth Buffer/Stencil Buffer (Only used during rendering. Sent for rendering, consumed there and  ignried)
*/
