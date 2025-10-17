#pragma once

#include <map>
#include <vector>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "material.h"
#include "camera.h"
#include "lights.h"
#include "model_interface.h"
#include "basic_mesh_entry.h"
#include "vulkan_texture.h"


class DemolitionRenderCallbacks
{
public:
    virtual void DrawStart_CB(unsigned int DrawIndex) = 0;

    virtual void ControlSpecularExponent_CB(bool IsEnabled) = 0;

    virtual void SetMaterial_CB(const Material& material) = 0;

    virtual void SetWorldMatrix_CB(const glm::mat4& World) = 0;
};

class CoreRenderingSystem;

class CoreModel : public IModel
{
public:
    CoreModel() {}

    CoreModel(CoreRenderingSystem* pCoreRenderingSystem) { m_pCoreRenderingSystem = pCoreRenderingSystem; }

    bool LoadAssimpModel(const std::string& Filename);

    const std::vector<Camera>& GetCameras() const { return m_cameras; }

    unsigned int NumBones() const { return (unsigned int)m_BoneNameToIndexMap.size(); }

    // This is the main function to drive the animation. It receives the animation time
    // in seconds and a reference to a vector of transformation matrices (one matrix per bone).
    // It calculates the current transformation for each bone according to the current time
    // and updates the corresponding matrix in the vector. This must then be updated in the VS
    // to be accumulated for the final local position (see skinning.vs). The animation index
    // is an optional param which selects one of the animations.
    void GetBoneTransforms(float AnimationTimeSec, std::vector<glm::mat4>& Transforms, unsigned int AnimationIndex = 0);

    // Same as above but this one blends two animations together based on a blending factor
    void GetBoneTransformsBlended(float AnimationTimeSec,
        std::vector<glm::mat4>& Transforms,
        unsigned int StartAnimIndex,
        unsigned int EndAnimIndex,
        float BlendFactor);

    const std::vector<DirectionalLight>& GetDirLights() const { return m_dirLights; }
    const std::vector<SpotLight>& GetSpotLights() const { return m_spotLights; }
    const std::vector<PointLight>& GetPointLights() const { return m_pointLights; }

    void SetTextureScale(float Scale) { m_textureScale = Scale; }

    bool IsAnimated() const;

    const Material* GetMaterialForMesh(int MeshIndex) const;

    virtual void SetColorTexture(int TextureHandle) { assert(0); }

    virtual void SetNormalMap(int TextureHandle) { assert(0); }

    virtual void SetHeightMap(int TextureHandle) { assert(0); }

protected:

    virtual void AllocBuffers() = 0;

    virtual Texture* AllocTexture2D() = 0;

    enum BUFFER_TYPE {
        INDEX_BUFFER = 0,
        VERTEX_BUFFER = 1,
        WVP_MAT_BUFFER = 2,  // required only for instancing
        WORLD_MAT_BUFFER = 3,  // required only for instancing
        NUM_BUFFERS = 4
    };


    struct Vertex {
        glm::vec3 Position;
        glm::vec2 TexCoords;
        glm::vec3 Normal;
        glm::vec3 Tangent;
        glm::vec3 Bitangent;
    };

#define MAX_NUM_BONES_PER_VERTEX 4

    struct VertexBoneData
    {
        unsigned int BoneIDs[MAX_NUM_BONES_PER_VERTEX] = { 0 };
        float Weights[MAX_NUM_BONES_PER_VERTEX] = { 0.0f };
        int index = 0;  // slot for the next update

        VertexBoneData() {}

        void AddBoneData(unsigned int BoneID, float Weight)
        {
            for (int i = 0; i < index; i++) {
                if (BoneIDs[i] == BoneID) {
                    //  printf("bone %d already found at index %d old weight %f new weight %f\n", BoneID, i, Weights[i], Weight);
                    return;
                }
            }

            // The iClone 7 Raptoid Mascot (https://sketchfab.com/3d-models/iclone-7-raptoid-mascot-free-download-56a3e10a73924843949ae7a9800c97c7)
            // has a problem of zero weights causing an overflow and the assertion below. This fixes it.
            if (Weight == 0.0f) {
                return;
            }

            // printf("Adding bone %d weight %f at index %i\n", BoneID, Weight, index);

            if (index == MAX_NUM_BONES_PER_VERTEX) {
                return;
                assert(0);
            }

            BoneIDs[index] = BoneID;
            Weights[index] = Weight;

            index++;
        }
    };

    struct SkinnedVertex {
        glm::vec3 Position;
        glm::vec2 TexCoords;
        glm::vec3 Normal;
        glm::vec3 Tangent;
        glm::vec3 Bitangent;
        VertexBoneData Bones;
    };

    virtual void InitGeometryPost() = 0;

    std::vector<BasicMeshEntry> m_Meshes;
    std::vector<Material> m_Materials;

    // Temporary space for vertex stuff before we load them into the GPU
    std::vector<unsigned int> m_Indices;

    CoreRenderingSystem* m_pCoreRenderingSystem = NULL;

private:

    template<typename VertexType>
    void ReserveSpace(std::vector<VertexType>& Vertices, unsigned int NumVertices, unsigned int NumIndices);

    template<typename VertexType>
    void InitSingleMesh(std::vector<VertexType>& Vertices, unsigned int MeshIndex, const aiMesh* paiMesh);

    template<typename VertexType>
    void InitSingleMeshOpt(std::vector<VertexType>& Vertices, unsigned int MeshIndex, const aiMesh* paiMesh);

    virtual void PopulateBuffersSkinned(std::vector<SkinnedVertex>& Vertices) = 0;

    virtual void PopulateBuffers(std::vector<Vertex>& Vertices) = 0;

    unsigned int CountValidFaces(const aiMesh& Mesh);

    bool InitFromScene(const aiScene* pScene, const std::string& Filename);

    bool InitGeometry(const aiScene* pScene, const std::string& Filename);

    template<typename VertexType>
    void InitGeometryInternal(std::vector<VertexType>& Vertices, int NumVertices, int NumIndices);

    void InitLights(const aiScene* pScene);

    void InitSingleLight(const aiScene* pScene, const aiLight& light);

    void InitDirectionalLight(const aiScene* pScene, const aiLight& light);

    void InitPointLight(const aiScene* pScene, const aiLight& light);

    void InitSpotLight(const aiScene* pScene, const aiLight& light);

    void CountVerticesAndIndices(const aiScene* pScene, unsigned int& NumVertices, unsigned int& NumIndices);

    template<typename VertexType>
    void InitAllMeshes(const aiScene* pScene, std::vector<VertexType>& Vertices);

    template<typename VertexType>
    void OptimizeMesh(int MeshIndex, std::vector<unsigned int>& Indices, std::vector<VertexType>& Vertices, std::vector<VertexType>& AllVertices);

    void CalculateMeshTransformations(const aiScene* pScene);
    void TraverseNodeHierarchy(glm::mat4 ParentTransformation, aiNode* pNode);

    bool InitMaterials(const aiScene* pScene, const std::string& Filename);

    void LoadTextures(const std::string& Dir, const aiMaterial* pMaterial, int index);

    void LoadDiffuseTexture(const std::string& Dir, const aiMaterial* pMaterial, int index);
    void LoadDiffuseTextureEmbedded(const aiTexture* paiTexture, int MaterialIndex);
    void LoadDiffuseTextureFromFile(const std::string& dir, const aiString& Path, int MaterialIndex);

    void LoadSpecularTexture(const std::string& Dir, const aiMaterial* pMaterial, int index);
    void LoadSpecularTextureEmbedded(const aiTexture* paiTexture, int MaterialIndex);
    void LoadSpecularTextureFromFile(const std::string& dir, const aiString& Path, int MaterialIndex);

    void LoadNormalTexture(const std::string& Dir, const aiMaterial* pMaterial, int index);
    void LoadNormalTextureEmbedded(const aiTexture* paiTexture, int MaterialIndex);
    void LoadNormalTextureFromFile(const std::string& dir, const aiString& Path, int MaterialIndex);

    void LoadColors(const aiMaterial* pMaterial, int index);

    void InitCameras(const aiScene* pScene);

    void InitSingleCamera(int Index, const aiScene* pScene);

    const aiScene* m_pScene = NULL;

    glm::mat4 m_GlobalInverseTransform = glm::mat4(1.0f);

    Assimp::Importer m_Importer;

    std::vector<Camera> m_cameras;
    std::vector<DirectionalLight> m_dirLights;
    std::vector<PointLight> m_pointLights;
    std::vector<SpotLight> m_spotLights;
    float m_textureScale = 1.0f;

    glm::vec3 m_minPos = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    glm::vec3 m_maxPos = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    /////////////////////////////////////
    // Skeletal animation stuff
    /////////////////////////////////////

    void LoadMeshBones(std::vector<SkinnedVertex>& SkinnedVertices, unsigned int MeshIndex, const aiMesh* paiMesh);
    void LoadSingleBone(std::vector<SkinnedVertex>& SkinnedVertices, unsigned int MeshIndex, const aiBone* pBone);
    int GetBoneId(const aiBone* pBone);
    void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    unsigned int FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
    unsigned int FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
    unsigned int FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
    const aiNodeAnim* FindNodeAnim(const aiAnimation& Animation, const std::string& NodeName);
    void ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const glm::mat4& ParentTransform, const aiAnimation& Animation);
    void ReadNodeHierarchyBlended(float StartAnimationTimeTicksm, float EndAnimationTimeTicks, const aiNode* pNode, const glm::mat4& ParentTransform,
        const aiAnimation& StartAnimation, const aiAnimation& EndAnimation, float BlendFactor);
    void MarkRequiredNodesForBone(const aiBone* pBone);
    void InitializeRequiredNodeMap(const aiNode* pNode);
    float CalcAnimationTimeTicks(float TimeInSeconds, unsigned int AnimationIndex);

    struct LocalTransform {
        aiVector3D Scaling;
        aiQuaternion Rotation;
        aiVector3D Translation;
    };

    void CalcLocalTransform(LocalTransform& Transform, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim);

    std::map<std::string, unsigned int> m_BoneNameToIndexMap;

    struct BoneInfo
    {
        glm::mat4 OffsetMatrix;
        glm::mat4 FinalTransformation;

        BoneInfo(const glm::mat4& Offset)
        {
            OffsetMatrix = Offset;
            FinalTransformation = glm::mat4(0.0f);
        }
    };

    std::vector<BoneInfo> m_BoneInfo;

    struct NodeInfo {

        NodeInfo() {}

        NodeInfo(const aiNode* n) { pNode = n; }

        const aiNode* pNode = NULL;
        bool isRequired = false;
    };

    std::map<std::string, NodeInfo> m_requiredNodeMap;
};