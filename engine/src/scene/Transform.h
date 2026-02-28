#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>  
#include <glm/gtx/quaternion.hpp>  
#include <vector>


class Transform
{
public:
    Transform() = default;
    Transform(const glm::vec3& position);
    Transform(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale);

    void setParent(Transform* newParent, bool keepWorldTransform = true);
    Transform* getParent() const;

    void setPosition(const glm::vec3& p);
    void translate(const glm::vec3& delta);

    void setRotation(const glm::quat& r);
    void rotate(const glm::quat& delta);

    void setScale(const glm::vec3& s);

    const glm::vec3& getPosition() const;
    const glm::quat& getRotation() const;
    const glm::vec3& getScale() const;

    const glm::mat4& getLocalMatrix();
    const glm::mat4& getWorldMatrix();

private:
    void markWrlDirty();
    void updateLocalMatrix();
    void updateWorldMatrix();

private:
    glm::vec3 position = {0,0,0};
    glm::quat rotation = {1,0,0,0};
    glm::vec3 scale = {1,1,1};;

    glm::mat4 localMatrix;
    glm::mat4 worldMatrix;

    bool localDirty = true;
    bool worldDirty = true;

    Transform* parent =  nullptr;
    std::vector<Transform*> children;
};


//Notes:
//Very basic implementation$
//Todo: Ideally i would rather have an external hierarchy system and no multiples children
//indexparent/indexchild/+ pointer to centralized children 
