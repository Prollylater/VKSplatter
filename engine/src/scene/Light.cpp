#include "Light.h"
#include "Camera.h"

uint32_t LightSystem::addDirLight(const DirectionalLight &light)
{
    directionalLights.push_back({light});
    return directionalLights.size() - 1;
}

uint32_t LightSystem::addPointLight(const PointLight &light)
{
    pointLights.push_back({light});
    return pointLights.size() - 1;
}

void LightSystem::enableShadow(uint32_t index, uint32_t cascadeCount)
{
    if (index >= directionalLights.size())
    {
        return;
    }

    DirectionalShadowData shadow;
    shadow.cascadeCount = std::min(cascadeCount, MAX_SHDW_CASCADES);

    directionalLights[index].shadow = shadow;
}

const std::vector<DirectionalLightInstance> &LightSystem::getDirLights() const
{
    return directionalLights;
}

const std::vector<PointLightInstance> &LightSystem::getPointLights() const
{
    return pointLights;
}

void LightSystem::addDirLightBehavior(uint32_t dirLightIndex, DLightBehavior behavior)
{
    directionalLights[dirLightIndex].behavior = behavior;
}

void LightSystem::addPoinLighthBehavior(uint32_t ptLightIndex, PLightBehavior behavior)
{
    pointLights[ptLightIndex].behavior = behavior;
}

uint32_t LightSystem::dirLightCount() const
{
    return directionalLights.size();
}

uint32_t LightSystem::pointLightCount() const
{
    return pointLights.size();
}

void LightSystem::update(float dt)
{
    for (auto &light : directionalLights)
    {
        if (light.behavior.has_value())
        {
            light.behavior->update(light.light);
        }
    }

    for (auto &light : directionalLights)
    {
        if (light.behavior.has_value())
        {
            light.behavior->update(light.light);
        }
    }
}

// https://johanmedestrom.wordpress.com/2016/03/18/opengl-cascaded-shadow-maps/
// Notes: Some method to handle light shimmmering was left out
void updateCascadeShadows(LightSystem &lights, const Camera &camera)
{
    // Todo: Const_cast here !!
    // It probably mean this might be better in Update
    // Left as a remember to reread about this particular design

    //Can skip this if light or camera haven't moved aka, introduce light camera dirty concept
    auto &dirLights = const_cast<std::vector<DirectionalLightInstance> &>(
        lights.getDirLights());

    float cascadeSplitLambda = 0.95f;

    glm::mat4 camView = camera.getViewMatrix();
    glm::mat4 camProj = camera.getProjectionMatrix();

    float near = camera.getNearPlane();
    float far = camera.getFarPlane();
    float clipRange = far - near;

    float minZ = near;
    float maxZ = near + clipRange;

    float range = maxZ - minZ;
    float ratio = maxZ / minZ;

    const glm::mat4 invCamera = glm::inverse(camProj * camView);

    for (auto &lightInstance : dirLights)
    {

        if (!lightInstance.shadow.has_value())
        {
            continue;
        }

        auto &shadow = *lightInstance.shadow;
        float cascadeSplitstmp[shadow.cascadeCount];

        // Calculate relevant split based on camera frustum
        for (uint32_t i = 0; i < shadow.cascadeCount; ++i)
        {
            float p = (i + 1) / (float)(shadow.cascadeCount);
            float log = (float)(minZ * std::pow(ratio, p));
            float uniform = minZ + range * p;
            float d = cascadeSplitLambda * (log - uniform) + uniform;
            cascadeSplitstmp[i] = (d - near) / clipRange;
        }

        // Calculate orthographic projection matrix for each cascade
        // Starting with inverse matrix ndc -> world space

        float lastSplitDist = 0.0;
        for (uint32_t i = 0; i < shadow.cascadeCount; i++)
        {
            float splitDist = cascadeSplitstmp[i];
            //-minus is left bottom, near in x y z
            glm::vec3 frustumCorners[8] = {
                glm::vec3(-1.0f, 1.0f, 0.0f),
                glm::vec3(1.0f, 1.0f, 0.0f),
                glm::vec3(1.0f, -1.0f, 0.0f),
                glm::vec3(-1.0f, -1.0f, 0.0f),
                glm::vec3(-1.0f, 1.0f, 1.0f),
                glm::vec3(1.0f, 1.0f, 1.0f),
                glm::vec3(1.0f, -1.0f, 1.0f),
                glm::vec3(-1.0f, -1.0f, 1.0f),
            };

            // Frustum to world space
            for (uint32_t j = 0; j < 8; j++)
            {
                glm::vec4 invCorner = invCamera * glm::vec4(frustumCorners[j], 1.0f);
                frustumCorners[j] = invCorner / invCorner.w;
            }

            // Notes: Are some operation reusable ?
            // Apply the split effect on "opposing vertices" to have correct distances
            for (uint32_t j = 0; j < 4; j++)
            {
                glm::vec3 cornerRay = frustumCorners[j + 4] - frustumCorners[j];

                frustumCorners[j + 4] = frustumCorners[j] + (cornerRay * splitDist);
                frustumCorners[j] = frustumCorners[j] + (cornerRay * lastSplitDist);
            }

            glm::vec3 center = glm::vec3(0.0f);
            for (uint32_t j = 0; j < 8; j++)
            {
                center += frustumCorners[j];
            }
            center /= 8.0f;

            // Radius of the split which can be used for the split AABB
            float radius = 0.0f;
            for (uint32_t j = 0; j < 8; j++)
            {
                float distance = glm::length(frustumCorners[j] - center);
                radius = glm::max(radius, distance);
            }
            // Round and quantize
            radius = std::ceil(radius * 16.0f) / 16.0f;

            // AABB
            glm::vec3 maxExtents = glm::vec3(radius);
            glm::vec3 minExtents = -maxExtents;
            auto lightDir = glm::vec3(lightInstance.light.direction.x, lightInstance.light.direction.y, lightInstance.light.direction.z);
            glm::mat4 lightViewMatrix = glm::lookAt(center - lightDir * -minExtents.z, center, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

            // Store split distance and matrix in cascade
            shadow.cascades[i].splitDepth = (near + splitDist * clipRange) * -1.0f;
            shadow.cascades[i].lightVP = lightOrthoMatrix * lightViewMatrix;

            lastSplitDist = shadow.cascades[i].splitDepth;
        }
    }
}