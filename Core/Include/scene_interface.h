#pragma once

#include <list>

#include "camera.h"
#include "lights.h"
#include "model_interface.h"

// Nobody needs more than 640k
#define MAX_NUM_ROTATIONS 8

class SceneObject : public SceneObjectBase {
public:
    void SetPosition(float x, float y, float z) { m_pos.x = x; m_pos.y = y; m_pos.z = z; }
    void SetRotation(float x, float y, float z);
    void SetScale(float x, float y, float z) { m_scale.x = x; m_scale.y = y; m_scale.z = z; }

    void SetPosition(const glm::vec3& Pos) { m_pos = Pos; }
    const glm::vec3& GetPosition() const { return m_pos; }
    void SetRotation(const glm::vec3& Rot);
    void PushRotation(const glm::vec3& Rot);
    void ResetRotations() { m_numRotations = 0; }
    void SetScale(const glm::vec3& Scale) { m_scale = Scale; }

    void RotateBy(float x, float y, float z);

    glm::mat4 GetMatrix() const;

    void SetFlatColor(const glm::vec4 Col) { m_flatColor = Col; }
    const glm::vec4& GetFlatColor() const { return m_flatColor; }

    void SetColorMod(float r, float g, float b) { m_colorMod.r = r; m_colorMod.g = g; m_colorMod.b = b; }
    glm::vec3 GetColorMod() const { return m_colorMod; }

    void SetQuaternion(const glm::quat& q) { m_quaternion = q; }

protected:
    SceneObject();
    void CalcRotationStack(glm::mat4& Rot) const;

    glm::vec3 m_pos = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 m_scale = glm::vec3(1.0f, 1.0f, 1.0f);

private:

    glm::vec3 m_rotations[MAX_NUM_ROTATIONS];
    int m_numRotations = 0;
    glm::vec4 m_flatColor = glm::vec4(-1.0f, -1.0f, -1.0f, -1.0f);
    glm::vec3 m_colorMod = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::quat m_quaternion = glm::quat(0.0f, 0.0f, 0.0f, 0.0f);
};


struct InfiniteGridConfig {
    bool Enabled = false;
    float Size = 100.0f;
    float CellSize = 0.025f;
    glm::vec4 ColorThin = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
    glm::vec4 ColorThick = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    float MinPixelsBetweenCells = 2.0f;
    bool ShadowsEnabled = false;
};

class SceneConfig
{
public:

    SceneConfig();

    void ControlShadowMapping(bool EnableShadowMapping) { m_shadowMappingEnabled = EnableShadowMapping; }
    bool IsShadowMappingEnabled() const { return m_shadowMappingEnabled; }

    void ControlPicking(bool EnablePicking) { m_pickingEnabled = EnablePicking; }
    bool IsPickingEnabled() const { return m_pickingEnabled; }

    void ControlSkybox(bool EnableSkybox) { m_skyboxEnabled = EnableSkybox; }
    bool IsSkyboxEnabled() const { return m_skyboxEnabled; }

    InfiniteGridConfig& GetInfiniteGrid() { return m_infiniteGridConfig; }

private:

    bool m_shadowMappingEnabled = true;
    bool m_pickingEnabled = false;
    bool m_skyboxEnabled = false;
    InfiniteGridConfig m_infiniteGridConfig;
};


struct CameraState {

};

class IScene {
public:
    IScene();

    ~IScene() {}

    virtual SceneObject* CreateSceneObject(IModel* pModel) = 0;

    virtual SceneObject* CreateSceneObject(const std::string& BasicShape) = 0;

    virtual void AddToRenderList(SceneObject* pSceneObject) = 0;

    virtual bool RemoveFromRenderList(SceneObject* pSceneObject) = 0;

    virtual std::list<SceneObject*> GetSceneObjectsList() = 0;

    virtual void SetCamera(const glm::vec3& Pos, const glm::vec3& Target) = 0;

    virtual Camera* GetCurrentCamera() = 0;

    virtual void SetCameraSpeed(float Speed) = 0;

    virtual SceneObject* GetPickedSceneObject() const = 0;

    virtual SceneConfig* GetConfig() = 0;

    virtual void LoadSkybox(const char* pFilename) = 0;

    std::vector<PointLight>& GetPointLights() { return m_pointLights; }

    std::vector<SpotLight>& GetSpotLights() { return m_spotLights; }

    std::vector<DirectionalLight>& GetDirLights() { return m_dirLights; }

    void SetClearColor(const glm::vec4& Color) { m_clearColor = Color; m_clearFrame = true; }

    void DisableClear() { m_clearFrame = false; }

protected:
    bool m_clearFrame = false;
    glm::vec4 m_clearColor;

    std::vector<PointLight> m_pointLights;
    std::vector<SpotLight> m_spotLights;
    std::vector<DirectionalLight> m_dirLights;
};