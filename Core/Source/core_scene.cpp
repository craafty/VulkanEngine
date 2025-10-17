#include "core_scene.h"
#include "core_rendering_system.h"

#define NUM_SCENE_OBJECTS 1024

SceneObject::SceneObject()
{
    memset(&m_rotations[0], 0, sizeof(m_rotations));
}

void SceneObject::SetRotation(const glm::vec3& Rot)
{
    m_rotations[0] = Rot;
    m_numRotations = 1;
}


void SceneObject::SetRotation(float x, float y, float z)
{
    m_rotations[0].x = x;
    m_rotations[0].y = y;
    m_rotations[0].z = z;
    m_numRotations = 1;
}

void SceneObject::RotateBy(float x, float y, float z)
{
    m_rotations[0].x += x;
    m_rotations[0].y += y;
    m_rotations[0].z += z;
    m_numRotations = 1;
}


void SceneObject::PushRotation(const glm::vec3& Rot)
{
    if (m_numRotations >= MAX_NUM_ROTATIONS) {
        printf("Exceeded max number of rotations - %d\n", m_numRotations);
        assert(0);
    }

    m_rotations[m_numRotations] = Rot;
    m_numRotations++;
}


bool QuaternionIsZero(const glm::quat& q)
{
    bool r = (q.x == 0 && q.y == 0 && q.z == 0.0f && q.w == 0.0f);
    return r;
}

glm::mat4 SceneObject::GetMatrix() const
{
    glm::mat4 Scale = glm::scale(glm::mat4(1.0f), m_scale);

    glm::mat4 Rotation;

    if (QuaternionIsZero(m_quaternion)) {
        CalcRotationStack(Rotation);
    }
    else {
        Rotation = glm::mat4_cast(m_quaternion);
    }

    glm::mat4 Translation = glm::translate(glm::mat4(1.0f), m_pos);

    glm::mat4 WorldTransformation = Translation * Rotation * Scale;

    return WorldTransformation;
}


void SceneObject::CalcRotationStack(glm::mat4& Rot) const
{
    if (m_numRotations == 0) {
        Rot = glm::mat4(1.0f);
    }
    else {
        Rot = glm::mat4_cast(glm::quat(glm::radians(m_rotations[0])));

        if (m_numRotations > MAX_NUM_ROTATIONS) {
            printf("Invalid number of rotations - %d\n", m_numRotations);
            assert(0);
        }
        for (int i = 1; i < m_numRotations; i++) {
            glm::mat4 r = glm::mat4_cast(glm::quat(glm::radians(m_rotations[i])));
            Rot = r * Rot;
        }
    }
}


SceneConfig::SceneConfig()
{

}



IScene::IScene()
{
}


CoreScene::CoreScene(CoreRenderingSystem* pRenderingSystem)
{
    m_pCoreRenderingSystem = pRenderingSystem;
    CreateDefaultCamera();
    m_sceneObjects.resize(NUM_SCENE_OBJECTS);
}

void CoreScene::LoadScene(const std::string& Filename)
{
    CoreModel* pModel = (CoreModel*)m_pCoreRenderingSystem->LoadModel(Filename.c_str());
    SceneObject* pSceneObject = CreateSceneObject(pModel);
    AddToRenderList(pSceneObject);

    if (pModel->GetCameras().size() == 0) {
        printf("Warning! '%s' does not include a camera. Falling back to default.\n", Filename.c_str());
    }
    else {
        //   m_defaultCamera = pModel->GetCameras()[0];
    }
}


void CoreScene::InitializeDefault()
{
    SceneObject* pSceneObject = CreateSceneObject("square");
    AddToRenderList(pSceneObject);
    pSceneObject->SetRotation(glm::vec3(-90.0f, 0.0f, 0.0f));
    pSceneObject->SetScale(glm::vec3(1000.0f, 1000.0f, 1000.0f));
    pSceneObject->SetFlatColor(glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
}


void CoreScene::CreateDefaultCamera()
{
    glm::vec3 Pos(0.0f, 0.0f, 0.0f);
    glm::vec3 Target(0.0f, 0.f, 1.0f);
    glm::vec3 Up(0.0, 1.0f, 0.0f);

    float FOV = 45.0f;
    float zNear = 0.1f;
    float zFar = 1000.0f;
    int WindowWidth = 0;
    int WindowHeight = 0;
    m_pCoreRenderingSystem->GetWindowSize(WindowWidth, WindowHeight);

    PersProjInfo persProjInfo = { FOV, (float)WindowWidth, (float)WindowHeight, zNear, zFar };

    m_defaultCamera.Init(Pos, persProjInfo);
}


void CoreScene::SetCamera(const glm::vec3& Pos, const glm::vec3& Target)
{
    m_defaultCamera.SetPos(Pos);
    m_defaultCamera.SetTarget(Target);
    m_defaultCamera.SetUp(glm::vec3(0.0f, 1.0f, 0.0f));
}


void CoreScene::SetCameraSpeed(float Speed)
{
    printf("Warning! SetCameraSpeed is not implemented!!!\n");
    //   m_defaultCamera.SetSpeed(Speed);
}


void CoreScene::AddToRenderList(SceneObject* pSceneObject)
{
    CoreSceneObject* pCoreSceneObject = (CoreSceneObject*)pSceneObject;
    std::list<CoreSceneObject*>::const_iterator it = std::find(m_renderList.begin(), m_renderList.end(), pCoreSceneObject);

    if (it == m_renderList.end()) {
        m_renderList.push_back(pCoreSceneObject);
    }
}


bool CoreScene::RemoveFromRenderList(SceneObject* pSceneObject)
{
    std::list<CoreSceneObject*>::const_iterator it = std::find(m_renderList.begin(), m_renderList.end(), pSceneObject);

    bool ret = false;

    if (it != m_renderList.end()) {
        m_renderList.erase(it);
        ret = true;
    }

    return ret;
}



std::list<SceneObject*> CoreScene::GetSceneObjectsList()
{
    // TODO: not very efficient. Currently used only by the GUI. For small lists it should be ok.

    std::list<SceneObject*> ObjectList;

    for (std::list<CoreSceneObject*>::const_iterator it = m_renderList.begin(); it != m_renderList.end(); it++) {
        ObjectList.push_back(*it);
    }

    return ObjectList;
}


SceneObject* CoreScene::CreateSceneObject(IModel* pModel)
{
    if (m_numSceneObjects == NUM_SCENE_OBJECTS) {
        printf("%s:%d - out of scene objects space\n", __FILE__, __LINE__);
        exit(0);
    }

    CoreSceneObject* pCoreSceneObject = CreateSceneObjectInternal((CoreModel*)pModel);

    return pCoreSceneObject;
}


SceneObject* CoreScene::CreateSceneObject(const std::string& BasicShape)
{
    CoreModel* pModel = (CoreModel*)m_pCoreRenderingSystem->GetModel(BasicShape);

    CoreSceneObject* pCoreSceneObject = CreateSceneObjectInternal(pModel);

    return pCoreSceneObject;
}


CoreSceneObject* CoreScene::CreateSceneObjectInternal(CoreModel* pModel)
{
    m_sceneObjects[m_numSceneObjects].SetModel(pModel);

    CoreSceneObject* pCoreSceneObject = &(m_sceneObjects[m_numSceneObjects]);
    int Id = m_numSceneObjects;
    pCoreSceneObject->SetId(Id);
    pCoreSceneObject->SetName("SceneObject_" + std::to_string(Id));

    m_numSceneObjects++;

    return pCoreSceneObject;
}


const std::vector<PointLight>& CoreScene::GetPointLights()
{
    if (m_pointLights.size() > 0) {
        return m_pointLights;
    }

    for (std::list<CoreSceneObject*>::const_iterator it = m_renderList.begin(); it != m_renderList.end(); it++) {
        CoreSceneObject* pSceneObject = *it;

        const std::vector<PointLight>& PointLights = pSceneObject->GetModel()->GetPointLights();

        if (PointLights.size() > 0) {
            return PointLights;
        }
    }

    return m_pointLights;
}


const std::vector<SpotLight>& CoreScene::GetSpotLights()
{
    if (m_spotLights.size() > 0) {
        return m_spotLights;
    }

    for (std::list<CoreSceneObject*>::const_iterator it = m_renderList.begin(); it != m_renderList.end(); it++) {
        CoreSceneObject* pSceneObject = *it;

        const std::vector<SpotLight>& SpotLights = pSceneObject->GetModel()->GetSpotLights();

        if (SpotLights.size() > 0) {
            return SpotLights;
        }
    }

    return m_spotLights;
}


const std::vector<DirectionalLight>& CoreScene::GetDirLights()
{
    if (m_dirLights.size() > 0) {
        return m_dirLights;
    }

    for (std::list<CoreSceneObject*>::const_iterator it = m_renderList.begin(); it != m_renderList.end(); it++) {
        CoreSceneObject* pSceneObject = *it;

        const std::vector<DirectionalLight>& DirLights = pSceneObject->GetModel()->GetDirLights();

        if (DirLights.size() > 0) {
            return DirLights;
        }
    }

    return m_dirLights;

}