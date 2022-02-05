#pragma once

#include <Rendering/BufferHandle.h>
#include <Rendering/ShaderHandle.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector2D.h>

class MeshGeometry;

class Vector2D;

struct RenderCamera
{
	// @TODO integrate with viewport maybe
	Vector3D Position;
	Matrix4x4 ClipSpace;
};

class RenderEngine
{
public:
	RenderEngine();
	~RenderEngine();
	RenderEngine(const RenderEngine&) = delete;
	RenderEngine(const RenderEngine&&) = delete;

public:
	void operator=(const RenderEngine&) = delete;
	void operator=(const RenderEngine&&) = delete;

public:
	void Initialize(void* windowHandle);
	void Update();
	void Render();
	void Shutdown();
	
	void ResizeCanvas(const Vector2D& newSize);

	RenderCamera& GetCamera() { return m_camera; }

private:
	RenderCamera m_camera;

	Vector2D m_canvasSize;

	// @TODO temp until ShaderTechniques are properly implemented
	// (could be a while)
	Rendering::VertexShaderHandle m_vertexShader;
	Rendering::ConstantBufferHandle m_cameraConstantBuffer;
};