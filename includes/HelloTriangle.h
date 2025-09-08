#include "BaseVk.h"
#include "ContextController.h"
#include "Renderer.h"

//Our Application only use one physical device and one logical device


/*
As you'll see, the general pattern that object creation function parameters in Vulkan follow is:

Pointer to struct with creation info
Pointer to custom allocator callbacks, always nullptr in this tutorial
Pointer to the variable that stores the handle to the new object
*/
class HelloTriangleApplication
{
public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}


private:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();


    VulkanContext context;
    Renderer renderer; 

    bool framebufferResized;
    GLFWwindow *window;
};


