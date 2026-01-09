#include "Light.h"

LightSystem::LightSystem()
    : directionalCount(0), pointCount(0) {}

LightSystem::~LightSystem() {}

void LightSystem::addDirLight(const DirectionalLight &light)
{
    //Todo: Temporary
    if (directionalCount == 10)
    {
        return;
    };

    directionalLights.push_back(light);
    directionalCount = static_cast<unsigned int>(directionalLights.size());
}

void LightSystem::addPointLight(const PointLight &light)
{
    pointLights.push_back(light);
    pointCount = static_cast<unsigned int>(pointLights.size());
}

const std::vector<DirectionalLight> &LightSystem::getDirLights() const
{
    return directionalLights;
}

const std::vector<PointLight> &LightSystem::getPointLights() const
{
    return pointLights;
}

uint32_t LightSystem::dirLightCount() const
{
    return directionalLights.size();
}

uint32_t LightSystem::pointLightCount() const
{
    return pointLights.size();
}

void LightSystem::updateLightData()
{
    // Todo:
}
