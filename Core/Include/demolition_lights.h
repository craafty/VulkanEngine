#pragma once

class BaseLight
{
public:
    Vector3f Color = Vector3f(1.0f, 1.0f, 1.0f);
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
    Vector3f WorldDirection = Vector3f(0.0f, 0.0f, 0.0f);
    Vector3f Up = Vector3f(0.0f, 1.0f, 0.0f);
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
    Vector3f WorldPosition = Vector3f(0.0f, 0.0f, 0.0f);
    LightAttenuation Attenuation;
};


class SpotLight : public PointLight
{
public:
    Vector3f WorldDirection = Vector3f(0.0f, 0.0f, 0.0f);
    Vector3f Up = Vector3f(0.0f, 1.0f, 0.0f);
    float Cutoff = 0.0f;
};


struct PBRLight {
    Vector4f PosDir;   // if w == 1 position, else direction
    Vector3f Intensity;
};