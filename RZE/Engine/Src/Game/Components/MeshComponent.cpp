#include <StdAfx.h>
#include <Game/Components/MeshComponent.h>

#include <Diotima/Graphics/Mesh.h>
#include <Diotima/Driver/OpenGL.h>

MeshComponent::MeshComponent()
{
	SetName("MeshComponent");

	RegisterComponentID<MeshComponent>();
}

MeshComponent::MeshComponent(const std::string& name)
{
	SetName(name);
	RegisterComponentID<MeshComponent>();
}

MeshComponent::~MeshComponent()
{
	RZE_Engine::Get()->GetResourceHandler().ReleaseResource(mMeshHandle);
}

std::vector<float>& MeshComponent::GetVertexList()
{
	return mVertexList;
}

void MeshComponent::SetMeshHandle(const ResourceHandle& resourceHandle)
{
	mMeshHandle = resourceHandle;
}

const ResourceHandle& MeshComponent::GetTextureHandle() const
{
	return mTextureHandle;
}

void MeshComponent::SetTextureHandle(const ResourceHandle& resourceHandle)
{
	mTextureHandle = resourceHandle;
}

GFXShaderGroup* const MeshComponent::GetShaderGroup()
{
	return mShaderGroup;
}

void MeshComponent::SetShaderGroup(GFXShaderGroup* const shaderGroup)
{
	mShaderGroup = shaderGroup;
}

const ResourceHandle& MeshComponent::GetMeshHandle() const
{
	return mMeshHandle;
}
