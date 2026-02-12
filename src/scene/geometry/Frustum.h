#pragma once
#include <glm/glm.hpp>
#include "geometry/Extents.h"

struct Plane
{
    // unit vector
    // ax + by + cz + d
    glm::vec3 normal = {0.f, 1.f, 0.f};
    float dist = 0.f;
};

inline void normalizePlane(Plane &plane)
{
    plane.dist /= glm::length(plane.normal);
    plane.normal = glm::normalize(plane.normal);
}

inline float distanceToPoint(const Plane &plane, const glm::vec3 &pt)
{
    return glm::dot(plane.normal, pt) + plane.dist;
}

/*
1. If dist < 0, then the point p lies in the negative halfspace.
2. If dist = 0, then the point p lies in the plane.
3. If dist > 0, then the point p lies in the positive halfspace
*/



inline bool testAgainstFrustum(const glm::mat4 &VP, const Extents &aabb)
{
    const glm::vec4 corners[8] = {
        {aabb.min.x, aabb.min.y, aabb.min.z, 1.f},
        {aabb.max.x, aabb.min.y, aabb.min.z, 1.f},
        {aabb.min.x, aabb.max.y, aabb.min.z, 1.f},
        {aabb.max.x, aabb.max.y, aabb.min.z, 1.f},
        {aabb.min.x, aabb.min.y, aabb.max.z, 1.f},
        {aabb.max.x, aabb.min.y, aabb.max.z, 1.f},
        {aabb.min.x, aabb.max.y, aabb.max.z, 1.f},
        {aabb.max.x, aabb.max.y, aabb.max.z, 1.f},
    };

    for (int plane = 0; plane < 6; ++plane)
    {
        int outside = 0;

        for (int i = 0; i < 8; ++i)
        {
            glm::vec4 p = VP * corners[i];

            switch (plane)
            {
                case 0: outside += (p.x < -p.w); break; // left
                case 1: outside += (p.x >=  p.w); break; // right
                case 2: outside += (p.y < -p.w); break; // bottom
                case 3: outside += (p.y >=  p.w); break; // top
                case 4: outside += (p.z < 0); break; // near
                case 5: outside += (p.z >=  p.w); break; // far
            }
        }
        
        if (outside == 8){
            return false;}
    }

    return true;
}


struct Frustum
{
    Plane topFace;
    Plane bottomFace;

    Plane rightFace;
    Plane leftFace;

    Plane farFace;
    Plane nearFace;
};

inline Frustum extractFrustumVk(const glm::mat4 &m)
{
    Frustum frustum;

    glm::vec4 c0 = m[0];
    glm::vec4 c1 = m[1];
    glm::vec4 c2 = m[2];
    glm::vec4 c3 = m[3];

    // Left   (x >= -w)
    glm::vec4 p = c3 + c0;
    frustum.leftFace.normal = glm::vec3(p);
    frustum.leftFace.dist = p.w;

    // Right  (x <= +w)
    p = c3 - c0;
    frustum.rightFace.normal = glm::vec3(p);
    frustum.rightFace.dist = p.w;

    // Bottom (y >= -w)
    p = c3 + c1;
    frustum.bottomFace.normal = glm::vec3(p);
    frustum.bottomFace.dist = p.w;

    // Top    (y <= +w)
    p = c3 - c1;
    frustum.topFace.normal = glm::vec3(p);
    frustum.topFace.dist = p.w;

    // Near   (Vulkan: z >= 0)
    p = c2;
    frustum.nearFace.normal = glm::vec3(p);
    frustum.nearFace.dist = p.w;

    // Far    (z <= w)
    p = c3 - c2;
    frustum.farFace.normal = glm::vec3(p);
    frustum.farFace.dist = p.w;

    normalizePlane(frustum.leftFace);
    normalizePlane(frustum.rightFace);
    normalizePlane(frustum.topFace);
    normalizePlane(frustum.bottomFace);
    normalizePlane(frustum.nearFace);
    normalizePlane(frustum.farFace);

    return frustum;
}


//From LearnOpenGL
/*
std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view)
{
    const auto inv = glm::inverse(proj * view);
    
    std::vector<glm::vec4> frustumCorners;
    for (unsigned int x = 0; x < 2; ++x)
    {
        for (unsigned int y = 0; y < 2; ++y)
        {
            for (unsigned int z = 0; z < 2; ++z)
            {
                const glm::vec4 pt = 
                    inv * glm::vec4(
                        2.0f * x - 1.0f,
                        2.0f * y - 1.0f,
                        2.0f * z - 1.0f,
                        1.0f);
                frustumCorners.push_back(pt / pt.w);
            }
        }
    }
    
    return frustumCorners;
}

*/