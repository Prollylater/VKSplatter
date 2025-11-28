#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

class Camera
{ // Perspective Camera
public:
    Camera()
        : mPosition(0.0f, 0.0f, 1.0f),
          mWorldUp(0.0f, 1.0f, 0.0f), 
          yaw(-90.0f),               
          pitch(0.0f),
          fov(45.0f),
          aspect(16.0f / 9.0f),
          nearPlane(0.1f),
          farPlane(100.0f)
    {
        updateCameraVectors();
        projection = glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
    }

    Camera(glm::vec3 up, float yaw, float pitch, float fov, float aspect, float nearPlane, float farPlane)
        : mPosition(0.0f, 0.0f, 0.0f), mWorldUp{}, yaw{}, pitch{}, fov(45), aspect(aspect), nearPlane(nearPlane), farPlane(farPlane)
    {
        updateCameraVectors();
        projection = glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
    }

    Camera(glm::vec3 position, glm::vec3 worldUp, float yaw, float pitch, float fov, float aspect, float nearPlane, float farPlane)
        : mPosition(position), mWorldUp(worldUp), yaw(yaw), pitch(pitch), fov(fov), aspect(aspect), nearPlane(nearPlane), farPlane(farPlane)
    {
        updateCameraVectors();
        projection = glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
    }

    glm::mat4 getViewMatrix() const
    {
        return glm::lookAt(mPosition, mPosition + mFront, mUp);
    }

    glm::mat4 getProjectionMatrix() const
    {
        return projection;
    }

    void processKeyboardMovement(float deltaTime, bool forward, bool backward, bool left, bool right,bool up, bool down)
    {
        std::cout<<"Entering in" <<  mPosition.x << " " << mPosition.y << " " << mPosition.z << std::endl;
        float velocity = movementSpeed * deltaTime;
        if (forward){
            mPosition += mFront * velocity;}
        if (backward){
            mPosition -= mFront * velocity;}
        if (left){
            mPosition -= mRight * velocity;}
        if (right){
            mPosition += mRight * velocity;}
        if (up){
            mPosition += glm::cross(mFront,mRight) * velocity;}
        if (down){
            mPosition -=  glm::cross(mFront,mRight) * velocity;}
        
        std::cout<<"Exiting" << mPosition.x << " " << mPosition.y << " "  << mPosition.z << std::endl;

    }

    void processMouseMovement(float xoffset, float yoffset)
    {
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        yaw += xoffset;
        pitch += yoffset;

        updateCameraVectors();
    }

    void processMouseScroll(float yoffset)
    {
        fov -= (float)yoffset;
        if (fov < 1.0f)
            fov = 1.0f;
        if (fov > 45.0f)
            fov = 45.0f;
        projection = glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
    }

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

    float movementSpeed = 1.f;
    float mouseSensitivity = 0.1f;

    void updateCameraVectors()
    {
        glm::vec3 frontTemp;
        frontTemp.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        frontTemp.y = sin(glm::radians(pitch));
        frontTemp.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        mFront = glm::normalize(frontTemp);

        mRight = glm::normalize(glm::cross(mFront, mWorldUp));
        mUp = glm::normalize(glm::cross(mRight, mFront));
    }
};
