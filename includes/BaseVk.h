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
const std::string vertPath = "./ressources/shaders/vert.spv";
const std::string fragPath = "./ressources/shaders/frag.spv";
const std::string MODEL_PATH = "./ressources/models/hearthspring.obj";
const std::string TEXTURE_PATH = "./ressources/models/hearthspring.png";

// Also repass on fucntion while looking into the structure

struct DeviceSelectionCriteria {
    bool requireGraphics = true;
    bool requirePresent = true;
    bool requireCompute = false;
    bool requireTransferQueue = false; //Not implemented yet

    bool requireGeometryShader = true;
    bool requireTessellationShader = true;
    bool requireSamplerAnisotropy = true;


    //Other features to look into

    //depth Clamp/vias/multiple viewports
    VkPhysicalDeviceType preferredType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;


};


class VulkanContext;
//Should eventually not be a variable but just a bunch of static element
struct ContextCreateInfo
{
public:
    const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    //friend VulkanContext;
    uint32_t versionMajor = 1;
    uint32_t versionMinor = 2;

    // Configuration
    std::vector<const char *> validationLayers = {
        "VK_LAYER_KHRONOS_validation"};

    std::vector<const char *> instanceExtensions;

    std::vector<const char *> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    bool enableValidationLayers =
#ifndef NDEBUG
        true;
#else
        false;
#endif


    DeviceSelectionCriteria selectionCriteria;
    // Getters
    uint32_t getVersionMajor() const { return versionMajor; }
    uint32_t getVersionMinor() const { return versionMinor; }

    const std::vector<const char *> &getValidationLayers() const { return validationLayers; }
    const std::vector<const char *> &getInstanceExtensions() const { return instanceExtensions; }
    const std::vector<const char *> &getDeviceExtensions() const { return deviceExtensions; }
    const DeviceSelectionCriteria &getDeviceSelector() const { return selectionCriteria; }

    bool isValidationLayersEnabled() const { return enableValidationLayers; }

    // Setters
private:
    void setVersionMajor(uint32_t major) { versionMajor = major; }
    void setVersionMinor(uint32_t minor) { versionMinor = minor; }

    void setValidationLayers(const std::vector<const char *> &layers) { validationLayers = layers; }
    void setInstanceExtensions(const std::vector<const char *> &extensions) { instanceExtensions = extensions; }
    void setDeviceExtensions(const std::vector<const char *> &extensions) { deviceExtensions = extensions; }
    
    void addDeviceExtension(const char *name, bool optional = false, void *featureStruct = nullptr);
    void addInstanceExtension(const char *name, bool optional = false, void *featureStruct = nullptr);

    void setEnableValidationLayers(bool enable) { enableValidationLayers = enable; };
};

// Or have the setter being private only accessible


//Should end up being a Variable passed into The context 
namespace ContextVk
{
    inline ContextCreateInfo contextInfo;
}

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
 