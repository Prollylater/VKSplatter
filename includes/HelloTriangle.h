#include "BaseVk.h"
#include "ContextController.h"
#include "Renderer.h"
#include "ResourceSystem.h"
#include "WindowVk.h"
#include "Clock.h"
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
        clock.reset();
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
    void initScene();
    void mainLoop();
    void cleanup();
    void onEvent(Event& event);

    VulkanContext context;
    Renderer renderer; 
    AssetSystem assetSystem; 
    Scene logicScene;//Scenegraph/ECS

    
    //Window abstraction for close, cleaner resize, pollingEvents too
    VulkanWindow window;
    //Window* window;

    //Application State
    cico::Clock clock;
    float appLastTime;
    bool vkInitialized = false;
    bool wdwInitialized = false;
    bool framebufferResized = false;

};

/*
Introduce enough genericity to have something like that ?
EngineCore
 ── WindowSystem x
 ── RendererSystem
 ── SceneSystem x
 ── ResourceSystem x
 ── InputSystem sort of

 Notes: Though this count as a way later addition, you should read about custom allocator and better resource system
 //Same as implementing a Hash_map simpler than unordered_map

 //Check Layers systems
*/