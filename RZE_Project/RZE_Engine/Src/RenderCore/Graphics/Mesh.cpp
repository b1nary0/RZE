#include <StdAfx.h>
#include <RenderCore/Graphics/Mesh.h>

#include <RenderCore/HardwareInterface/OpenGL.h>

#include <Assimp/Importer.hpp>
#include <Assimp/postprocess.h>
#include <Assimp/scene.h>

GFXMesh::GFXMesh()
{
    mVAO = new OpenGLVAO();
    mEBO = new OpenGLEBO(mVAO);

    mVertexVBO = new OpenGLVBO(mVAO);
}

GFXMesh::~GFXMesh()
{
    if (mVAO)
    {
        delete mVAO;
    }

    if (mVertexVBO)
    {
        delete mVertexVBO;
    }

    if (mEBO)
    {
        delete mEBO;
    }
}

OpenGLVAO* GFXMesh::GetVAO() const
{
    return mVAO;
}

OpenGLVBO* GFXMesh::GetVertexVBO()
{
    return mVertexVBO;
}


OpenGLEBO* GFXMesh::GetEBO()
{
    return mEBO;
}

std::vector<GFXVertex> GFXMesh::GetVertexList()
{
    return mVertices;
}

std::vector<U32> GFXMesh::GetIndices()
{
    return mIndices;
}

void GFXMesh::OnLoadFinished()
{
    for (size_t vertIdx = 0; vertIdx < mVertices.size(); vertIdx++)
    {
        mPositions.push_back(mVertices[vertIdx].Position);
        mNormals.push_back(mVertices[vertIdx].Normal);
    }

    const GLsizeiptr verticesSize = mPositions.size() * sizeof(Vector3D);
    const GLsizeiptr normalsSize = mNormals.size() * sizeof(Vector3D);
    const GLsizeiptr totalSize = verticesSize + normalsSize;

    const GLsizeiptr normalsStart = verticesSize;

    const GLvoid* normalsStartPtr = reinterpret_cast<GLvoid*>(normalsStart);

    mVAO->Bind();
    mVertexVBO->SetBufferData(nullptr, totalSize);
    mVertexVBO->SetBufferSubData(mPositions.data(), 0, verticesSize);
    mVertexVBO->SetBufferSubData(mNormals.data(), verticesSize, normalsSize);

    mEBO->SetBufferData(mIndices.data(), sizeof(U32) * mIndices.size());

    // vertices
    OpenGLRHI::Get().EnableVertexAttributeArray(0);
    OpenGLRHI::Get().VertexAttribPointer(0, 3, EGLDataType::Float, EGLBooleanValue::False, sizeof(Vector3D), nullptr);

    // normals
    OpenGLRHI::Get().EnableVertexAttributeArray(1);
    OpenGLRHI::Get().VertexAttribPointer(1, 3, EGLDataType::Float, EGLBooleanValue::False, sizeof(Vector3D), normalsStartPtr);

    mVAO->Unbind();
}

MeshData::MeshData()
{
}

MeshData::~MeshData()
{
}

bool MeshData::Load(const std::string& filePath)
{
    Assimp::Importer ModelImporter;
    const aiScene* AssimpScene = ModelImporter.ReadFile(filePath,
                                                            aiProcess_Triangulate 
                                                        |   aiProcess_GenNormals);

    bool bAssimpNotLoaded =
        !AssimpScene 
        || !AssimpScene->mRootNode 
        || AssimpScene->mFlags == AI_SCENE_FLAGS_INCOMPLETE;

    if (bAssimpNotLoaded)
    {
        // #TODO More informative error message.
        LOG_CONSOLE("Failed to load Assimp.");
        return false;
    }

    ProcessNode(*AssimpScene->mRootNode, *AssimpScene);

    if (mMeshList.size() != AssimpScene->mNumMeshes)
    {
        // #TODO More informative error message.
        LOG_CONSOLE("Error reading meshes.");
        return false;
    }

    return true;
}

std::vector<GFXMesh*>& MeshData::GetMeshList()
{
    return mMeshList;
}

void MeshData::ProcessNode(const aiNode& node, const aiScene& scene)
{
    for (U32 meshIdx = 0; meshIdx < node.mNumMeshes; meshIdx++)
    {
        const unsigned int assimpMeshIdx = node.mMeshes[meshIdx];
        const aiMesh& assimpMesh = *scene.mMeshes[assimpMeshIdx];

        GFXMesh* Mesh = new GFXMesh();
        ProcessMesh(assimpMesh, scene, *Mesh);
        mMeshList.push_back(Mesh);
    }

    for (U32 childNodeIdx = 0; childNodeIdx < node.mNumChildren; childNodeIdx++)
    {
        ProcessNode(*node.mChildren[childNodeIdx], scene);
    }
}

void MeshData::ProcessMesh(const aiMesh& mesh, const aiScene& scene, GFXMesh& outMesh)
{
    for (U32 vertexIdx = 0; vertexIdx < mesh.mNumVertices; vertexIdx++)
    {
        const aiVector3D& assimpVert = mesh.mVertices[vertexIdx];
        const aiVector3D& assimpNormal = mesh.mNormals[vertexIdx];

        Vector3D vertPos(assimpVert.x, assimpVert.y, assimpVert.z);
        Vector3D vertNormal(assimpNormal.x, assimpNormal.y, assimpNormal.z);

        GFXVertex vertex;
        vertex.Position = vertPos;
        vertex.Normal = vertNormal;

        outMesh.mVertices.push_back(vertex);
    }

    for (U32 faceIdx = 0; faceIdx < mesh.mNumFaces; faceIdx++)
    {
        const aiFace& assimpFace = mesh.mFaces[faceIdx];
        for (U32 indexIdx = 0; indexIdx < assimpFace.mNumIndices; indexIdx++)
        {
            outMesh.mIndices.push_back(assimpFace.mIndices[indexIdx]);
        }
    }

    outMesh.OnLoadFinished();
}
