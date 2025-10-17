#include "camera.h"
#include <algorithm>
#include <stdio.h>

Camera::Camera() {
    m_position = glm::vec3(0.0f, 0.0f, 5.0f); // Start back a bit
    m_velocity = glm::vec3(0.0f);
    m_currentMousePos = glm::vec2(0.0f);
    m_oldMousePos = glm::vec2(0.0f);
    m_heading = 0.0f;
    m_pitch = 0.0f;
}

void Camera::Init(const glm::vec3& pos, const PersProjInfo& projInfo) {
    m_position = pos;
    m_projInfo = projInfo;
    updateProjectionMatrix();
}

void Camera::Update(float dt) {
    glm::vec2 deltaMouse = m_currentMousePos - m_oldMousePos;
    m_oldMousePos = m_currentMousePos;

    updateOrientation(deltaMouse);
    updateVelocity(dt);

    // To move correctly, we calculate the final orientation and apply velocity
    glm::quat orientation = glm::angleAxis(glm::radians(m_heading), glm::vec3(0.0f, 1.0f, 0.0f));
    orientation = glm::normalize(orientation * glm::angleAxis(glm::radians(m_pitch), glm::vec3(1.0f, 0.0f, 0.0f)));

    glm::vec3 forward = orientation * glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 right = orientation * glm::vec3(1.0f, 0.0f, 0.0f);

    glm::vec3 acceleration = glm::vec3(0.0f);
    if (m_movement.Forward)    acceleration += forward;
    if (m_movement.Backward)   acceleration -= forward;
    if (m_movement.StrafeLeft)  acceleration -= right;
    if (m_movement.StrafeRight) acceleration += right;

    // Using world-axis up/down as requested, corrected for Y-down
    if (m_movement.Up)         acceleration -= glm::vec3(0.0f, 1.0f, 0.0f);
    if (m_movement.Down)       acceleration += glm::vec3(0.0f, 1.0f, 0.0f);

    if (glm::length(acceleration) > 0.0f) {
        m_velocity += glm::normalize(acceleration) * m_acceleration * dt;
    }

    m_position += m_velocity * dt;
}

void Camera::SetPos(const glm::vec3& pos)
{
    m_position = pos;
}

void Camera::SetTarget(const glm::vec3& target)
{
    glm::vec3 forward = glm::normalize(target - m_position);
    m_pitch = glm::degrees(asin(forward.y));
    m_heading = glm::degrees(atan2(-forward.x, -forward.z));
}

void Camera::SetUp(const glm::vec3& up)
{
}


void Camera::SetMousePos(float x, float y) {
    m_currentMousePos = { x, y };
}

void Camera::updateProjectionMatrix() {
    float aspectRatio = (float)m_projInfo.Width / (float)m_projInfo.Height;
    m_projection = glm::perspective(glm::radians(m_projInfo.FOV), aspectRatio, m_projInfo.zNear, m_projInfo.zFar);
}

void Camera::updateOrientation(const glm::vec2& deltaMouse) {
    // Inspired by the example: update heading and pitch angles
    m_heading -= deltaMouse.x * m_mouseSpeed;
    m_pitch += deltaMouse.y * m_mouseSpeed;

    // Clamp the pitch to prevent flipping upside down
    if (m_pitch > 89.0f) {
        m_pitch = 89.0f;
    }
    if (m_pitch < -89.0f) {
        m_pitch = -89.0f;
    }

    // Keep heading in the 0-360 range
    if (m_heading > 360.0f) {
        m_heading -= 360.0f;
    }
    if (m_heading < 0.0f) {
        m_heading += 360.0f;
    }
}

void Camera::updateVelocity(float dt) {
    // Damping
    if (glm::length(m_velocity) > 0.001f) {
        m_velocity -= m_velocity * std::min(dt * m_damping, 1.0f);
    }

    float maxSpeed = m_movement.FastSpeed ? m_maxSpeed * m_fastCoef : m_maxSpeed;
    if (glm::length(m_velocity) > maxSpeed) {
        m_velocity = glm::normalize(m_velocity) * maxSpeed;
    }
}

glm::mat4 Camera::GetViewMatrix() const {
    glm::quat orientation = glm::angleAxis(glm::radians(m_heading), glm::vec3(0.0f, 1.0f, 0.0f));
    orientation = glm::normalize(orientation * glm::angleAxis(glm::radians(m_pitch), glm::vec3(1.0f, 0.0f, 0.0f)));

    glm::mat4 view = glm::translate(glm::mat4_cast(glm::conjugate(orientation)), -m_position);
    return view;
}

glm::mat4 Camera::GetVPMatrix() const {
    return m_projection * GetViewMatrix();
}