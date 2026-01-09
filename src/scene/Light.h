#pragma once

#include <glm/glm.hpp>
#include <vector>

//All property Bool Cast shadow, receive shadows
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

class  LightSystem
{
public:
    static constexpr uint32_t MAX_DIR_LIGHT = 10;
    LightSystem();
    ~LightSystem();

    void addDirLight(const DirectionalLight &light);
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
    std::vector<PointLight> pointLights;
};

