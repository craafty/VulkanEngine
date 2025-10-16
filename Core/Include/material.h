#pragma once

#include "vulkan_texture.h"

struct PBRMaterial
{
    float Roughness = 0.0f;
    bool IsMetal = false;
    Vector3f Color = Vector3f(0.0f, 0.0f, 0.0f);
    Texture* pAlbedo = NULL;
    Texture* pRoughness = NULL;
    Texture* pMetallic = NULL;
    Texture* pNormalMap = NULL;
    Texture* pAO = NULL;
    Texture* pEmissive = NULL;
};


class Material {

public:

    std::string m_name;

    Vector4f AmbientColor = Vector4f(0.0f, 0.0f, 0.0f, 0.0f);
    Vector4f DiffuseColor = Vector4f(0.0f, 0.0f, 0.0f, 0.0f);
    Vector4f SpecularColor = Vector4f(0.0f, 0.0f, 0.0f, 0.0f);

    PBRMaterial PBRmaterial;

    Texture* pDiffuse = NULL; // base color of the material
    Texture* pNormal = NULL;
    Texture* pSpecularExponent = NULL;

    float m_transparencyFactor = 1.0f;
    float m_alphaTest = 0.0f;

    ~Material()
    {
        if (pDiffuse) {
            delete pDiffuse;
        }

        if (pSpecularExponent) {
            delete pSpecularExponent;
        }
    }
};