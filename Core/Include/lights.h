#pragma once

#include <glm/glm.hpp>

class BaseLight
{
public:
    glm::vec3 Color = glm::vec3(1.0f, 1.0f, 1.0f);
    float AmbientIntensity = 0.0f;
    float DiffuseIntensity = 0.0f;

    bool IsZero() const
    {
        return ((AmbientIntensity == 0) && (DiffuseIntensity == 0.0f));
    }
};


class DirectionalLight : public BaseLight
{
public:
    glm::vec3 WorldDirection = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
};


struct LightAttenuation
{
    float Constant = 0.1f;
    float Linear = 0.0f;
    float Exp = 0.0f;
};


class PointLight : public BaseLight
{
public:
    glm::vec3 WorldPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    LightAttenuation Attenuation;
};


class SpotLight : public PointLight
{
public:
    glm::vec3 WorldDirection = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 Up = glm::vec3(0.0f, 1.0f, 0.0f);
    float Cutoff = 0.0f;
};


struct PBRLight {
    glm::vec4 PosDir; // if w == 1 position, else direction
    glm::vec3 Intensity;
};