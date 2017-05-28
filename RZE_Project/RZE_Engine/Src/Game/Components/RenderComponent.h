#pragma once

#include <vector>

#include "Game/ECS/EntityComponent.h"

class GFXShaderGroup;

class RenderComponent : public IEntityComponent
{
public:
	RenderComponent();
	virtual ~RenderComponent();

	const std::vector<float>& GetVertexList() const;
	void SetVertexList(const std::vector<float>& vertexList);

	const GFXShaderGroup* const GetShaderGroup() const;
	void SetShaderGroup(GFXShaderGroup* const shaderGroup);

private:
	// @todo come back to this cause it probably shouldn't be a raw pointer
	GFXShaderGroup* mShaderGroup;
	std::vector<float> mVertexList;
};
