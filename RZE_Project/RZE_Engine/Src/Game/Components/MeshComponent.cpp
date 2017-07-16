#include <StdAfx.h>
#include <Game/Components/MeshComponent.h>

#include <RenderCore/Graphics/Mesh.h>

#include <RenderCore/HardwareInterface/OpenGL.h>

MeshComponent::MeshComponent()
{
    SetName("MeshComponent");
}

MeshComponent::MeshComponent(const std::string& name)
{
    SetName(name);
}

MeshComponent::~MeshComponent()
{
    if (mMeshData)
    {
       delete mMeshData;
    }
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
