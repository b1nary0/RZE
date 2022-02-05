#pragma once

#include <Rendering/ShaderHandle.h>

#include <Utils/Math/Vector2D.h>

class MeshGeometry;

class Vector2D;

class RenderEngine
{
public:
	RenderEngine();
	~RenderEngine();

	void Initialize(void* windowHandle);
	void Update();
	void Render();
	void Shutdown();
	
	void ResizeCanvas(const Vector2D& newSize);

private:
	Vector2D m_canvasSize;

	// @TODO temp until ShaderTechniques are properly implemented
	// (could be a while)
	Rendering::VertexShaderHandle m_vertexShader;
};