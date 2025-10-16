#pragma once

#include <string>
#include <glm/glm.hpp>
#include "vulkan_texture.h"

struct PBRMaterial
{
    float Roughness = 0.0f;
    bool IsMetal = false;
    glm::vec3 Color = glm::vec3(0.0f, 0.0f, 0.0f);
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

    glm::vec4 AmbientColor = glm::vec4(0.0f);
    glm::vec4 DiffuseColor = glm::vec4(0.0f);
    glm::vec4 SpecularColor = glm::vec4(0.0f);

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