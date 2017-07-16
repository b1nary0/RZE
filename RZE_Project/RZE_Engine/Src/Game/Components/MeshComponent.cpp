#include <StdAfx.h>
#include <Game/Components/MeshComponent.h>

#include <RenderCore/Graphics/Mesh.h>

#include <RenderCore/HardwareInterface/OpenGL.h>

MeshComponent::MeshComponent()
{
    OpenGLRHI& RHI = OpenGLRHI::Get();

    const char* const meshFilePath = "./../RZE_Engine/Assets/3D/shuttle.obj";
}

MeshComponent::~MeshComponent()
{
}

std::vector<float>& MeshComponent::GetVertexList()
{
    return mVertexList;
}

void MeshComponent::SetMeshData(MeshData* const meshData)
{
    mMeshData = meshData;
}

GFXShaderGroup* const MeshComponent::GetShaderGroup()
{
    return mShaderGroup;
}

void MeshComponent::SetShaderGroup(GFXShaderGroup* const shaderGroup)
{
    mShaderGroup = shaderGroup;
}

MeshData* const MeshComponent::GetMeshData() const
{
    return mMeshData;
}
