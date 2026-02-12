#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <array>

// All property Bool Cast shadow, receive shadows
struct alignas(16) DirectionalLight
{
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

class LightSystem
{
public:
    static constexpr uint32_t MAX_DIR_LIGHT = 10;
    LightSystem();
    ~LightSystem();

    void addDirLight(const DirectionalLight &light);
    void setMatchingShadow(uint32_t light, bool castShadow, uint32_t shadowId);
    void addPointLight(const PointLight &light);

    const std::vector<DirectionalLight> &getDirLights() const;
    const std::vector<PointLight> &getPointLights() const;
    uint32_t dirLightCount() const;
    uint32_t pointLightCount() const;

    void updateLightData();

private:
    uint32_t directionalCount;
    uint32_t pointCount;
    std::vector<DirectionalLight> directionalLights;

    struct ShadowMatch
    {
        bool castShadow;
        int shadowIndex;
    };

    std::vector<ShadowMatch> shadowMatches;
    std::vector<PointLight> pointLights;
    // int lightSet;
    // int lightUBO;
};

#include "Camera.h"

struct alignas(16) Cascade
{
    glm::mat4 lightVP;
    float splitDepth;
};

class CascadedShadow
{

public:
    static constexpr uint32_t MAX_CASCADES = 3;
    static constexpr uint32_t TEXTURE_SIZE = 1024;

    struct DirectionalShadow
    {
        std::array<Cascade, MAX_CASCADES> cascades;
    };

    const glm::mat4 *getLightVPs() const;
    const float *getSplits() const;

    // https://johanmedestrom.wordpress.com/2016/03/18/opengl-cascaded-shadow-maps/
    // Notes: Some method to handle light shimmmering was left out
    void updateCascadeShadows(int shadowID, const Camera &cam, glm::vec3 lightDir);
    uint32_t createDirectionalShadow(size_t cascadeCount);

    uint32_t shadowsCount()
    {
        return shadows.size();
    }
    
    uint32_t requiredTextureCount()
    {
        uint32_t total = 0;
        //Different  number of cascade per shadow
        // for (auto& shadow : shadows  ){
        //
        //      }
        return shadows.size()* MAX_CASCADES;
    }

private:
    std::vector<DirectionalShadow> shadows;
};

class OmniShadow
{
public:
    static constexpr uint32_t MAX_POINT_SHADOWS = 16;

private:
    // Texture depthCubes;
    glm::mat4 lightVP[MAX_POINT_SHADOWS * 6];
};

class ShadowSystem
{
public:
    CascadedShadow &getCascadeShadows()
    {
        return cascadeShadows;
    }

private:
    CascadedShadow cascadeShadows;
    // OmniShadow omni;
    int shadowSet; // Same for each frame
    int shadowUBO; // Directly mapped in frameHandler
    int shadowTexture;
};