#pragma once

#include <Utils/PrimitiveDefs.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>

#include <memory>
#include <vector>

class MeshGeometry;

class Vector2D;

namespace Rendering
{
	class Renderer;
	struct RenderObject;
}

class RenderProxy
{
	// #TODO is this okay?
	friend class RenderEngine;

public:
	RenderProxy() = default;
	~RenderProxy() = default;

	// #TODO Make non-copyable and non-assignable
private:
	// #TODO Eventually make it so we don't need to manage a RenderObject per mesh
	std::vector<Rendering::RenderObject*> m_renderObjects;
};

struct CameraData
{
	Vector3D Position;
	Matrix4x4 ProjectionMat;
	Matrix4x4 ViewMat;

	float FOV;
	float AspectRatio;
	float NearCull;
	float FarCull;
};

class RenderEngine
{
public:
	RenderEngine();
	~RenderEngine();

	void Initialize();
	void Update();
	void Render();
	void Shutdown();

	void SetWindow(void* windowHandle);

	void ResizeCanvas(const Vector2D& newSize);

	void SetCameraData(const CameraData& cameraData);

	RenderProxy* CreateRenderProxy(const std::vector<MeshGeometry>& meshGeometry, const Matrix4x4& transform);

	Int32 CreatePixelShader(const FilePath& filePath);

private:
	std::unique_ptr<Rendering::Renderer> m_renderer;

	// #TODO Don't store RenderProxy* - implementing like this for now
	// to move rendering responsibility flow to RenderEngine -> Rendering::Renderer
	std::vector<RenderProxy*> m_renderProxies;
};