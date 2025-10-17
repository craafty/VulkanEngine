#include <glm/gtx/string_cast.hpp>
#include "core_rendering_system.h"
#include "core_model.h"
#include "util.h"
#include "meshoptimizer.h"
#include <algorithm>

using namespace std;

// config flags
static bool UseMeshOptimizer = false;

#define MAX_BONES 100

#define DEMOLITION_ASSIMP_LOAD_FLAGS (aiProcess_JoinIdenticalVertices | \
                                      aiProcess_Triangulate | \
                                      aiProcess_GenSmoothNormals | \
                                      aiProcess_LimitBoneWeights | \
                                      aiProcess_SplitLargeMeshes | \
                                      aiProcess_ImproveCacheLocality | \
                                      aiProcess_RemoveRedundantMaterials | \
                                      aiProcess_FindDegenerates | \
                                      aiProcess_FindInvalidData | \
                                      aiProcess_GenUVCoords | \
                                      aiProcess_CalcTangentSpace)

//aiProcess_MakeLeftHanded | \
//aiProcess_FlipWindingOrder | \

Texture* s_pMissingTexture = NULL;

static void traverse(int depth, aiNode* pNode);
static bool GetFullTransformation(const aiNode* pRootNode, const char* pName, glm::mat4& Transformation);

inline glm::mat4 AssimpToGLM(const aiMatrix4x4& from) {
    glm::mat4 to;
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}

static inline glm::vec3 VectorFromAssimpVector(const aiVector3D& v)
{
    return glm::vec3(v.x, v.y, v.z);
}

static inline float ToDegree(float radians)
{
    return radians * 180.0f / 3.1415926535f;
}


bool CoreModel::LoadAssimpModel(const string& Filename)
{
    AllocBuffers();

    bool Ret = false;

    m_pScene = m_Importer.ReadFile(Filename.c_str(), DEMOLITION_ASSIMP_LOAD_FLAGS);

    if (m_pScene) {
        printf("--- START Node Hierarchy ---\n");
        traverse(0, m_pScene->mRootNode);
        printf("--- END Node Hierarchy ---\n");
        m_GlobalInverseTransform = glm::inverse(AssimpToGLM(m_pScene->mRootNode->mTransformation));
        Ret = InitFromScene(m_pScene, Filename);
    }
    else {
        printf("Error parsing '%s': '%s'\n", Filename.c_str(), m_Importer.GetErrorString());
    }

    return Ret;
}


bool CoreModel::InitFromScene(const aiScene* pScene, const string& Filename)
{
    if (!InitGeometry(pScene, Filename)) {
        return false;
    }

    InitCameras(pScene);

    InitLights(pScene);

    return true;
}


bool CoreModel::InitGeometry(const aiScene* pScene, const string& Filename)
{
    printf("\n*** Initializing geometry ***\n");
    m_Meshes.resize(pScene->mNumMeshes);
    m_Materials.resize(pScene->mNumMaterials);

    unsigned int NumVertices = 0;
    unsigned int NumIndices = 0;

    CountVerticesAndIndices(pScene, NumVertices, NumIndices);

    printf("Num animations %d\n", pScene->mNumAnimations);

    if (pScene->mNumAnimations > 0) {
        std::vector<SkinnedVertex> Vertices;
        InitGeometryInternal<SkinnedVertex>(Vertices, NumVertices, NumIndices);
        PopulateBuffersSkinned(Vertices);
    }
    else {
        std::vector<Vertex> Vertices;
        InitGeometryInternal<Vertex>(Vertices, NumVertices, NumIndices);
        PopulateBuffers(Vertices);
    }

    if (!InitMaterials(pScene, Filename)) {
        return false;
    }

    CalculateMeshTransformations(pScene);

    InitGeometryPost();

    return true;
}


template<typename VertexType>
void CoreModel::InitGeometryInternal(std::vector<VertexType>& Vertices, int NumVertices, int NumIndices)
{
    ReserveSpace<VertexType>(Vertices, NumVertices, NumIndices);

    InitAllMeshes<VertexType>(m_pScene, Vertices);

    printf("Min pos: %s\n", glm::to_string(m_minPos).c_str());
    printf("Max pos: %s\n", glm::to_string(m_maxPos).c_str());
}




void CoreModel::CountVerticesAndIndices(const aiScene* pScene, unsigned int& NumVertices, unsigned int& NumIndices)
{
    for (unsigned int i = 0; i < m_Meshes.size(); i++) {
        m_Meshes[i].MaterialIndex = pScene->mMeshes[i]->mMaterialIndex;
        m_Meshes[i].ValidFaces = CountValidFaces(*pScene->mMeshes[i]);
        m_Meshes[i].NumIndices = m_Meshes[i].ValidFaces * 3;
        m_Meshes[i].NumVertices = pScene->mMeshes[i]->mNumVertices;
        m_Meshes[i].BaseVertex = NumVertices;
        m_Meshes[i].BaseIndex = NumIndices;

        NumVertices += pScene->mMeshes[i]->mNumVertices;
        NumIndices += m_Meshes[i].NumIndices;
    }
}


unsigned int CoreModel::CountValidFaces(const aiMesh& Mesh)
{
    unsigned int NumValidFaces = 0;

    for (unsigned int i = 0; i < Mesh.mNumFaces; i++) {
        if (Mesh.mFaces[i].mNumIndices == 3) {
            NumValidFaces++;
        }
    }

    return NumValidFaces;
}


template<typename VertexType>
void CoreModel::ReserveSpace(std::vector<VertexType>& Vertices, unsigned int NumVertices, unsigned int NumIndices)
{
    Vertices.reserve(NumVertices);
    m_Indices.reserve(NumIndices);
    //m_Bones.resize(NumVertices); // TODO: only if there are any bones
    InitializeRequiredNodeMap(m_pScene->mRootNode);
}


template<typename VertexType>
void CoreModel::InitAllMeshes(const aiScene* pScene, std::vector<VertexType>& Vertices)
{
    for (unsigned int i = 0; i < m_Meshes.size(); i++) {
        const aiMesh* paiMesh = pScene->mMeshes[i];
        if (UseMeshOptimizer) {
            InitSingleMeshOpt<VertexType>(Vertices, i, paiMesh);
        }
        else {
            InitSingleMesh<VertexType>(Vertices, i, paiMesh);
        }
    }
}


void CoreModel::CalculateMeshTransformations(const aiScene* pScene)
{
    printf("----------------------------------------\n");
    printf("Calculating mesh transformations\n");
    glm::mat4 Transformation(1.0f);

    TraverseNodeHierarchy(Transformation, pScene->mRootNode);
}


void CoreModel::TraverseNodeHierarchy(glm::mat4 ParentTransformation, aiNode* pNode)
{
    printf("Traversing node '%s'\n", pNode->mName.C_Str());
    glm::mat4 NodeTransformation = AssimpToGLM(pNode->mTransformation);

    glm::mat4 CombinedTransformation = ParentTransformation * NodeTransformation;

    if (pNode->mNumMeshes > 0) {
        printf("Num meshes: %d - ", pNode->mNumMeshes);
        for (int i = 0; i < (int)pNode->mNumMeshes; i++) {
            int MeshIndex = pNode->mMeshes[i];
            printf("%d ", MeshIndex);
            m_Meshes[MeshIndex].Transformation = CombinedTransformation;
        }
        printf("\n");
    }
    else {
        printf("No meshes\n");
    }

    for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
        TraverseNodeHierarchy(CombinedTransformation, pNode->mChildren[i]);
    }

}


template<typename VertexType>
void CoreModel::InitSingleMesh(vector<VertexType>& Vertices, unsigned int MeshIndex, const aiMesh* paiMesh)
{
    const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

    printf("Mesh %d %s\n", MeshIndex, paiMesh->mName.C_Str());
    // Populate the vertex attribute vectors
    VertexType v;

    for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {
        const aiVector3D& Pos = paiMesh->mVertices[i];
        v.Position = glm::vec3(Pos.x, Pos.y, Pos.z);

        m_minPos.x = std::min(m_minPos.x, v.Position.x);
        m_minPos.y = std::min(m_minPos.y, v.Position.y);
        m_minPos.z = std::min(m_minPos.z, v.Position.z);

        m_maxPos.x = std::max(m_maxPos.x, v.Position.x);
        m_maxPos.y = std::max(m_maxPos.y, v.Position.y);
        m_maxPos.z = std::max(m_maxPos.z, v.Position.z);

        if (paiMesh->mNormals) {
            const aiVector3D& pNormal = paiMesh->mNormals[i];
            v.Normal = glm::vec3(pNormal.x, pNormal.y, pNormal.z);
        }
        else {
            aiVector3D Normal(0.0f, 1.0f, 0.0f);
            v.Normal = glm::vec3(Normal.x, Normal.y, Normal.z);
        }

        if (paiMesh->HasTextureCoords(0)) {
            const aiVector3D& pTexCoord = paiMesh->mTextureCoords[0][i];
            v.TexCoords = glm::vec2(pTexCoord.x, pTexCoord.y);

            const aiVector3D& pTangent = paiMesh->mTangents[i];
            v.Tangent = glm::vec3(pTangent.x, pTangent.y, pTangent.z);

            const aiVector3D& pBitangent = paiMesh->mBitangents[i];
            v.Bitangent = glm::vec3(pBitangent.x, pBitangent.y, pBitangent.z);
        }
        else {
            v.TexCoords = glm::vec2(0.0f);
            v.Tangent = glm::vec3(0.0f);
            v.Bitangent = glm::vec3(0.0f);
        }

        /*   printf("Pos %d: ", i); v.Position.Print();
           printf("Normal: "); v.Normal.Print();
           printf("Tangent: "); v.Tangent.Print();
           printf("Bitangent: "); v.Bitangent.Print();*/

        Vertices.push_back(v);
    }

    // Populate the index buffer
    for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
        const aiFace& Face = paiMesh->mFaces[i];
        //  printf("num indices %d\n", Face.mNumIndices);
        if (Face.mNumIndices != 3) {
            printf("Warning! face %d has %d indices\n", i, Face.mNumIndices);
            continue;
        }
        /*   printf("%d: %d\n", i * 3, Face.mIndices[0]);
           printf("%d: %d\n", i * 3 + 1, Face.mIndices[1]);
           printf("%d: %d\n", i * 3 + 2, Face.mIndices[2]);*/
        m_Indices.push_back(Face.mIndices[0]);
        m_Indices.push_back(Face.mIndices[1]);
        m_Indices.push_back(Face.mIndices[2]);
    }

    if constexpr (std::is_same_v<VertexType, SkinnedVertex>) {
        LoadMeshBones(Vertices, MeshIndex, paiMesh);
    }
}


template<typename VertexType>
void CoreModel::InitSingleMeshOpt(vector<VertexType>& AllVertices, unsigned int MeshIndex, const aiMesh* paiMesh)
{
    const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

    // printf("Mesh %d\n", MeshIndex);
    // Populate the vertex attribute vectors
    VertexType v;

    std::vector<VertexType> Vertices(paiMesh->mNumVertices);

    for (unsigned int i = 0; i < paiMesh->mNumVertices; i++) {
        const aiVector3D& Pos = paiMesh->mVertices[i];
        // printf("%d: ", i); glm::vec3 v(pPos.x, pPos.y, pPos.z); v.Print();
        v.Position = glm::vec3(Pos.x, Pos.y, Pos.z);

        m_minPos.x = std::min(m_minPos.x, v.Position.x);
        m_minPos.y = std::min(m_minPos.y, v.Position.y);
        m_minPos.z = std::min(m_minPos.z, v.Position.z);
        m_maxPos.x = std::max(m_maxPos.x, v.Position.x);
        m_maxPos.y = std::max(m_maxPos.y, v.Position.y);
        m_maxPos.z = std::max(m_maxPos.z, v.Position.z);

        if (paiMesh->mNormals) {
            const aiVector3D& pNormal = paiMesh->mNormals[i];
            v.Normal = glm::vec3(pNormal.x, pNormal.y, pNormal.z);
        }
        else {
            aiVector3D Normal(0.0f, 1.0f, 0.0f);
            v.Normal = glm::vec3(Normal.x, Normal.y, Normal.z);
        }

        const aiVector3D& pTexCoord = paiMesh->HasTextureCoords(0) ? paiMesh->mTextureCoords[0][i] : Zero3D;
        v.TexCoords = glm::vec2(pTexCoord.x, pTexCoord.y);

        const aiVector3D& pTangent = paiMesh->mTangents[i];
        v.Tangent = glm::vec3(pTangent.x, pTangent.y, pTangent.z);

        const aiVector3D& pBitangent = paiMesh->mBitangents[i];
        v.Bitangent = glm::vec3(pBitangent.x, pBitangent.y, pBitangent.z);

        /*   printf("Pos %d: ", i); v.Position.Print();
           printf("Normal: "); v.Normal.Print();
           printf("Tangent: "); v.Tangent.Print();
           printf("Bitangent: "); v.Bitangent.Print();*/


        Vertices[i] = v;
    }

    m_Meshes[MeshIndex].BaseVertex = (unsigned int)AllVertices.size();
    m_Meshes[MeshIndex].BaseIndex = (unsigned int)m_Indices.size();

    int NumIndices = paiMesh->mNumFaces * 3;

    std::vector<unsigned int> Indices;
    Indices.resize(NumIndices);

    // Populate the index buffer
    for (unsigned int i = 0; i < paiMesh->mNumFaces; i++) {
        const aiFace& Face = paiMesh->mFaces[i];

        if (Face.mNumIndices != 3) {
            printf("Warning! face %d has %d indices\n", i, Face.mNumIndices);
            continue;
        }

        Indices[i * 3 + 0] = Face.mIndices[0];
        Indices[i * 3 + 1] = Face.mIndices[1];
        Indices[i * 3 + 2] = Face.mIndices[2];
    }

    if constexpr (std::is_same_v<VertexType, SkinnedVertex>) {
        LoadMeshBones(Vertices, MeshIndex, paiMesh);
    }

    OptimizeMesh(MeshIndex, Indices, Vertices, AllVertices);
}


template<typename VertexType>
void CoreModel::OptimizeMesh(int MeshIndex, std::vector<unsigned int>& Indices, std::vector<VertexType>& Vertices, std::vector<VertexType>& AllVertices)
{
    size_t NumIndices = Indices.size();
    size_t NumVertices = Vertices.size();

    // Create a remap table
    std::vector<unsigned int> remap(NumIndices);
    size_t OptVertexCount = meshopt_generateVertexRemap(remap.data(),    // dst addr
        Indices.data(),  // src indices
        NumIndices,      // ...and size
        Vertices.data(), // src vertices
        NumVertices,     // ...and size
        sizeof(VertexType)); // stride
    // Allocate a local index/vertex arrays
    std::vector<unsigned int> OptIndices;
    std::vector<VertexType> OptVertices;
    OptIndices.resize(NumIndices);
    OptVertices.resize(OptVertexCount);

    // Optimization #1: remove duplicate vertices    
    meshopt_remapIndexBuffer(OptIndices.data(), Indices.data(), NumIndices, remap.data());

    meshopt_remapVertexBuffer(OptVertices.data(), Vertices.data(), NumVertices, sizeof(VertexType), remap.data());

    // Optimization #2: improve the locality of the vertices
    meshopt_optimizeVertexCache(OptIndices.data(), OptIndices.data(), NumIndices, OptVertexCount);

    // Optimization #3: reduce pixel overdraw
    meshopt_optimizeOverdraw(OptIndices.data(), OptIndices.data(), NumIndices, &(OptVertices[0].Position.x), OptVertexCount, sizeof(VertexType), 1.05f);

    // Optimization #4: optimize access to the vertex buffer
    meshopt_optimizeVertexFetch(OptVertices.data(), OptIndices.data(), NumIndices, OptVertices.data(), OptVertexCount, sizeof(VertexType));

    // Optimization #5: create a simplified version of the model
    float Threshold = 1.0f;
    size_t TargetIndexCount = (size_t)(NumIndices * Threshold);

    float TargetError = 0.0f;
    std::vector<unsigned int> SimplifiedIndices(OptIndices.size());
    size_t OptIndexCount = meshopt_simplify(SimplifiedIndices.data(), OptIndices.data(), NumIndices,
        &OptVertices[0].Position.x, OptVertexCount, sizeof(VertexType), TargetIndexCount, TargetError);

    static int num_indices = 0;
    num_indices += (int)NumIndices;
    static int opt_indices = 0;
    opt_indices += (int)OptIndexCount;
    printf("Num indices %d\n", num_indices);
    //printf("Target num indices %d\n", TargetIndexCount);
    printf("Optimized number of indices %d\n", opt_indices);
    SimplifiedIndices.resize(OptIndexCount);

    // Concatenate the local arrays into the class attributes arrays
    m_Indices.insert(m_Indices.end(), SimplifiedIndices.begin(), SimplifiedIndices.end());

    AllVertices.insert(AllVertices.end(), OptVertices.begin(), OptVertices.end());

    m_Meshes[MeshIndex].NumIndices = (unsigned int)OptIndexCount;
}


bool CoreModel::InitMaterials(const aiScene* pScene, const string& Filename)
{
    string Dir = GetDirFromFilename(Filename);

    bool Ret = true;

    printf("Num materials: %d\n", pScene->mNumMaterials);

    // Initialize the materials
    for (unsigned int i = 0; i < pScene->mNumMaterials; i++) {
        const aiMaterial* pMaterial = pScene->mMaterials[i];

        printf("Loading material %d: '%s'\n", i, pMaterial->GetName().C_Str());

        LoadTextures(Dir, pMaterial, i);

        LoadColors(pMaterial, i);
    }

    return Ret;
}


const Material* CoreModel::GetMaterialForMesh(int MeshIndex) const
{
    if (MeshIndex >= m_Meshes.size()) {
        printf("Invalid mesh index %d, num meshes %d\n", MeshIndex, (int)m_Meshes.size());
        exit(1);
    }

    int MaterialIndex = m_Meshes[MeshIndex].MaterialIndex;

    const Material* pMaterial = NULL;

    if ((MaterialIndex >= 0) && (MaterialIndex < m_Materials.size())) {
        pMaterial = &m_Materials[MaterialIndex];
    }

    return pMaterial;
}


static std::string TextureTypeToString(aiTextureType type) {
    static std::map<aiTextureType, std::string> textureTypeNames = {
        {aiTextureType_DIFFUSE, "Diffuse"},
        {aiTextureType_SPECULAR, "Specular"},
        {aiTextureType_AMBIENT, "Ambient"},
        {aiTextureType_EMISSIVE, "Emissive"},
        {aiTextureType_HEIGHT, "Height"},
        {aiTextureType_NORMALS, "Normals"},
        {aiTextureType_SHININESS, "Shininess"},
        {aiTextureType_OPACITY, "Opacity"},
        {aiTextureType_DISPLACEMENT, "Displacement"},
        {aiTextureType_LIGHTMAP, "Lightmap"},
        {aiTextureType_REFLECTION, "Reflection"},
        {aiTextureType_UNKNOWN, "Unknown"}
    };

    auto it = textureTypeNames.find(type);
    if (it != textureTypeNames.end()) {
        return it->second;
    }
    else {
        return "Invalid Type";
    }
}


static int GetTextureCount(const aiMaterial* pMaterial)
{
    int TextureCount = 0;

    for (int i = 0; i <= aiTextureType_UNKNOWN; ++i) { // UNKNOWN is the last texture type in Assimp.
        aiTextureType ttype = (aiTextureType)(i);
        int Count = pMaterial->GetTextureCount(ttype);
        TextureCount += Count;

        if (Count > 0) {
            printf("Found texture %s\n", TextureTypeToString(ttype).c_str());
        }
    }

    return TextureCount;
}


void CoreModel::LoadTextures(const string& Dir, const aiMaterial* pMaterial, int index)
{
    int TextureCount = GetTextureCount(pMaterial);

    printf("Number of textures %d\n", TextureCount);

    LoadDiffuseTexture(Dir, pMaterial, index);
    LoadSpecularTexture(Dir, pMaterial, index);
    LoadNormalTexture(Dir, pMaterial, index);
}


void CoreModel::LoadDiffuseTexture(const string& Dir, const aiMaterial* pMaterial, int MaterialIndex)
{
    m_Materials[MaterialIndex].pDiffuse = NULL;

    if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
        aiString Path;

        if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
            const aiTexture* paiTexture = m_pScene->GetEmbeddedTexture(Path.C_Str());

            if (paiTexture) {
                LoadDiffuseTextureEmbedded(paiTexture, MaterialIndex);
            }
            else {
                LoadDiffuseTextureFromFile(Dir, Path, MaterialIndex);
            }
        }
    }
    else {
        printf("Warning! no diffuse texture\n");

        if (!s_pMissingTexture) {
            printf("Loading default texture\n");
            s_pMissingTexture = AllocTexture2D();

            s_pMissingTexture->Load("../Assets/Textures/no_texture.png");
        }

        m_Materials[MaterialIndex].pDiffuse = s_pMissingTexture;
    }
}


void CoreModel::LoadDiffuseTextureEmbedded(const aiTexture* paiTexture, int MaterialIndex)
{
    printf("Embeddeded diffuse texture type '%s'\n", paiTexture->achFormatHint);
    m_Materials[MaterialIndex].pDiffuse = AllocTexture2D();
    int buffer_size = paiTexture->mWidth;   // TODO: just the width???
    m_Materials[MaterialIndex].pDiffuse->Load(buffer_size, paiTexture->pcData);
}


void CoreModel::LoadDiffuseTextureFromFile(const string& Dir, const aiString& Path, int MaterialIndex)
{
    string p(Path.data);

    for (int i = 0; i < p.length(); i++) {
        if (p[i] == '\\') {
            p[i] = '/';
        }
    }

    if (p.substr(0, 2) == ".\\") {
        p = p.substr(2, p.size() - 2);
    }

    string FullPath = Dir + "/" + p;

    m_Materials[MaterialIndex].pDiffuse = AllocTexture2D();

    m_Materials[MaterialIndex].pDiffuse->Load(FullPath.c_str());
    printf("Loaded diffuse texture '%s' at index %d\n", FullPath.c_str(), MaterialIndex);
}


void CoreModel::LoadSpecularTexture(const string& Dir, const aiMaterial* pMaterial, int MaterialIndex)
{
    m_Materials[MaterialIndex].pSpecularExponent = NULL;

    if (pMaterial->GetTextureCount(aiTextureType_SHININESS) > 0) {
        aiString Path;

        if (pMaterial->GetTexture(aiTextureType_SHININESS, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
            const aiTexture* paiTexture = m_pScene->GetEmbeddedTexture(Path.C_Str());

            if (paiTexture) {
                LoadSpecularTextureEmbedded(paiTexture, MaterialIndex);
            }
            else {
                LoadSpecularTextureFromFile(Dir, Path, MaterialIndex);
            }
        }
    }
}


void CoreModel::LoadSpecularTextureEmbedded(const aiTexture* paiTexture, int MaterialIndex)
{
    printf("Embeddeded specular texture type '%s'\n", paiTexture->achFormatHint);
    m_Materials[MaterialIndex].pSpecularExponent = AllocTexture2D();
    int buffer_size = paiTexture->mWidth;   // TODO: just the width???
    m_Materials[MaterialIndex].pSpecularExponent->Load(buffer_size, paiTexture->pcData);
}


void CoreModel::LoadSpecularTextureFromFile(const string& Dir, const aiString& Path, int MaterialIndex)
{
    string p(Path.data);

    if (p == "C:\\\\") {
        p = "";
    }
    else if (p.substr(0, 2) == ".\\") {
        p = p.substr(2, p.size() - 2);
    }

    string FullPath = Dir + "/" + p;

    m_Materials[MaterialIndex].pSpecularExponent = AllocTexture2D();

    m_Materials[MaterialIndex].pSpecularExponent->Load(FullPath.c_str());
    printf("Loaded specular texture '%s'\n", FullPath.c_str());
}


void CoreModel::LoadNormalTexture(const string& Dir, const aiMaterial* pMaterial, int MaterialIndex)
{
    m_Materials[MaterialIndex].pNormal = NULL;

    if (pMaterial->GetTextureCount(aiTextureType_NORMALS) > 0) {
        aiString Path;

        if (pMaterial->GetTexture(aiTextureType_NORMALS, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
            const aiTexture* paiTexture = m_pScene->GetEmbeddedTexture(Path.C_Str());

            if (paiTexture) {
                LoadNormalTextureEmbedded(paiTexture, MaterialIndex);
            }
            else {
                LoadNormalTextureFromFile(Dir, Path, MaterialIndex);
            }
        }
    }
}


void CoreModel::LoadNormalTextureEmbedded(const aiTexture* paiTexture, int MaterialIndex)
{
    printf("Embeddeded nroaml texture type '%s'\n", paiTexture->achFormatHint);
    m_Materials[MaterialIndex].pNormal = AllocTexture2D();
    int buffer_size = paiTexture->mWidth;   // TODO: just the width???
    m_Materials[MaterialIndex].pNormal->Load(buffer_size, paiTexture->pcData);
}


void CoreModel::LoadNormalTextureFromFile(const string& Dir, const aiString& Path, int MaterialIndex)
{
    string p(Path.data);

    if (p == "C:\\\\") {
        p = "";
    }
    else if (p.substr(0, 2) == ".\\") {
        p = p.substr(2, p.size() - 2);
    }

    string FullPath = Dir + "/" + p;

    m_Materials[MaterialIndex].pNormal = AllocTexture2D();

    m_Materials[MaterialIndex].pNormal->Load(FullPath.c_str());
    printf("Loaded normal texture '%s'\n", FullPath.c_str());
}


void CoreModel::LoadColors(const aiMaterial* pMaterial, int index)
{
    Material& material = m_Materials[index];

    material.m_name = pMaterial->GetName().C_Str();

    glm::vec4 AllOnes(1.0f, 1.0f, 1.0f, 1.0f);

    int ShadingModel = 0;
    if (pMaterial->Get(AI_MATKEY_SHADING_MODEL, ShadingModel) == AI_SUCCESS) {
        //  printf("Shading model %d\n", ShadingModel);
    }

    aiColor4D AmbientColor(0.0f, 0.0f, 0.0f, 0.0f);

    if (pMaterial->Get(AI_MATKEY_COLOR_AMBIENT, AmbientColor) == AI_SUCCESS) {
        printf("Loaded ambient color [%f %f %f]\n", AmbientColor.r, AmbientColor.g, AmbientColor.b);
        material.AmbientColor.r = AmbientColor.r;
        material.AmbientColor.g = AmbientColor.g;
        material.AmbientColor.b = AmbientColor.b;
        material.AmbientColor.a = std::min(AmbientColor.a, 1.0f);
    }
    else {
        material.AmbientColor = AllOnes;
    }

    aiColor4D EmissiveColor(0.0f, 0.0f, 0.0f, 0.0f);

    if (pMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, EmissiveColor) == AI_SUCCESS) {
        printf("Loaded emissive color [%f %f %f]\n", EmissiveColor.r, EmissiveColor.g, EmissiveColor.b);
        material.AmbientColor.r += EmissiveColor.r;
        material.AmbientColor.g += EmissiveColor.g;
        material.AmbientColor.b += EmissiveColor.b;
        material.AmbientColor.a += EmissiveColor.a;
        material.AmbientColor.a = std::min(material.AmbientColor.a, 1.0f);
    }
    else {
        material.AmbientColor = AllOnes;
    }

    aiColor4D DiffuseColor(0.0f, 0.0f, 0.0f, 0.0f);

    if (pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, DiffuseColor) == AI_SUCCESS) {
        printf("Loaded diffuse color [%f %f %f]\n", DiffuseColor.r, DiffuseColor.g, DiffuseColor.b);
        material.DiffuseColor.r = DiffuseColor.r;
        material.DiffuseColor.g = DiffuseColor.g;
        material.DiffuseColor.b = DiffuseColor.b;
        material.DiffuseColor.a = std::min(DiffuseColor.a, 1.0f);
    }

    aiColor4D SpecularColor(0.0f, 0.0f, 0.0f, 0.0f);

    if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, SpecularColor) == AI_SUCCESS) {
        printf("Loaded specular color [%f %f %f]\n", SpecularColor.r, SpecularColor.g, SpecularColor.b);
        material.SpecularColor.r = SpecularColor.r;
        material.SpecularColor.g = SpecularColor.g;
        material.SpecularColor.b = SpecularColor.b;
        material.SpecularColor.a = std::min(SpecularColor.a, 1.0f);
    }

    float OpaquenessThreshold = 0.05f;
    float Opacity = 1.0f;

    if (pMaterial->Get(AI_MATKEY_OPACITY, Opacity) == AI_SUCCESS) {
        material.m_transparencyFactor = CLAMP(1.0f - Opacity, 0.0f, 1.0f);
        if (material.m_transparencyFactor >= 1.0f - OpaquenessThreshold) {
            material.m_transparencyFactor = 0.0f;
        }
    }

    aiColor4D TransparentColor;
    if (pMaterial->Get(AI_MATKEY_COLOR_TRANSPARENT, TransparentColor) == AI_SUCCESS) {
        float Opacity = std::max(std::max(TransparentColor.r, TransparentColor.g), TransparentColor.b);
        material.m_transparencyFactor = CLAMP(Opacity, 0.0f, 1.0f);
        if (material.m_transparencyFactor >= 1.0f - OpaquenessThreshold) {
            material.m_transparencyFactor = 0.0f;
        }

        material.m_alphaTest = 0.5f;
    }
}




static void traverse(int depth, aiNode* pNode)
{
    for (int i = 0; i < depth; i++) {
        printf(" ");
    }

    printf("%s\n", pNode->mName.C_Str());

    for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
        traverse(depth + 1, pNode->mChildren[i]);
    }
}


void CoreModel::InitCameras(const aiScene* pScene)
{
    printf("\n*** Initializing cameras ***\n");
    printf("Loading %d cameras\n", pScene->mNumCameras);

    m_cameras.resize(pScene->mNumCameras);

    for (unsigned int i = 0; i < pScene->mNumCameras; i++) {
        InitSingleCamera(i, pScene);
    }
}


void CoreModel::InitSingleCamera(int Index, const aiScene* pScene)
{
    const aiCamera* pCamera = pScene->mCameras[Index];
    printf("Camera name: '%s'\n", pCamera->mName.C_Str());

    glm::mat4 Transformation;
    GetFullTransformation(pScene->mRootNode, pCamera->mName.C_Str(), Transformation);

    glm::vec3 Pos = VectorFromAssimpVector(pCamera->mPosition);
    glm::vec3 Target = VectorFromAssimpVector(pCamera->mLookAt);
    glm::vec3 Up = VectorFromAssimpVector(pCamera->mUp);

    glm::vec4 Pos4D(Pos, 1.0f);
    Pos4D = Transformation * Pos4D;
    glm::vec3 FinalPos(Pos4D);

    glm::vec4 Target4D(Target, 0.0f);
    Target4D = Transformation * Target4D;
    glm::vec3 FinalTarget(Target4D);

    glm::vec4 Up4D(Up, 0.0f);
    Up4D = Transformation * Up4D;
    glm::vec3 FinalUp(Up4D);

    PersProjInfo persProjInfo;
    persProjInfo.zNear = pCamera->mClipPlaneNear;
    persProjInfo.zFar = pCamera->mClipPlaneFar;
    persProjInfo.Width = pCamera->mAspect;
    persProjInfo.Height = 1.0f;
    persProjInfo.FOV = ToDegree(pCamera->mHorizontalFOV) / 2.0f;

    glm::vec3 Center = FinalPos + FinalTarget;
    m_cameras[Index].Init(FinalPos, persProjInfo);
}


void CoreModel::InitLights(const aiScene* pScene)
{
    printf("\n*** Initializing lights ***\n");

    for (int i = 0; i < (int)pScene->mNumLights; i++) {
        InitSingleLight(pScene, *pScene->mLights[i]);
    }
}


void CoreModel::InitSingleLight(const aiScene* pScene, const aiLight& light)
{
    printf("Init light '%s'\n", light.mName.C_Str());

    switch (light.mType) {

    case aiLightSource_DIRECTIONAL:
        InitDirectionalLight(pScene, light);
        break;

    case aiLightSource_POINT:
        InitPointLight(pScene, light);
        break;

    case aiLightSource_SPOT:
        InitSpotLight(pScene, light);
        break;

    case aiLightSource_AMBIENT:
        printf("Ambient light is not implemented\n");
        exit(0);

    case aiLightSource_AREA:
        printf("Area light is not implemented\n");
        exit(0);

    case aiLightSource_UNDEFINED:
    default:
        printf("Light type is undefined\n");
        exit(0);
    }
}


static bool GetFullTransformation(const aiNode* pRootNode, const char* pName, glm::mat4& Transformation)
{
    Transformation = glm::mat4(1.0f);

    const aiNode* pNode = pRootNode->FindNode(pName);

    if (!pNode) {
        printf("Warning! Cannot find a node for '%s'\n", pName);
        return false;
    }

    while (pNode) {
        glm::mat4 NodeTransformation = AssimpToGLM(pNode->mTransformation);
        Transformation = NodeTransformation * Transformation;
        pNode = pNode->mParent;
    }

    return true;
}


void CoreModel::InitDirectionalLight(const aiScene* pScene, const aiLight& light)
{
    if (m_dirLights.size() > 0) {
        printf("The lighting shader currently supports only a single directional light!\n");
        exit(0);
    }

    DirectionalLight l;
    l.Color = glm::vec3(1.0f);
    l.DiffuseIntensity = 1.0f;

    glm::mat4 Transformation;
    GetFullTransformation(pScene->mRootNode, light.mName.C_Str(), Transformation);

    glm::vec3 Direction = VectorFromAssimpVector(light.mDirection);
    glm::vec4 Dir4D(Direction, 0.0f);
    Dir4D = Transformation * Dir4D;
    l.WorldDirection = glm::vec3(Dir4D);

    glm::vec3 Up = VectorFromAssimpVector(light.mUp);
    glm::vec4 Up4D(Up, 0.0f);
    Up4D = Transformation * Up4D;
    l.Up = glm::vec3(Up4D);

    m_dirLights.push_back(l);
}


void CoreModel::InitPointLight(const aiScene* pScene, const aiLight& light)
{
    PointLight l;
    l.Color = glm::vec3(light.mColorDiffuse.r, light.mColorDiffuse.g, light.mColorDiffuse.b);
    l.DiffuseIntensity = 1.0f;

    glm::vec3 Position = VectorFromAssimpVector(light.mPosition);

    glm::mat4 Transformation;

    GetFullTransformation(pScene->mRootNode, light.mName.C_Str(), Transformation);

    glm::vec4 Pos4D(Position, 1.0f);
    Pos4D = Transformation * Pos4D;
    glm::vec3 WorldPosition(Pos4D);
    l.WorldPosition = WorldPosition;

    l.Attenuation.Constant = light.mAttenuationConstant;
    l.Attenuation.Linear = light.mAttenuationLinear;
    l.Attenuation.Exp = light.mAttenuationQuadratic;

    printf("Attenuation: constant %f linear %f exp %f\n", l.Attenuation.Constant, l.Attenuation.Linear, l.Attenuation.Exp);

    m_pointLights.push_back(l);
}


void CoreModel::InitSpotLight(const aiScene* pScene, const aiLight& light)
{
    SpotLight l;
    l.Color = glm::vec3(1.0f);
    l.DiffuseIntensity = 1.0f;

    glm::mat4 Transformation;
    GetFullTransformation(pScene->mRootNode, light.mName.C_Str(), Transformation);

    glm::vec3 Direction = VectorFromAssimpVector(light.mDirection);
    glm::vec4 Dir4D(Direction, 0.0f);
    Dir4D = Transformation * Dir4D;
    l.WorldDirection = glm::vec3(Dir4D);

    glm::vec3 Up = VectorFromAssimpVector(light.mUp);
    if (glm::length(Up) == 0) {
        printf("Overiding a zero up vector\n");
        if ((Dir4D == glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)) || (Dir4D == glm::vec4(0.0f, -1.0f, 0.0f, 0.0f))) {
            l.Up = glm::vec3(1.0f, 0.0f, 0.0f);
        }
        else {
            l.Up = glm::vec3(0.0f, 1.0f, 0.0f);
        }
    }
    else {
        glm::vec4 Up4D(Up, 0.0f);
        Up4D = Transformation * Up4D;
        l.Up = glm::vec3(Up4D);
    }

    glm::vec3 Position = VectorFromAssimpVector(light.mPosition);
    glm::vec4 Pos4D(Position, 1.0f);
    Pos4D = Transformation * Pos4D;
    glm::vec3 WorldPosition(Pos4D);
    l.WorldPosition = WorldPosition;

    l.Attenuation.Constant = light.mAttenuationConstant;
    l.Attenuation.Linear = light.mAttenuationLinear;
    l.Attenuation.Exp = light.mAttenuationQuadratic / 100.0f;

    printf("Attenuation: constant %f linear %f exp (adjusted!) %f\n", l.Attenuation.Constant, l.Attenuation.Linear, l.Attenuation.Exp);

    if (light.mAngleInnerCone != light.mAngleOuterCone) {
        printf("Warning!!! Different values for spot light inner/outer cone angles is not supported\n");
    }

    l.Cutoff = ToDegree(light.mAngleOuterCone);

    printf("Cutoff angle %f\n", l.Cutoff);

    m_spotLights.push_back(l);
}


void CoreModel::LoadMeshBones(vector<SkinnedVertex>& SkinnedVertices, unsigned int MeshIndex, const aiMesh* pMesh)
{
    if (pMesh->mNumBones > MAX_BONES) {
        printf("The number of bones in the model (%d) is larger than the maximum supported (%d)\n", pMesh->mNumBones, MAX_BONES);
        printf("Make sure to increase the macro MAX_BONES in the C++ header as well as in the shader to the same value\n");
        assert(0);
    }

    // printf("Loading mesh bones %d\n", MeshIndex);
    for (unsigned int i = 0; i < pMesh->mNumBones; i++) {
        // printf("Bone %d %s\n", i, pMesh->mBones[i]->mName.C_Str());
        LoadSingleBone(SkinnedVertices, MeshIndex, pMesh->mBones[i]);
    }
}


void CoreModel::LoadSingleBone(vector<SkinnedVertex>& SkinnedVertices, unsigned int MeshIndex, const aiBone* pBone)
{
    int BoneId = GetBoneId(pBone);

    if (BoneId == m_BoneInfo.size()) {
        BoneInfo bi(AssimpToGLM(pBone->mOffsetMatrix));
        m_BoneInfo.push_back(bi);
    }

    for (unsigned int i = 0; i < pBone->mNumWeights; i++) {
        const aiVertexWeight& vw = pBone->mWeights[i];
        unsigned int GlobalVertexID = m_Meshes[MeshIndex].BaseVertex + pBone->mWeights[i].mVertexId;
        SkinnedVertices[GlobalVertexID].Bones.AddBoneData(BoneId, vw.mWeight);
    }

    MarkRequiredNodesForBone(pBone);
}


void CoreModel::MarkRequiredNodesForBone(const aiBone* pBone)
{
    string NodeName(pBone->mName.C_Str());

    const aiNode* pParent = NULL;

    do {
        map<string, NodeInfo>::iterator it = m_requiredNodeMap.find(NodeName);

        if (it == m_requiredNodeMap.end()) {
            printf("Cannot find bone %s in the hierarchy\n", NodeName.c_str());
            assert(0);
        }

        it->second.isRequired = true;

        pParent = it->second.pNode->mParent;

        if (pParent) {
            NodeName = string(pParent->mName.C_Str());
        }

    } while (pParent);
}


void CoreModel::InitializeRequiredNodeMap(const aiNode* pNode)
{
    string NodeName(pNode->mName.C_Str());

    NodeInfo info(pNode);

    m_requiredNodeMap[NodeName] = info;

    for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
        InitializeRequiredNodeMap(pNode->mChildren[i]);
    }
}


int CoreModel::GetBoneId(const aiBone* pBone)
{
    int BoneIndex = 0;
    string BoneName(pBone->mName.C_Str());

    if (m_BoneNameToIndexMap.find(BoneName) == m_BoneNameToIndexMap.end()) {
        // Allocate an index for a new bone
        BoneIndex = (int)m_BoneNameToIndexMap.size();
        m_BoneNameToIndexMap[BoneName] = BoneIndex;
    }
    else {
        BoneIndex = m_BoneNameToIndexMap[BoneName];
    }

    return BoneIndex;
}


unsigned int CoreModel::FindPosition(float AnimationTimeTicks, const aiNodeAnim* pNodeAnim)
{
    for (unsigned int i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++) {
        float t = (float)pNodeAnim->mPositionKeys[i + 1].mTime;
        if (AnimationTimeTicks < t) {
            return i;
        }
    }

    return 0;
}


void CoreModel::CalcInterpolatedPosition(aiVector3D& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim)
{
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumPositionKeys == 1) {
        Out = pNodeAnim->mPositionKeys[0].mValue;
        return;
    }

    unsigned int PositionIndex = FindPosition(AnimationTimeTicks, pNodeAnim);
    unsigned int NextPositionIndex = PositionIndex + 1;
    assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
    float t1 = (float)pNodeAnim->mPositionKeys[PositionIndex].mTime;
    if (t1 > AnimationTimeTicks) {
        Out = pNodeAnim->mPositionKeys[PositionIndex].mValue;
    }
    else {
        float t2 = (float)pNodeAnim->mPositionKeys[NextPositionIndex].mTime;
        float DeltaTime = t2 - t1;
        float Factor = (AnimationTimeTicks - t1) / DeltaTime;
        assert(Factor >= 0.0f && Factor <= 1.0f);
        const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
        const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
        aiVector3D Delta = End - Start;
        Out = Start + Factor * Delta;
    }
}


unsigned int CoreModel::FindRotation(float AnimationTimeTicks, const aiNodeAnim* pNodeAnim)
{
    assert(pNodeAnim->mNumRotationKeys > 0);

    for (unsigned int i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++) {
        float t = (float)pNodeAnim->mRotationKeys[i + 1].mTime;
        if (AnimationTimeTicks < t) {
            return i;
        }
    }

    return 0;
}


void CoreModel::CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim)
{
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumRotationKeys == 1) {
        Out = pNodeAnim->mRotationKeys[0].mValue;
        return;
    }

    unsigned int RotationIndex = FindRotation(AnimationTimeTicks, pNodeAnim);
    unsigned int NextRotationIndex = RotationIndex + 1;
    assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
    float t1 = (float)pNodeAnim->mRotationKeys[RotationIndex].mTime;
    if (t1 > AnimationTimeTicks) {
        Out = pNodeAnim->mRotationKeys[RotationIndex].mValue;
    }
    else {
        float t2 = (float)pNodeAnim->mRotationKeys[NextRotationIndex].mTime;
        float DeltaTime = t2 - t1;
        float Factor = (AnimationTimeTicks - t1) / DeltaTime;
        assert(Factor >= 0.0f && Factor <= 1.0f);
        const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
        const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
        aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
    }

    Out.Normalize();
}


unsigned int CoreModel::FindScaling(float AnimationTimeTicks, const aiNodeAnim* pNodeAnim)
{
    assert(pNodeAnim->mNumScalingKeys > 0);

    for (unsigned int i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++) {
        float t = (float)pNodeAnim->mScalingKeys[i + 1].mTime;
        if (AnimationTimeTicks < t) {
            return i;
        }
    }

    return 0;
}


void CoreModel::CalcInterpolatedScaling(aiVector3D& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim)
{
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumScalingKeys == 1) {
        Out = pNodeAnim->mScalingKeys[0].mValue;
        return;
    }

    unsigned int ScalingIndex = FindScaling(AnimationTimeTicks, pNodeAnim);
    unsigned int NextScalingIndex = ScalingIndex + 1;
    assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
    float t1 = (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime;
    if (t1 > AnimationTimeTicks) {
        Out = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
    }
    else {
        float t2 = (float)pNodeAnim->mScalingKeys[NextScalingIndex].mTime;
        float DeltaTime = t2 - t1;
        float Factor = (AnimationTimeTicks - (float)t1) / DeltaTime;
        assert(Factor >= 0.0f && Factor <= 1.0f);
        const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
        const aiVector3D& End = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
        aiVector3D Delta = End - Start;
        Out = Start + Factor * Delta;
    }
}


void CoreModel::ReadNodeHierarchy(float AnimationTimeTicks, const aiNode* pNode, const glm::mat4& ParentTransform, const aiAnimation& Animation)
{
    string NodeName(pNode->mName.data);

    glm::mat4 NodeTransformation = AssimpToGLM(pNode->mTransformation);

    const aiNodeAnim* pNodeAnim = FindNodeAnim(Animation, NodeName);

    if (pNodeAnim) {
        LocalTransform Transform;
        CalcLocalTransform(Transform, AnimationTimeTicks, pNodeAnim);

        glm::mat4 ScalingM = glm::scale(glm::mat4(1.0f), glm::vec3(Transform.Scaling.x, Transform.Scaling.y, Transform.Scaling.z));
        glm::mat4 RotationM = glm::mat4_cast(glm::quat(Transform.Rotation.w, Transform.Rotation.x, Transform.Rotation.y, Transform.Rotation.z));
        glm::mat4 TranslationM = glm::translate(glm::mat4(1.0f), glm::vec3(Transform.Translation.x, Transform.Translation.y, Transform.Translation.z));

        NodeTransformation = TranslationM * RotationM * ScalingM;
    }

    glm::mat4 GlobalTransformation = ParentTransform * NodeTransformation;

    if (m_BoneNameToIndexMap.find(NodeName) != m_BoneNameToIndexMap.end()) {
        unsigned int BoneIndex = m_BoneNameToIndexMap[NodeName];
        m_BoneInfo[BoneIndex].FinalTransformation = m_GlobalInverseTransform * GlobalTransformation * m_BoneInfo[BoneIndex].OffsetMatrix;
    }

    for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
        string ChildName(pNode->mChildren[i]->mName.data);

        map<string, NodeInfo>::iterator it = m_requiredNodeMap.find(ChildName);

        if (it == m_requiredNodeMap.end()) {
            printf("Child %s cannot be found in the required node map\n", ChildName.c_str());
            assert(0);
        }

        if (it->second.isRequired) {
            ReadNodeHierarchy(AnimationTimeTicks, pNode->mChildren[i], GlobalTransformation, Animation);
        }
    }
}


void CoreModel::ReadNodeHierarchyBlended(float StartAnimationTimeTicks, float EndAnimationTimeTicks, const aiNode* pNode, const glm::mat4& ParentTransform,
    const aiAnimation& StartAnimation, const aiAnimation& EndAnimation, float BlendFactor)
{
    string NodeName(pNode->mName.data);

    glm::mat4 NodeTransformation = AssimpToGLM(pNode->mTransformation);

    const aiNodeAnim* pStartNodeAnim = FindNodeAnim(StartAnimation, NodeName);

    LocalTransform StartTransform;

    if (pStartNodeAnim) {
        CalcLocalTransform(StartTransform, StartAnimationTimeTicks, pStartNodeAnim);
    }

    LocalTransform EndTransform;

    const aiNodeAnim* pEndNodeAnim = FindNodeAnim(EndAnimation, NodeName);

    if ((pStartNodeAnim && !pEndNodeAnim) || (!pStartNodeAnim && pEndNodeAnim)) {
        printf("On the node %s there is an animation node for only one of the start/end animations.\n", NodeName.c_str());
        printf("This case is not supported\n");
        exit(0);
    }

    if (pEndNodeAnim) {
        CalcLocalTransform(EndTransform, EndAnimationTimeTicks, pEndNodeAnim);
    }

    if (pStartNodeAnim && pEndNodeAnim) {
        const aiVector3D& Scale0 = StartTransform.Scaling;
        const aiVector3D& Scale1 = EndTransform.Scaling;
        aiVector3D BlendedScaling = (1.0f - BlendFactor) * Scale0 + Scale1 * BlendFactor;
        glm::mat4 ScalingM = glm::scale(glm::mat4(1.0f), glm::vec3(BlendedScaling.x, BlendedScaling.y, BlendedScaling.z));

        const aiQuaternion& Rot0 = StartTransform.Rotation;
        const aiQuaternion& Rot1 = EndTransform.Rotation;
        aiQuaternion BlendedRot;
        aiQuaternion::Interpolate(BlendedRot, Rot0, Rot1, BlendFactor);
        glm::mat4 RotationM = glm::mat4_cast(glm::quat(BlendedRot.w, BlendedRot.x, BlendedRot.y, BlendedRot.z));

        const aiVector3D& Pos0 = StartTransform.Translation;
        const aiVector3D& Pos1 = EndTransform.Translation;
        aiVector3D BlendedTranslation = (1.0f - BlendFactor) * Pos0 + Pos1 * BlendFactor;
        glm::mat4 TranslationM = glm::translate(glm::mat4(1.0f), glm::vec3(BlendedTranslation.x, BlendedTranslation.y, BlendedTranslation.z));

        NodeTransformation = TranslationM * RotationM * ScalingM;
    }

    glm::mat4 GlobalTransformation = ParentTransform * NodeTransformation;

    if (m_BoneNameToIndexMap.find(NodeName) != m_BoneNameToIndexMap.end()) {
        unsigned int BoneIndex = m_BoneNameToIndexMap[NodeName];
        m_BoneInfo[BoneIndex].FinalTransformation = m_GlobalInverseTransform * GlobalTransformation * m_BoneInfo[BoneIndex].OffsetMatrix;
    }

    for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
        string ChildName(pNode->mChildren[i]->mName.data);

        map<string, NodeInfo>::iterator it = m_requiredNodeMap.find(ChildName);

        if (it == m_requiredNodeMap.end()) {
            printf("Child %s cannot be found in the required node map\n", ChildName.c_str());
            assert(0);
        }

        if (it->second.isRequired) {
            ReadNodeHierarchyBlended(StartAnimationTimeTicks, EndAnimationTimeTicks,
                pNode->mChildren[i], GlobalTransformation, StartAnimation, EndAnimation, BlendFactor);
        }
    }
}


void CoreModel::CalcLocalTransform(LocalTransform& Transform, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim)
{
    CalcInterpolatedScaling(Transform.Scaling, AnimationTimeTicks, pNodeAnim);
    CalcInterpolatedRotation(Transform.Rotation, AnimationTimeTicks, pNodeAnim);
    CalcInterpolatedPosition(Transform.Translation, AnimationTimeTicks, pNodeAnim);
}


void CoreModel::GetBoneTransforms(float TimeInSeconds, vector<glm::mat4>& Transforms, unsigned int AnimationIndex)
{
    if (AnimationIndex >= m_pScene->mNumAnimations) {
        printf("Invalid animation index %d, max is %d\n", AnimationIndex, m_pScene->mNumAnimations);
        assert(0);
    }

    glm::mat4 Identity(1.0f);

    float AnimationTimeTicks = CalcAnimationTimeTicks(TimeInSeconds, AnimationIndex);
    const aiAnimation& Animation = *m_pScene->mAnimations[AnimationIndex];

    ReadNodeHierarchy(AnimationTimeTicks, m_pScene->mRootNode, Identity, Animation);
    Transforms.resize(m_BoneInfo.size());

    for (unsigned int i = 0; i < m_BoneInfo.size(); i++) {
        Transforms[i] = m_BoneInfo[i].FinalTransformation;
    }
}


void CoreModel::GetBoneTransformsBlended(float TimeInSeconds,
    vector<glm::mat4>& BlendedTransforms,
    unsigned int StartAnimIndex,
    unsigned int EndAnimIndex,
    float BlendFactor)
{
    if (StartAnimIndex >= m_pScene->mNumAnimations) {
        printf("Invalid start animation index %d, max is %d\n", StartAnimIndex, m_pScene->mNumAnimations);
        assert(0);
    }

    if (EndAnimIndex >= m_pScene->mNumAnimations) {
        printf("Invalid end animation index %d, max is %d\n", EndAnimIndex, m_pScene->mNumAnimations);
        assert(0);
    }

    if ((BlendFactor < 0.0f) || (BlendFactor > 1.0f)) {
        printf("Invalid blend factor %f\n", BlendFactor);
        assert(0);
    }

    float StartAnimationTimeTicks = CalcAnimationTimeTicks(TimeInSeconds, StartAnimIndex);
    float EndAnimationTimeTicks = CalcAnimationTimeTicks(TimeInSeconds, EndAnimIndex);

    const aiAnimation& StartAnimation = *m_pScene->mAnimations[StartAnimIndex];
    const aiAnimation& EndAnimation = *m_pScene->mAnimations[EndAnimIndex];

    glm::mat4 Identity(1.0f);

    ReadNodeHierarchyBlended(StartAnimationTimeTicks, EndAnimationTimeTicks, m_pScene->mRootNode, Identity, StartAnimation, EndAnimation, BlendFactor);

    BlendedTransforms.resize(m_BoneInfo.size());

    for (unsigned int i = 0; i < m_BoneInfo.size(); i++) {
        BlendedTransforms[i] = m_BoneInfo[i].FinalTransformation;
    }
}


float CoreModel::CalcAnimationTimeTicks(float TimeInSeconds, unsigned int AnimationIndex)
{
    float TicksPerSecond = (float)(m_pScene->mAnimations[AnimationIndex]->mTicksPerSecond != 0 ? m_pScene->mAnimations[AnimationIndex]->mTicksPerSecond : 25.0f);
    float TimeInTicks = TimeInSeconds * TicksPerSecond;
    // we need to use the integral part of mDuration for the total length of the animation
    float Duration = 0.0f;
    float fraction = modf((float)m_pScene->mAnimations[AnimationIndex]->mDuration, &Duration);
    float AnimationTimeTicks = fmod(TimeInTicks, Duration);
    return AnimationTimeTicks;
}


const aiNodeAnim* CoreModel::FindNodeAnim(const aiAnimation&
    Animation, const string& NodeName)
{
    for (unsigned int i = 0; i < Animation.mNumChannels; i++) {
        const aiNodeAnim* pNodeAnim = Animation.mChannels[i];

        if (string(pNodeAnim->mNodeName.data) == NodeName) {
            return pNodeAnim;
        }
    }

    return NULL;
}


bool CoreModel::IsAnimated() const
{
    bool ret = m_pScene->mNumAnimations > 0;

    if (ret && (NumBones() == 0)) {
        printf("Animations without bones? need to check this\n");
        assert(0);
    }

    return ret;
}