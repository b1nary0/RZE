#pragma once

#include <vector>

#include <EngineCore/Resources/Resource.h>

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
    friend class MeshResource;

public:
    GFXMesh();
    ~GFXMesh();

    OpenGLVAO* GetVAO() const;
    OpenGLEBO* GetEBO();
    OpenGLVBO* GetVertexVBO();

    std::vector<GFXVertex> GetVertexList();
    std::vector<U32> GetIndices();

private:
    void OnLoadFinished();

    OpenGLVAO* mVAO;
    OpenGLVBO* mVertexVBO;
    OpenGLVBO* mNormalVBO;
    OpenGLEBO* mEBO;

    std::vector<GFXVertex> mVertices;
    std::vector<Vector3D> mPositions;
    std::vector<Vector3D> mNormals;
    std::vector<U32> mIndices;
};

struct aiMesh;
struct aiNode;
struct aiScene;
class MeshResource : public IResource
{
public:
    MeshResource();
    ~MeshResource();
    
    virtual bool Load(const std::string& filePath) override;

    std::vector<GFXMesh*>& GetMeshList();

private:
    void ProcessNode(const aiNode& node, const aiScene& scene);
    void ProcessMesh(const aiMesh& mesh, const aiScene& scene, GFXMesh& outMesh);
    std::vector<GFXMesh*> mMeshList;
};
