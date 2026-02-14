#include "Application.h"
/*
As you'll see, the general pattern that object creation function parameters in Vulkan follow is:

Pointer to struct with creation info
Pointer to custom allocator callbacks, always nullptr in this tutorial
Pointer to the variable that stores the handle to the new object
*/

/*
Todo: Gp gems 4/6
*/

class HelloTriangleApplication : public Application
{
public:
    HelloTriangleApplication(ContextCreateInfo & info) : Application(info){};
    void initScene();
    
    void setup() override;
    void onEvent(Event &event) override;
    void update(float dt) override;
    void render() override;
    void shutdown() override
    {
        return;
    }

private:
    bool framebufferResized = false;
};
