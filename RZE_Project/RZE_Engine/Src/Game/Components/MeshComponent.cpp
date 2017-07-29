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
    RZE_Engine::Get()->GetResourceHandler().ReleaseResource(mResourceHandle);
}

std::vector<float>& MeshComponent::GetVertexList()
{
    return mVertexList;
}

void MeshComponent::SetResource(const ResourceHandle& resourceHandle)
{
    mResourceHandle = resourceHandle;
}

GFXShaderGroup* const MeshComponent::GetShaderGroup()
{
    return mShaderGroup;
}

void MeshComponent::SetShaderGroup(GFXShaderGroup* const shaderGroup)
{
    mShaderGroup = shaderGroup;
}

const ResourceHandle& MeshComponent::GetResource() const
{
    return mResourceHandle;
}
