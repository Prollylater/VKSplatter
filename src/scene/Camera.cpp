#include "Camera.h"

// Todo: Remove
#include <iostream>

Camera::Camera()
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

Camera::Camera(glm::vec3 position, glm::vec3 worldUp, float yaw, float pitch, float fov, float aspect, float nearPlane, float farPlane)
    : mPosition(position), mWorldUp(worldUp), yaw(yaw), pitch(pitch), fov(fov), aspect(aspect), nearPlane(nearPlane), farPlane(farPlane)
{
    updateCameraVectors();
    projection = glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
}

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(mPosition, mPosition + mFront, mUp);
}

glm::mat4 Camera::getProjectionMatrix() const
{
    return projection;
}

glm::vec4 Camera::getEye() const
{
    return glm::vec4(mPosition, 0.0);
}



void Camera::processKeyboardMovement(float deltaTime, bool forward, bool backward, bool left, bool right, bool up, bool down)
{
    // std::cout << "Entering in" << mPosition.x << " " << mPosition.y << " " << mPosition.z << std::endl;
    float velocity = movementSpeed * deltaTime;
    if (forward)
    {
        mPosition += mFront * velocity;
    }
    if (backward)
    {
        mPosition -= mFront * velocity;
    }
    if (left)
    {
        mPosition -= mRight * velocity;
    }
    if (right)
    {
        mPosition += mRight * velocity;
    }
    if (up)
    {
        mPosition -= glm::cross(mFront, mRight) * velocity;
    }
    if (down)
    {
        mPosition += glm::cross(mFront, mRight) * velocity;
    }

    // std::cout << "Exiting" << mPosition.x << " " << mPosition.y << " " << mPosition.z << std::endl;
}

void Camera::processMouseMovement(float xoffset, float yoffset)
{
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    updateCameraVectors();
}

void Camera::processMouseScroll(float yoffset)
{
    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 45.0f)
        fov = 45.0f;
    projection = glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
}

void Camera::setMvmtSpd(float newSpeed)
{
    movementSpeed = newSpeed;
}

void Camera::setMouseSensitivity(float newSensitivity)
{
    mouseSensitivity = newSensitivity;
}

void Camera::updateCameraVectors()
{
    glm::vec3 frontTemp;
    frontTemp.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    frontTemp.y = sin(glm::radians(pitch));
    frontTemp.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    mFront = glm::normalize(frontTemp);

    mRight = glm::normalize(glm::cross(mFront, mWorldUp));
    mUp = glm::normalize(glm::cross(mRight, mFront));
}
