#include "StdAfx.h"

#include "Game/Components/RenderComponent.h"

RenderComponent::RenderComponent()
	: mShaderGroup(nullptr)
{
}

RenderComponent::~RenderComponent()
{
}

const std::vector<float>& RenderComponent::GetVertexList() const
{
	return mVertexList;
}

void RenderComponent::SetVertexList(const std::vector<float>& vertexList)
{
	mVertexList = vertexList;
}

const GFXShaderGroup* const RenderComponent::GetShaderGroup() const
{
	return mShaderGroup;
}

void RenderComponent::SetShaderGroup(GFXShaderGroup* const shaderGroup)
{
	mShaderGroup = shaderGroup;
}
