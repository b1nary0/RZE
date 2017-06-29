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

class GFXMesh
{
    friend class MeshData;

public:
    GFXMesh();
    ~GFXMesh();

private:
    std::vector<GFXVertex> mVertices;
    std::vector<int> mIndices;
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

private:
    void ProcessNode(const aiNode& node, const aiScene& scene);
    void ProcessMesh(const aiMesh& mesh, const aiScene& scene, GFXMesh& outMesh);

    std::vector<GFXMesh*> mMeshList;
};
