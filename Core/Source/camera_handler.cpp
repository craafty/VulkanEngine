#include "camera_handler.h"
#include <GLFW/glfw3.h>

bool HandleCameraKeys(Camera* pCamera, int Key, int Action)
{
    if (!pCamera) {
        return false;
    }

    bool isPressed = (Action == GLFW_PRESS) || (Action == GLFW_REPEAT);
    bool handled = true;

    switch (Key) {
    case GLFW_KEY_W:
        pCamera->m_movement.Forward = isPressed;
        break;
    case GLFW_KEY_S:
        pCamera->m_movement.Backward = isPressed;
        break;
    case GLFW_KEY_A:
        pCamera->m_movement.StrafeLeft = isPressed;
        break;
    case GLFW_KEY_D:
        pCamera->m_movement.StrafeRight = isPressed;
        break;
    case GLFW_KEY_SPACE:
        pCamera->m_movement.Up = isPressed;
        break;
    case GLFW_KEY_LEFT_CONTROL:
        pCamera->m_movement.Down = isPressed;
        break;
    case GLFW_KEY_LEFT_SHIFT:
        pCamera->m_movement.FastSpeed = isPressed;
        break;
    default:
        handled = false;
        break;
    }

    return handled;
}

void HandleCameraMouseButton(Camera* pCamera, int Button, int Action)
{
    if (!pCamera) {
        return;
    }

    if (Button == GLFW_MOUSE_BUTTON_LEFT) {
        
    }
}