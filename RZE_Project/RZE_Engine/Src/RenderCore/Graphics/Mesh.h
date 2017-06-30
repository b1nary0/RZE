#pragma once

#include <vector>

#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

struct GFXVertex
{
    Vector3D Position;
    Vector3D Normal;
    Vector2D UVData;
};

class OpenGLVAO;
class OpenGLVBO;
class OpenGLEBO;

class GFXMesh
{
    friend class MeshData;

public:
    GFXMesh();
    ~GFXMesh();

    OpenGLVAO* GetVAO() const;
    OpenGLVBO* GetVBO();
    OpenGLEBO* GetEBO();

    std::vector<GFXVertex> GetVertexList();
    std::vector<U32> GetIndices();

private:
    void OnLoadFinished();

    OpenGLVAO* mVAO;
    OpenGLVBO* mVBO;
    OpenGLEBO* mEBO;

    std::vector<GFXVertex> mVertices;
    std::vector<U32> mIndices;
};

struct aiMesh;
struct aiNode;
struct aiScene;
class MeshData
{
public:
    MeshData();
    ~MeshData();
    
    void LoadFromFile(const std::string& filePath);

    std::vector<GFXMesh*>& GetMeshList();

private:
    void ProcessNode(const aiNode& node, const aiScene& scene);
    void ProcessMesh(const aiMesh& mesh, const aiScene& scene, GFXMesh& outMesh);
    std::vector<GFXMesh*> mMeshList;
};
