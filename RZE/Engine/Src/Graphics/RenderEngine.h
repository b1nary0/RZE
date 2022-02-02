#pragma once

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
};