#pragma once

#define GLFW_INCLUDE_NONE      
#define VULKAN_HPP_RAII_ENABLE 
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_raii.hpp> 
#include <GLFW/glfw3.h>           
#include "WindowInterface.h"

static bool s_glfwInitialized = false;
static bool s_glfwWinCount = 0;

//Replace by a Window tied to OS and not the graphic API
class VulkanWindow : public AbstractWindow
{
public:
    VulkanWindow() = default;
    ~VulkanWindow();

    bool init(const std::string &title, uint32_t width, uint32_t height) override;
    void onUpdate() override;

    bool isOpen() const override;
    void close() override;

    void setEventCallback(const EvtCallback &callback) override;
    uint32_t getWidth() const override;
    uint32_t getHeight() const override;

    void *getNativeWindowHandle() const override;
    GLFWwindow *getGLFWWindow();

    // VkSurfaceKHR getSurface() const { return surface; }

private:
    // VkSurfaceKHR surface;
    GLFWwindow *mWindow;
    uint32_t width;
    uint32_t height;
    EvtCallback eventCallback;

    // Todo: Framebuffer indepent from this display  ?
    static void framebufferResizeCallback(GLFWwindow *window, int width, int height)
    {
        // auto app = reinterpret_cast<HelloTriangleApplication *>(glfwGetWindowUserPointer(window));
        // app->framebufferResized = true;
    }
};
