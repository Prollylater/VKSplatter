#pragma once

#include <array>
#include <GLFW/glfw3.h>
#include "InputsCode.h"

namespace cico::InputCode
{

    struct MouseState
    {
        int16_t x;
        int16_t y;
        std::array<bool, MouseButton::COUNT> keys;
    };

    //Mainly thought as insurance, GLFW Should work for now
    struct InputState{
        std::array<bool, 256> keysPrevious;
        std::array<bool, 256> keysCurrent;
        MouseState mouseStatePrevious;
        MouseState mouseStateCurrent;
    };

    //It is also a bit more API Agnostic
    //static bool  s_inputStateInit;
    //static InputState;
    //The idea would have been to actively use the InputState
    //We copu current to previous each frame
    //When  we recieinved a key to process, we check if it is pressed 
    //through the table and update it around 
    //Then we check in the table directly for is Key this or that
    //We check in two table for held and released

    bool IsKeyPressed(GLFWwindow *window, const KeyCode key)
    {
        auto state = glfwGetKey(window, static_cast<int32_t>(key));
        return state == GLFW_PRESS;
    };
    bool IsKeyReleased(GLFWwindow *window, const KeyCode key)
    {
        auto state = glfwGetKey(window, static_cast<int32_t>(key));
        return state == GLFW_RELEASE;
    }

    bool IsKeyHeld(GLFWwindow *window, const KeyCode key)
    {
        auto state = glfwGetKey(window, static_cast<int32_t>(key));
        return state == GLFW_REPEAT;
    }

    bool IsCharPressed(GLFWwindow *window, const KeyCode key)
    {
        auto state = glfwGetKey(window, static_cast<int32_t>(key));
        return state == GLFW_PRESS;
    }

    bool IsMouseButtonPressed(GLFWwindow *window, const MouseButton button)
    {
        auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
        return state == GLFW_PRESS;
    }

    bool IsMouseButtonHeld(GLFWwindow *window, const MouseButton button)
    {
        auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
        return state == GLFW_REPEAT;
    }

    bool IsMouseButtonReleased(GLFWwindow *window, const MouseButton button)
    {
        auto state = glfwGetMouseButton(window, static_cast<int32_t>(button));
        return state == GLFW_RELEASE;
    }

    std::array<float, 2> GetMousePosition(GLFWwindow *window)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        return {static_cast<float>(xpos), static_cast<float>(ypos)};
    }

    float GetMouseX(GLFWwindow *window)
    {
        return GetMousePosition(window)[0];
    }

    float GetMouseY(GLFWwindow *window)
    {
        return GetMousePosition(window)[1];
    }

    // Todo: For Joystick   https://www.glfw.org/docs/3.3/input_guide.html
}
