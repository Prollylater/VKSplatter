#include "WindowVk.h"
#include "EventsSub.h"

VulkanWindow::~VulkanWindow()
{
    /*
     if (surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance, surface, nullptr);
    }*/
    if (mWindow)
    {
        glfwDestroyWindow(mWindow);
        glfwTerminate();
    }
}

bool VulkanWindow::init(const std::string &title, uint32_t width, uint32_t height)
{
    if (!s_glfwInitialized)
    {
        if (!glfwInit())
        {
            return false;
        }
        s_glfwInitialized = true;
    }

    // Always Vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    width = width;
    height = height;

    mWindow = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!mWindow)
    {
        glfwTerminate();
        return false;
    }

    // More opengl stuff  GLFW_NO_API_ should render it useless
    glfwMakeContextCurrent(mWindow);
    // Set up resize callback
    glfwSetWindowUserPointer(mWindow, this);

    glfwSetFramebufferSizeCallback(mWindow, VulkanWindow::framebufferResizeCallback);

    /*
    // Create Vulkan surface
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
    {
        return false;
    }*/

    // Set GLFW callbacks
    /*
    glfwSetWindowSizeCallback(mWindow, [](GLFWwindow *window, int width, int height)
                              {
        auto data = reinterpret_cast<VulkanWindow *>(glfwGetWindowUserPointer(window));
        data->width = width;
        data->height = height;
 });

    glfwSetWindowCloseCallback(mWindow, [](GLFWwindow *window)
                               {
        auto data = reinterpret_cast<VulkanWindow *>(glfwGetWindowUserPointer(window));
        WindowCloseEvent event;
        data.EventCallback(event); });*/
    // Callback

    glfwSetKeyCallback(mWindow, [](GLFWwindow *window, int key, int scancode, int action, int mods)
                       {
        auto& windwClass = *(reinterpret_cast<VulkanWindow *>(glfwGetWindowUserPointer(window)));

        switch (action)
        {
            case GLFW_PRESS:
            {
                KeyPressedEvent event(key);
                windwClass.eventCallback(event);
                break;
            }
            case GLFW_RELEASE:
            {
                KeyReleasedEvent event(key);
                windwClass.eventCallback(event);
                break;
            }
            case GLFW_REPEAT:
            {
                KeyPressedEvent event(key, true);
                windwClass.eventCallback(event);
                break;
            }
        } });

    //  glfwSetCharCallback(mWindow, [](GLFWwindow *window, unsigned int keycode)
    //                    { auto &windwClass = *(reinterpret_cast<VulkanWindow *>(glfwGetWindowUserPointer(window))); });

    glfwSetMouseButtonCallback(mWindow, [](GLFWwindow *window, int button, int action, int mods)
                               {
        auto& windwClass = *(reinterpret_cast<VulkanWindow *>(glfwGetWindowUserPointer(window)));

        switch (action)
        {
            case GLFW_PRESS:
            {
                MouseButtonPressedEvent event(button);
                windwClass.eventCallback(event);
                break;
            }
            case GLFW_RELEASE:
            {
                MouseButtonReleasedEvent event(button);
                windwClass.eventCallback(event);
                break;
            }
            case GLFW_REPEAT: //Should work as is Dragging if MouseButtonLeft
            {
            
            }
        } });

        /*
    glfwSetCursorPosCallback(mWindow, [](GLFWwindow *window, double xPos, double yPos)
                             { auto &windwClass = *(reinterpret_cast<VulkanWindow *>(glfwGetWindowUserPointer(window))); });

    glfwSetScrollCallback(mWindow, [](GLFWwindow *window, double xPos, double yPos)
                          { auto &windwClass = *(reinterpret_cast<VulkanWindow *>(glfwGetWindowUserPointer(window))); });*/

    return true;
}

void VulkanWindow::onUpdate()
{
    glfwPollEvents();
    if (glfwWindowShouldClose(mWindow))
    {
        close();
    }
}

bool VulkanWindow::isOpen() const
{
    return !glfwWindowShouldClose(mWindow);
}

void VulkanWindow::close()
{
    glfwSetWindowShouldClose(mWindow, GLFW_TRUE);
}

void VulkanWindow::setEventCallback(const EvtCallback &callback)
{
    eventCallback = callback;
}

uint32_t VulkanWindow::getWidth() const { return width; }
uint32_t VulkanWindow::getHeight() const { return height; }

void *VulkanWindow::getNativeWindowHandle() const
{
    return (void *)mWindow;
}

GLFWwindow *VulkanWindow::getGLFWWindow()
{
    return mWindow;
}
