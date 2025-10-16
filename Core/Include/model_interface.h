#pragma once

#include "scene_object.h"
#include "material.h"

class IModel : public SceneObjectBase
{
public:

    virtual void SetColorTexture(int TextureHandle) = 0;

    virtual void SetNormalMap(int TextureHandle) = 0;

    virtual void SetHeightMap(int TextureHandle) = 0;

    virtual void SetTextureScale(float Scale) = 0;

    PBRMaterial& GetPBRMaterial() { return m_PBRmaterial; };

    void SetPBR(bool IsPBR) { m_isPBR = IsPBR; }

    bool IsPBR() const { return m_isPBR; }

protected:
    PBRMaterial m_PBRmaterial;
    bool m_isPBR = false;
};


class Grid : public IModel
{
};