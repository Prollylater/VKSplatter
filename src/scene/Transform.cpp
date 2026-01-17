#include "Transform.h"
#include <algorithm>

inline glm::mat4 composeTRS(
    const glm::vec3 &t,
    const glm::quat &r,
    const glm::vec3 &s)
{
    return glm::translate(glm::mat4(1.0f), t) * glm::toMat4(r) * glm::scale(glm::mat4(1.0f), s);
}

Transform::Transform()
    : position{}, rotation{}, scale{1, 1, 1},
      localDirty(true), worldDirty(true),
      parent(nullptr)
{
}

Transform::Transform(const glm::vec3 &p)
    : Transform()
{
    position = p;
    worldDirty = true;
    localDirty = true;
}

Transform::Transform(const glm::vec3 &p, const glm::quat &r, const glm::vec3 &s)
    : position(p), rotation(r), scale(s),
      localDirty(true), worldDirty(true),
      parent(nullptr)
{
}

void Transform::setParent(Transform *newParent, bool keepWrldTrsf)
{
    if (parent == newParent)
        return;

    glm::mat4 oldWorld;
    if (keepWrldTrsf)
        oldWorld = getWorldMatrix();

    if (parent)
    {
        auto &siblings = parent->children;
        siblings.erase(
            std::remove(siblings.begin(), siblings.end(), this),
            siblings.end());
    }

    parent = newParent;

    if (parent)
        parent->children.push_back(this);

    if (keepWrldTrsf)
    {
        if (parent)
            localMatrix = inverse(parent->getWorldMatrix()) * oldWorld;
        else
            localMatrix = oldWorld;

        localDirty = false;
    }

    markWrlDirty();
}

Transform *Transform::getParent() const
{
    return parent;
}

void Transform::setPosition(const glm::vec3 &p)
{
    position = p;
    localDirty = true;
    markWrlDirty();
}

void Transform::translate(const glm::vec3 &d)
{
    position += d;
    localDirty = true;
    markWrlDirty();
}

void Transform::setRotation(const glm::quat &r)
{
    rotation = r;
    localDirty = true;
    markWrlDirty();
}

void Transform::rotate(const glm::quat &d)
{
    rotation = d * rotation;
    localDirty = true;
    markWrlDirty();
}

void Transform::setScale(const glm::vec3 &s)
{
    scale = s;
    localDirty = true;
    markWrlDirty();
}

const glm::vec3 &Transform::getPosition() const { return position; }
const glm::quat &Transform::getRotation() const { return rotation; }
const glm::vec3 &Transform::getScale() const { return scale; }

const glm::mat4 &Transform::getLocalMatrix()
{
    if (localDirty)
        updateLocalMatrix();
    return localMatrix;
}

const glm::mat4 &Transform::getWorldMatrix()
{
    if (worldDirty)
        updateWorldMatrix();
    return worldMatrix;
}

void Transform::updateLocalMatrix()
{
    localMatrix = composeTRS(position, rotation, scale);
    localDirty = false;
}

void Transform::updateWorldMatrix()
{
    if (parent)
    {
        worldMatrix = parent->getWorldMatrix() * getLocalMatrix();
    }
    else
    {
        worldMatrix = getLocalMatrix();
    }
    worldDirty = false;
}

void Transform::markWrlDirty()
{
    if (worldDirty)
        return;

    worldDirty = true;
    for (Transform *child : children)
    {
        child->markWrlDirty();
    }
}
