#pragma once

#include "camera.h"
#include "rendering_system_interface.h"
#include "core_model.h"
#include "core_scene.h"


class CoreRenderingSystem : public IRenderingSystem
{
public:

    virtual IScene* CreateScene(const std::string& Filename);

    virtual IScene* CreateDefaultScene();

    virtual void* CreateWindow(int Width, int Height, const char* pWindowName);

    virtual IModel* LoadModel(const std::string& Filename);

    virtual Grid* CreateGrid(int Width, int Depth);

    virtual IModel* GetModel(const std::string& BasicShape);

    virtual void SetScene(IScene* pScene);

    virtual IScene* GetScene();

    virtual BaseTexture* GetTexture(int TextureHandle) = 0;

    virtual void GetWindowSize(int& Width, int& Height) const { Width = m_windowWidth; Height = m_windowHeight; }

    virtual long long GetElapsedTimeMillis() const { return m_elapsedTimeMillis; }

    virtual Camera* GetCurrentCamera() { return m_pCamera; }

protected:

    CoreRenderingSystem(GameCallbacks* pGameCallbacks, bool LoadBasicShapes);

    ~CoreRenderingSystem();

    virtual void* CreateWindowInternal(const char* pWindowName) = 0;

    virtual CoreModel* LoadModelInternal(const std::string& Filename) = 0;

    virtual Grid* CreateGridInternal(int Width, int Depth) = 0;

    virtual void SetCamera(Camera* pCamera) = 0;

    long long m_elapsedTimeMillis = 0;
    int m_windowWidth = 0;
    int m_windowHeight = 0;
    Camera* m_pCamera = NULL;
    GameCallbacks* m_pGameCallbacks = NULL;
    GameCallbacks m_defaultGameCallbacks;
    CoreScene* m_pScene = NULL;

private:
    void InitializeBasicShapes();

    std::vector<CoreModel*> m_models;
    std::map<std::string, CoreModel*> m_shapeToModel;
    int m_numModels = 0;
    bool m_loadBasicShapes = false;
};