#pragma once

#include <Game/ECS/EntityComponent.h>

class GFXShaderGroup;
class OpenGLVBO;

class MeshComponent : public IEntityComponent
{
public:
	MeshComponent();

	std::vector<float>& GetVertexList();
	void SetVertexList(const std::vector<float>& vertexList);

	GFXShaderGroup* const GetShaderGroup();
	void SetShaderGroup(GFXShaderGroup* const shaderGroup);

	U32 GetVAO() const;
	OpenGLVBO* GetVBO();

private:
	U32 mVAO;
	OpenGLVBO* mVBO;

	GFXShaderGroup* mShaderGroup;
	std::vector<float> mVertexList;
};
