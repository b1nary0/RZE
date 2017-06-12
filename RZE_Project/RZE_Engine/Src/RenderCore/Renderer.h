#pragma once

#include <vector>

#include <RenderCore/HardwareInterface/OpenGL.h>
#include <RenderCore/SceneCamera.h>

class GFXShaderGroup;

class RZE_Renderer
{
public:
	typedef struct RenderItemProtocol
	{
		std::vector<float>* VertexList;
		GFXShaderGroup* ShaderGroup;
		Matrix4x4 ModelViewProjection;
		OpenGLVAO* VAO;
		OpenGLVBO* VBO;
	} RenderItemProtocol;

public:
	RZE_Renderer();

	void AddRenderItem(const RenderItemProtocol& itemProtocol);
	void Render();

	SceneCamera& GetSceneCamera();

private:
	void RenderSingleItem(RenderItemProtocol& itemProtocol);

private:
	SceneCamera* mSceneCamera;

	std::queue<RenderItemProtocol> mRenderList;
};
