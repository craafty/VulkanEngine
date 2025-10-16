#pragma once

#include "camera.h"
#include "demolition_scene.h"
#include "demolition_model.h"


class GameCallbacks
{
public:

    virtual void OnFrame(long long DeltaTimeMillis) {}

    virtual void OnFrameEnd() {}

    virtual bool OnKeyboard(int key, int action)
    {
        return false;
    }

    virtual bool OnMouseMove(int x, int y)
    {
        return false;
    }

    virtual bool OnMouseButton(int Button, int Action, int Mode, int x, int y)
    {
        return false;
    }
};


enum RENDERING_SYSTEM {
    RENDERING_SYSTEM_GL,
    RENDERING_SYSTEM_VK,
    RENDERING_SYSTEM_DX12,
    RENDERING_SYSTEM_DX11
};


class RenderingSystem
{
public:

    static RenderingSystem* CreateRenderingSystem(RENDERING_SYSTEM RenderingSystem, GameCallbacks* pGameCallbacks, bool LoadBasicShapes);

    virtual void* CreateWindow(int Width, int Height, const char* pWindowName) = 0;

    virtual void Shutdown() = 0;

    virtual void Execute() = 0;

    virtual Scene* CreateEmptyScene() = 0;

    virtual Scene* CreateScene(const std::string& Filename) = 0;

    virtual Scene* CreateDefaultScene() = 0;

    virtual void SetScene(Scene* pScene) = 0;

    virtual Scene* GetScene() = 0;

    virtual Model* LoadModel(const std::string& Filename) = 0;

    virtual Grid* CreateGrid(int Width, int Depth) = 0;

    virtual int LoadTexture2D(const std::string& Filename) = 0;

    virtual int LoadCubemapTexture(const std::string& Filename) = 0;

    virtual void GetWindowSize(int& Width, int& Height) const = 0;

    virtual long long GetElapsedTimeMillis() const = 0;

    virtual Camera* GetCurrentCamera() = 0;
};