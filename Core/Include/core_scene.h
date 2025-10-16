#pragma once

#include <list>

#include "scene_interface.h"
#include "scene_object.h"
#include "core_model.h"


class CoreSceneObject : public SceneObject {
public:
    CoreSceneObject() {}

    void SetModel(CoreModel* pModel)
    {
        if (m_pModel) {
            printf("%s:%d - model already initialized\n", __FILE__, __LINE__);
            exit(0);
        }

        m_pModel = pModel;
    }

    CoreModel* GetModel() const { return m_pModel; }

    void SetId(int id) { m_id = id; }

    int GetId() const { return m_id; }

private:
    CoreModel* m_pModel = NULL;
    int m_id = -1;
};


class CoreRenderingSystem;

/*class CoreSceneConfig : public SceneConfig()
{
public:
    CoreSceneConfig() {}
}*/

class CoreScene : public IScene, public SceneObject {
public:
    CoreScene(CoreRenderingSystem* pRenderingSystem);

    virtual ~CoreScene() {}

    virtual void LoadScene(const std::string& Filename);

    virtual SceneObject* CreateSceneObject(IModel* pModel);

    virtual SceneObject* CreateSceneObject(const std::string& BasicShape);

    virtual std::list<SceneObject*> GetSceneObjectsList();

    const std::vector<PointLight>& GetPointLights();

    const std::vector<SpotLight>& GetSpotLights();

    const std::vector<DirectionalLight>& GetDirLights();

    Camera* GetCurrentCamera() { return &m_defaultCamera; }

    void InitializeDefault();

    const std::list<CoreSceneObject*>& GetRenderList() { return m_renderList; }

    void AddToRenderList(SceneObject* pSceneObject);

    bool RemoveFromRenderList(SceneObject* pSceneObject);

    bool IsClearFrame() const { return m_clearFrame; }

    const glm::vec4& GetClearColor() { return m_clearColor; }

    void SetCamera(const glm::vec3& Pos, const glm::vec3& Target);

    void SetCameraSpeed(float Speed);

    void SetPickedSceneObject(CoreSceneObject* pSceneObject) { m_pPickedSceneObject = pSceneObject; }

    SceneObject* GetPickedSceneObject() const { return m_pPickedSceneObject; }

    SceneConfig* GetConfig() { return &m_config; }

protected:
    CoreRenderingSystem* m_pCoreRenderingSystem = NULL;
    std::list<CoreSceneObject*> m_renderList;

private:
    void CreateDefaultCamera();
    CoreSceneObject* CreateSceneObjectInternal(CoreModel* pModel);

    Camera m_defaultCamera;
    std::vector<CoreSceneObject> m_sceneObjects;
    int m_numSceneObjects = 0;
    CoreSceneObject* m_pPickedSceneObject = NULL;
    SceneConfig m_config;
};