#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

class Camera {
public:
    Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch, float fov, float aspect, float nearPlane, float farPlane)
        : position(position), up(up), yaw(yaw), pitch(pitch), fov(fov), aspect(aspect), nearPlane(nearPlane), farPlane(farPlane) {
        updateCameraVectors();
        projection = glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
    }

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, position + front, up);
    }

    glm::mat4 getProjectionMatrix() const {
        return projection;
    }

    void processKeyboardMovement(float deltaTime, bool moveForward, bool moveBackward, bool moveLeft, bool moveRight) {
        float velocity = movementSpeed * deltaTime;
        if (moveForward) position += front * velocity;
        if (moveBackward) position -= front * velocity;
        if (moveLeft) position -= right * velocity;
        if (moveRight) position += right * velocity;
    }

    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        yaw += xoffset;
        pitch += yoffset;

        if (constrainPitch) {
            if (pitch > 89.0f) pitch = 89.0f;
            if (pitch < -89.0f) pitch = -89.0f;
        }

        updateCameraVectors();
    }

    void processMouseScroll(float yoffset) {
        fov -= (float)yoffset;
        if (fov < 1.0f) fov = 1.0f;
        if (fov > 45.0f) fov = 45.0f;
        projection = glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
    }

private:
    glm::vec3 position;
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;
    
    float yaw;
    float pitch;
    float fov;
    float aspect;
    float nearPlane;
    float farPlane;

    glm::mat4 projection;

    float movementSpeed = 2.5f;
    float mouseSensitivity = 0.1f;

    void updateCameraVectors() {
        glm::vec3 frontTemp;
        frontTemp.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        frontTemp.y = sin(glm::radians(pitch));
        frontTemp.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(frontTemp);
        
        right = glm::normalize(glm::cross(front, worldUp));  // Right vector
        up = glm::normalize(glm::cross(right, front));  // Up vector
    }
};

