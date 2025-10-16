#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct CameraMovement {
    bool Forward = false;
    bool Backward = false;
    bool StrafeLeft = false;
    bool StrafeRight = false;
    bool Up = false;
    bool Down = false;
    bool FastSpeed = false;
};

struct PersProjInfo {
    float FOV;
    unsigned int Width;
    unsigned int Height;
    float zNear;
    float zFar;
};

class Camera {
public:
    Camera();

    void Init(const glm::vec3& pos, const PersProjInfo& projInfo);
    void Update(float dt);
    void SetMousePos(float x, float y);

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetVPMatrix() const;

    CameraMovement m_movement;
    float m_acceleration = 50.0f;
    float m_damping = 5.0f;
    float m_maxSpeed = 10.0f;
    float m_fastCoef = 3.0f;
    float m_mouseSpeed = 0.1f;

private:
    void updateOrientation(const glm::vec2& deltaMouse);
    void updateVelocity(float dt);
    void updateProjectionMatrix();

    PersProjInfo m_projInfo;
    glm::mat4 m_projection;
    glm::vec3 m_position;
    glm::vec3 m_velocity;

    // Using separate heading and pitch like the example for stable control
    float m_heading; // Yaw angle (around world Y-axis)
    float m_pitch;   // Pitch angle (around local X-axis)

    glm::vec2 m_currentMousePos;
    glm::vec2 m_oldMousePos;
};