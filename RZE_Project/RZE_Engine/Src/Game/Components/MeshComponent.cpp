#include <StdAfx.h>
#include <Game/Components/MeshComponent.h>

#include <RenderCore/HardwareInterface/OpenGL.h>

MeshComponent::MeshComponent()
{
	OpenGLRHI& RHI = OpenGLRHI::Get();
	RHI.GenVertexArrays(1, &mVAO);

	mVBO = new OpenGLVBO();
}

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

U32 MeshComponent::GetVAO() const
{
	return mVAO;
}

OpenGLVBO* MeshComponent::GetVBO()
{
	return mVBO;
}
