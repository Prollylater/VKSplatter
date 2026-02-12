#include "Light.h"

LightSystem::LightSystem()
    : directionalCount(0), pointCount(0) {}

LightSystem::~LightSystem() {}

void LightSystem::addDirLight(const DirectionalLight &light)
{
    directionalLights.push_back(light);
    shadowMatches.push_back({false, 0});//No

    directionalCount = static_cast<unsigned int>(directionalLights.size());
}

void LightSystem::setMatchingShadow(uint32_t light, bool castShadow, uint32_t shadowId)
{
    shadowMatches[light].castShadow = castShadow;
    shadowMatches[light].shadowIndex = shadowId;
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

void CascadedShadow::updateCascadeShadows(int shadowID, const Camera &cam, glm::vec3 lightDir)
{
    auto &shadow = shadows[shadowID];
    float cascadeSplitLambda = 0.95f;

    glm::mat4 camView = cam.getViewMatrix();
    glm::mat4 camProj = cam.getProjectionMatrix();

    float near = cam.getNearPlane();
    float far = cam.getFarPlane();
    float clipRange = far - near;

    float minZ = near;
    float maxZ = near + clipRange;

    float range = maxZ - minZ;
    float ratio = maxZ / minZ;

    float cascadeSplitstmp[MAX_CASCADES];
    // Calculate relevant split based on camera frustum
    for (uint32_t i = 0; i < MAX_CASCADES; i++)
    {
        float p = (i + 1) / (float)(MAX_CASCADES);
        float log = (float)(minZ * std::pow(ratio, p));
        float uniform = minZ + range * p;
        float d = cascadeSplitLambda * (log - uniform) + uniform;
        cascadeSplitstmp[i] = (d - near) / clipRange;
    }

    // Calculate orthographic projection matrix for each cascade

    // Inverse matrix ndc -> world space
    const glm::mat4 invCamera = glm::inverse(camProj * camView);
    float lastSplitDist = 0.0;
    for (uint32_t i = 0; i < MAX_CASCADES; i++)
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

        glm::mat4 lightViewMatrix = glm::lookAt(center - lightDir * -minExtents.z, center, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

        // Store split distance and matrix in cascade
        shadow.cascades[i].splitDepth = (near + splitDist * clipRange) * -1.0f;
        shadow.cascades[i].lightVP = lightOrthoMatrix * lightViewMatrix;

        lastSplitDist = shadow.cascades[i].splitDepth;
    }
}

uint32_t CascadedShadow::createDirectionalShadow(size_t cascadeCount)
{
    // shadows.push_back(DirectionalShadow{std::vector<Cascade>(cascadeCount)});
    shadows.push_back(DirectionalShadow{});
    return static_cast<uint32_t>(shadows.size() - 1);
}
