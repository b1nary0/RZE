#pragma once

#include <Game/ECS/EntityComponent.h>

class GFXShaderGroup;

class MeshComponent : public IEntityComponent
{
public:
	std::vector<float>& GetVertexList();
	void SetVertexList(const std::vector<float>& vertexList);

	GFXShaderGroup* const GetShaderGroup();
	void SetShaderGroup(GFXShaderGroup* const shaderGroup);

private:
	GFXShaderGroup* mShaderGroup;
	std::vector<float> mVertexList;
};
