#include <StdAfx.h>
#include <Game/Components/MeshComponent.h>

std::vector<float>& MeshComponent::GetVertexList()
{
	return mVertexList;
}

void MeshComponent::SetVertexList(const std::vector<float>& vertexList)
{
	mVertexList = vertexList;
}

GFXShaderGroup* const MeshComponent::GetShaderGroup()
{
	return mShaderGroup;
}

void MeshComponent::SetShaderGroup(GFXShaderGroup* const shaderGroup)
{
	mShaderGroup = shaderGroup;
}
