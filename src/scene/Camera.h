#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

class Camera
{ // Perspective Camera
public:
    Camera();

    Camera(glm::vec3 position, glm::vec3 worldUp, float yaw, float pitch, float fov, float aspect, float nearPlane, float farPlane);

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;
    float getFov() const;
    float getAspect() const;
    glm::vec3 getFront() const;
    glm::vec4 getEye() const;
    
    float getNearPlane() const
    {
        return nearPlane;
    }

    float getFarPlane() const
    {
        return farPlane;
    }
    void processKeyboardMovement(float deltaTime, bool forward, bool backward, bool left, bool right, bool up, bool down);
    void processMouseMovement(float xoffset, float yoffset);
    void processMouseScroll(float yoffset);

    void setPosition(const glm::vec3 &pos)
    {
        mPosition = pos;
        updateCameraVectors();
    }

    float getMvmtSpd()
    {
        return movementSpeed;
    };
    void setMvmtSpd(float newSpeed);
    float getMouseSns()
    {
        return mouseSensitivity;
    };
    void setMouseSensitivity(float newSensitivity);

private:
    glm::vec3 mPosition;
    glm::vec3 mWorldUp{0.0f, 1.0f, 0.0f};

    // Camera "reference"
    glm::vec3 mFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 mUp;
    glm::vec3 mRight;

    // Notes: Consider this kind of scheme as ooposte to =0.0f
    // Probably more relevant for object themselves
    // Camera cam; vs Camera cam{};

    float fov;
    float aspect;
    float nearPlane;
    float farPlane;

    // Rotation
    float yaw;
    float pitch;
    float roll; // Not now
    glm::mat4 projection;

    float movementSpeed = 5.f;
    float mouseSensitivity = 0.5f;

    void updateCameraVectors();
};
