#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <array>
#include <optional>
#include <functional>

// All property Bool Cast shadow, receive shadows
struct alignas(16) DirectionalLight
{
    // Could be vec3
    glm::vec4 direction;
    glm::vec4 color;
    float intensity;
};

struct alignas(16) PointLight
{
    glm::vec4 position;
    glm::vec4 color;
    float intensity;
    float radius;
};

struct alignas(16) AreaLight
{
    glm::vec4 position;
    glm::vec4 color;
    float intensity;
    float radius;
};

struct alignas(16) Cascade
{
    glm::mat4 lightVP;
    float splitDepth;
};

struct DirectionalShadowData
{
    uint32_t cascadeCount = 0;
    std::array<Cascade, LightSystem::MAX_CASCADES> cascades;
};

struct DLightBehavior
{
    std::function<void(DirectionalLight &)> update;
};

struct DirectionalLightInstance
{
    DirectionalLight light;
    std::optional<DirectionalShadowData> shadow;
    std::optional<DLightBehavior> behavior;
};

struct PLightBehavior
{
    std::function<void(PLightBehavior &)> update;
};

struct PointLightInstance
{
    PointLight light;
    std::optional<PLightBehavior> behavior;
};

//Todo: With AreaLIght, we might need a "common" Light Interfaces
class LightSystem
{
public:
    static constexpr uint32_t MAX_DIR_LIGHT = 10;
    static constexpr uint32_t MAX_CASCADES = 3;
    static constexpr uint32_t TEXTURE_SIZE = 1024;

    LightSystem() = default;
    ~LightSystem() = default;

    [[noexcept]] uint32_t addDirLight(const DirectionalLight &light);
    void addDirLightBehavior(uint32_t dirLightIndex, DLightBehavior behavior);
    [[noexcept]] uint32_t addPointLight(const PointLight &light);
    void addPoinLighthBehavior(uint32_t ptLightIndex, PLightBehavior behavior);
    void enableShadow(uint32_t dirLightIndex, uint32_t cascadeCount = MAX_CASCADES);

    const std::vector<DirectionalLightInstance> &getDirLights() const;
    const std::vector<PointLightInstance> &getPointLights() const;
    uint32_t dirLightCount() const;
    uint32_t pointLightCount() const;

    void update(float dt);

private:
    std::vector<DirectionalLightInstance> directionalLights;
    std::vector<PointLightInstance> pointLights;
    /*
       // int lightSet;
    // int lightUBO;

      int shadowSet; // Same for each frame
    int shadowUBO; // Directly mapped in frameHandler
    int shadowTexture;
*/
};

class Camera;
void updateCascadeShadows(LightSystem &lights, const Camera &camera);

class OmniShadow
{
public:
    static constexpr uint32_t MAX_POINT_SHADOWS = 16;

private:
    // Texture depthCubes;
    glm::mat4 lightVP[MAX_POINT_SHADOWS * 6];
};