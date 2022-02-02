#pragma once

#include <Utils/PrimitiveDefs.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>

#include <memory>
#include <vector>

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
	ResourceHandle m_vertexShader;
};