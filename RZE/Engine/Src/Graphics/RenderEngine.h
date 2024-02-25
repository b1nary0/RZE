#pragma once

#include <Graphics/StaticMeshInstance.h>

#include <Graphics/GraphicsDefines.h>

#include <Rendering/BufferHandle.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>

namespace Rendering
{
	class RenderTargetTexture;
}

class IRenderStage;

struct RenderViewport
{
	Vector2D Size;
};

struct RenderCamera
{
	// @TODO integrate with viewport maybe
	Matrix4x4 ClipSpace;
	Vector3D Position;
	RenderViewport Viewport;
};

// @TODO Move to own file
class LightObject
{
public:
	struct PropertyBufferLayout
	{
		Vector3D position;
		Vector4D colour;
		float strength;
	};

public:
	LightObject() = default;
	~LightObject() = default;

	void Initialize();

	void SetPosition(const Vector3D& position) { m_data.position = position; }
	const Vector3D& GetPosition() const { return m_data.position; }

	void SetStrength(float strength) { m_data.strength = strength; }
	float GetStrength() const { return m_data.strength; }

	void SetColour(const Vector4D& colour) { m_data.colour = colour; }
	const Vector4D& GetColour() const { return m_data.colour; }

	const PropertyBufferLayout& GetData() { return m_data; }

	const Rendering::ConstantBufferHandle& GetPropertyBuffer() { return m_propertyBuffer; }

private:
	PropertyBufferLayout m_data;

private:
	Rendering::ConstantBufferHandle m_propertyBuffer;
};

class RenderObject
{
public:
	RenderObject() = default;
	~RenderObject() = default;

public:
	struct MatrixMem
	{
		Matrix4x4 transform;
		Matrix4x4 invTransform;
	};

public:
	void SetStaticMesh(const StaticMeshInstance& staticMesh) { m_staticMesh = staticMesh; }
	const StaticMeshInstance& GetStaticMesh() { return m_staticMesh; }

	void SetTransform(const Matrix4x4& transform) { m_matrixMem.transform = transform; m_matrixMem.invTransform = transform.Inverse(); }
	const Matrix4x4& GetTransform() const { return m_matrixMem.transform; }

private:
	// @TODO This will be replaced after a batching system is implemented
	// Will pack all geometry into single vertex buffer and all materials into single material buffer and index away
	// or something i dunno im shit at my job. Also.. limited to just static mesh here, bad. Will end up with refactor
	// when skinned meshes are a thing
	StaticMeshInstance m_staticMesh;
	MatrixMem m_matrixMem;
};

//
// Buckets
//
// std::vector<DrawBucket> mBuckets;
//
// void Draw()
// {
//		for (const DrawBucket& bucket : mBuckets)
//		{
//			Draw bucket.RenderObject[i] with bucket.DrawState
//		}
// }
//
// BucketProxy* bucketProxy = Renderer::StartBucket();
// bucketProxy->SetDrawState(someState);
// bucketProxy->AddRenderObjectThatPassedSomeCullingOperation(someObject);
// Renderer::SubmitBucket(bucketProxy); // We're done here, bucketProxy invalid now.
// ^^^^^ This looks like it should be some reference-type structure/architecture

typedef std::vector<std::unique_ptr<RenderObject>> RenderObjectContainer;
typedef std::vector<std::unique_ptr<LightObject>> LightObjectContainer;

class RenderEngine
{
public:
	struct SceneData
	{
		// @TODO Make not vector or something
		RenderObjectContainer renderObjects;
		// @TODO Maybe move light stuff into its own area?
		LightObjectContainer lightObjects;
	};

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
	// @todo isMainRenderCall is kinda a hack and should be solved by API instead
	void Render(const char* frameName, bool isMainRenderCall, bool withImgui);
	// Finish() does all the work that the main render phase needs including device present
	void Finish();
	void Shutdown();

	void ClearObjects();

public:
	template <typename TRenderStageType, typename... Args>
	void AddRenderStage(Args... args);

	RenderObjectPtr CreateRenderObject(const StaticMeshInstance& staticMesh);
	void DestroyRenderObject(RenderObjectPtr& renderObject);

	LightObjectPtr CreateLightObject();
	void DestroyLightObject(LightObjectPtr& lightObject);

	void ResizeCanvas(const Vector2D& newSize);
	const Vector2D& GetCanvasSize() const;

	RenderCamera& GetCamera() { return m_camera; }
	
	const Rendering::RenderTargetTexture& GetRenderTarget();
	void SetRenderTarget(Rendering::RenderTargetTexture* renderTarget) { m_renderTarget = renderTarget; }
	
	void SetViewportSize(const Vector2D& size) { m_viewportSize = size; }
	const Vector2D& GetViewportSize() const { return m_viewportSize; }

	// RenderView()
	// Renders the current scene to a render target from a specified camera setup. renderTarget will be allocated
	// if passed nullptr
	void RenderView(const char* frameName, const RenderCamera& renderCamera, std::unique_ptr<Rendering::RenderTargetTexture>& renderTarget);

private:
	void InternalAddRenderStage(IRenderStage* pipeline);

private:
	RenderCamera m_camera;
	SceneData m_sceneData;

	Vector2D m_canvasSize;
	Vector2D m_viewportSize;

	// @TODO currently only single render target support - also write engine-side RenderTarget
	Rendering::RenderTargetTexture* m_renderTarget = nullptr;

	std::vector<std::unique_ptr<IRenderStage>> m_renderStages;
};

template <typename TRenderStageType, typename... Args>
void RenderEngine::AddRenderStage(Args... args)
{
	InternalAddRenderStage(new TRenderStageType(std::forward<Args>(args)...));
}
