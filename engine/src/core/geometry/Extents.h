#pragma once
#include <glm/glm.hpp>

// Todo:
// I need to reconsider properly this and other functions need to have member functions
// Main reason for the current state was to not "litter" namespace...

struct Extents
{
    glm::vec3 min;
    glm::vec3 max;

    Extents() : min(glm::vec3(std::numeric_limits<float>::max())),
                max(glm::vec3(std::numeric_limits<float>::lowest()))
    {
    }

    void translate(const glm::vec3 &offset)
    {
        min += offset;
        max += offset;
    }

    glm::vec3 center() const
    {
        return (min + max) * 0.5f;
    }

    glm::vec3 extent() const
    {
        return max - min;
    }

    float radius() const
    {
        return glm::length(extent()) * 0.5;
    }

    void expand(const glm::vec3 &p)
    {
        min = glm::min(min, p);
        max = glm::max(max, p);
    }
    void expand(const Extents &other)
    {
        std::cout << "Expand from" << min[0] << " " << min[1] << " " << min[2] << " " << max[0] << " " << min[1] << " " << max[2] << " to ";
        min = glm::min(min, other.min);
        max = glm::max(max, other.max);
        std::cout << other.min[0] << " " << other.min[1] << " " << other.min[2] << " " << other.max[0] << " " << other.max[1] << " " << other.max[2] << std::endl;
    }

    bool intersects(const Extents &other) const
    {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }

    bool contains(const glm::vec3 &p) const
    {
        return (p.x >= min.x && p.x <= max.x) &&
               (p.y >= min.y && p.y <= max.y) &&
               (p.z >= min.z && p.z <= max.z);
    }
};
