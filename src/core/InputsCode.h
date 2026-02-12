#pragma once

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <cstdint>

namespace cico
{

    namespace InputCode
    {
        enum MouseButton : uint16_t
        {
            BUTTON_LEFT,
            BUTTON_RIGHT,
            BUTTON_MIDDLE,
            BUTTON_4,
            COUNT
        };

        enum KeyCode : uint16_t
        {

            BSpace = 8,
            HTab = 9,
            Space = 32,
            Apostrophe = 39, /* ' */
            Comma = 44,      /* , */
            Minus = 45,      /* - */
            Period = 46,     /* . */
            Slash = 47,      /* / */

            NUM0 = 48,
            NUM1 = 49,
            NUM2 = 50,
            NUM3 = 51,
            NUM4 = 52,
            NUM5 = 53,
            NUM6 = 54,
            NUM7 = 55,
            NUM8 = 56,
            NUM9 = 57,

            Semicolon = 59, /* ; */
            Equal = 61,     /* = */

            A = 65,
            B = 66,
            C = 67,
            D = 68,
            E = 69,
            F = 70,
            G = 71,
            H = 72,
            I = 73,
            J = 74,
            K = 75,
            L = 76,
            M = 77,
            N = 78,
            O = 79,
            P = 80,
            Q = 81,
            R = 82,
            S = 83,
            T = 84,
            U = 85,
            V = 86,
            W = 87,
            X = 88,
            Y = 89,
            Z = 90,

            LeftBracket = 91,  /* [ */
            Backslash = 92,    /* \ */
            RightBracket = 93, /* ] */
            GraveAccent = 96,  /* ` */

            /* Function keys from GLFW */
            Escape = 256,
            Enter = 257,
            Tab = 258,
            Backspace = 259,
            Insert = 260,
            Delete = 261,
            Right = 262,
            Left = 263,
            Down = 264,
            Up = 265,
            PageUp = 266,
            PageDown = 267,
            Home = 268,
            End = 269,
            CapsLock = 280,
            ScrollLock = 281,
            NumLock = 282,
            PrintScreen = 283,
            Pause = 284,
            F1 = 290,
            F2 = 291,
            F3 = 292,
            F4 = 293,
            F5 = 294,
            F6 = 295,
            F7 = 296,
            F8 = 297,
            F9 = 298,
            F10 = 299,
            F11 = 300,
            F12 = 301,
            F13 = 302,
            F14 = 303,
            F15 = 304,
            F16 = 305,
            F17 = 306,
            F18 = 307,
            F19 = 308,
            F20 = 309,
            F21 = 310,
            F22 = 311,
            F23 = 312,
            F24 = 313,
            F25 = 314,

            NPAD0 = 320,
            NPAD1 = 321,
            NPAD2 = 322,
            NPAD3 = 323,
            NPAD4 = 324,
            NPAD5 = 325,
            NPAD6 = 326,
            NPAD7 = 327,
            NPAD8 = 328,
            NPAD9 = 329,
            NPADDecimal = 330,
            NPADDivide = 331,
            NPADMultiply = 332,
            NPADSubtract = 333,
            NPADAdd = 334,
            NPADEnter = 335,
            NPADEqual = 336,

            LShift = 340,
            LControl = 341,
            LAlt = 342,
            LeftSuper = 343,
            RShift = 344,
            RControl = 345,
            RAlt = 346,
            RSuper = 347,
            Menu = 348,
        };

        // Joystick buttons
        enum class GamepadButton : int
        {
            A = 0,
            B = 1,
            X = 2,
            Y = 3,
            LB = 4,
            RB = 5,
            LT = 6,
            RT = 7,
            START = 8,
            BACK = 9,
            L3 = 10,
            R3 = 11,
        };

        // Joystick axes
        enum class GamepadAxis : int
        {
            LEFT_X = 0,  // Left stick X axis
            LEFT_Y = 1,  // Left stick Y axis
            RIGHT_X = 2, // Right stick X axis
            RIGHT_Y = 3, // Right stick Y axis
            LT = 4,      // Left trigger axis
            RT = 5,      // Right trigger axis
            /*
       ctrl+f this to check other joystick related stufdf
#define GLFW_HAT_CENTERED           0

            */
        };
    }
}

#define KEYCODE_TO_STRING_CASE(key)     \
    case cico::InputCode::KeyCode::key: \
        return #key;                    \
        break;

namespace cico::Input
{
    const char *keyToString(::cico::InputCode::KeyCode key)
    {
        switch (key)
        {
            KEYCODE_TO_STRING_CASE(A)
            KEYCODE_TO_STRING_CASE(B)
            KEYCODE_TO_STRING_CASE(C)
            KEYCODE_TO_STRING_CASE(D)
            KEYCODE_TO_STRING_CASE(E)
            KEYCODE_TO_STRING_CASE(F)
            KEYCODE_TO_STRING_CASE(G)
            KEYCODE_TO_STRING_CASE(H)
            KEYCODE_TO_STRING_CASE(I)
            KEYCODE_TO_STRING_CASE(J)
            KEYCODE_TO_STRING_CASE(K)
            KEYCODE_TO_STRING_CASE(L)
            KEYCODE_TO_STRING_CASE(M)
            KEYCODE_TO_STRING_CASE(N)
            KEYCODE_TO_STRING_CASE(O)
            KEYCODE_TO_STRING_CASE(P)
            KEYCODE_TO_STRING_CASE(Q)
            KEYCODE_TO_STRING_CASE(R)
            KEYCODE_TO_STRING_CASE(S)
            KEYCODE_TO_STRING_CASE(T)
            KEYCODE_TO_STRING_CASE(NUM0)
            KEYCODE_TO_STRING_CASE(NUM1)
            KEYCODE_TO_STRING_CASE(NUM2)
            KEYCODE_TO_STRING_CASE(F1)
            KEYCODE_TO_STRING_CASE(LShift)
        default:
            return "Unknow Key";
            break;
        }
    }
}