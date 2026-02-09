#pragma once

#include "BaseVk.h" //Config mainly
#include "Clock.h"
#include "logging/Logger.h"
#include "filesystem/Filesystem.h"
//#include "ContextController.h"
//#include "Renderer.h"
//#include "ResourceSystem.h"
//#include "MaterialSystem.h"
#include <memory>

// Forward declarations to avoid exposing all headers
class Renderer;
class Scene;
class AssetSystem;
class MaterialSystem;
class VulkanWindow; //Lot of name don't fit their file
class Event;
class VulkanContext;

class Application
{
public:
    Application();
    virtual ~Application();

    int run();

protected:
    virtual void setup() = 0;         
    virtual void update(float dt) = 0;      
    virtual void render() = 0;              
    virtual void shutdown() = 0;   
    virtual void onEvent(Event &event) = 0;

    void quit() { mRunning = false; }

    // Access to framework systems (read-only for safety)
    Scene &getScene() { return *mScene; }
    Renderer &getRenderer() { return *mRenderer; }
    AssetSystem &getAssetSystem() { return *mAssetSystem; }
    MaterialSystem &getMaterialSystem() { return *mMaterialSystem; }
    VulkanWindow &getWindow() { return *mWindow; }

    //Introduce AppConfiguration
    // Configuration (call before run() or in constructor)
    void setWindowTitle(const std::string &title) { mWindowTitle = title; }
    void setWindowSize(int width, int height)
    {
        mWindowWidth = width;
        mWindowHeight = height;
    }
    void setAssetRoot(const std::filesystem::path &root);

protected:
    // Notes: Unimplemented
    // void setCustomAssetSystem(std::unique_ptr<AssetSystem> system);
    // void setCustomRenderer(std::unique_ptr<Renderer> renderer);

private:
    std::unique_ptr<VulkanWindow> mWindow;
    std::unique_ptr<VulkanContext> mContext;
    std::unique_ptr<Renderer> mRenderer;
    std::unique_ptr<Scene> mScene;
    std::unique_ptr<AssetSystem> mAssetSystem;
    std::unique_ptr<MaterialSystem> mMaterialSystem;

    // Configuration
    std::string mWindowTitle = "Vulkan Application";
    int mWindowWidth = 800;
    int mWindowHeight = 600;
    uint16_t mFrameRateLimit = 0;
    bool mFramebufferResized = false;
    cico::Clock mClock;
    bool mRunning = false; //Notused

    // Framework initialization
    void initFramework();
    void initVulkan(ContextCreateInfo& info);
    // void initDefaultScene();
    void mainLoop();
    void cleanup();

    // Internal callbacks
    static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
};