#pragma once

#include <Graphics/StaticMesh.h>

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

struct RenderCamera
{
	// @TODO integrate with viewport maybe
	Matrix4x4 ClipSpace;
	Vector3D Position;
};

// @TODO Move to own file
class LightObject
{
public:
	LightObject() = default;
	~LightObject() = default;

	void Initialize();

	void SetPosition(const Vector3D& position) { m_position = position; }
	const Vector3D& GetPosition() const { return m_position; }

	void SetStrength(float strength) { m_strength = strength; }
	float GetStrength() const { return m_strength; }

	void SetColour(const Vector4D& colour) { m_colour = colour; }
	const Vector4D& GetColour() const { return m_colour; }

	const Rendering::ConstantBufferHandle& GetPropertyBuffer() { return m_propertyBuffer; }

private:
	Vector3D m_position = Vector3D::ZERO;
	Vector4D m_colour = Vector4D::ZERO;
	float m_strength = 0.0f;

private:
	Rendering::ConstantBufferHandle m_propertyBuffer;
};

class RenderObject
{
public:
	RenderObject() = default;
	~RenderObject() = default;

	void SetStaticMesh(const StaticMesh& staticMesh) { m_staticMesh = staticMesh; }
	const StaticMesh& GetStaticMesh() { return m_staticMesh; }

	void SetTransform(const Matrix4x4& transform) { m_transform = transform; }
	const Matrix4x4& GetTransform() const { return m_transform; }

private:
	// @TODO This will be replaced after a batching system is implemented
	// Will pack all geometry into single vertex buffer and all materials into single material buffer and index away
	// or something i dunno im shit at my job. Also.. limited to just static mesh here, bad. Will end up with refactor
	// when skinned meshes are a thing
	StaticMesh m_staticMesh;
	Matrix4x4 m_transform;
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

public:
	template <typename TRenderStageType, typename... Args>
	void AddRenderStage(Args... args);

	std::shared_ptr<RenderObject> CreateRenderObject(const StaticMesh& staticMesh);
	void DestroyRenderObject(std::shared_ptr<RenderObject>& renderObject);

	std::shared_ptr<LightObject> CreateLightObject();
	void DestroyLightObject(std::shared_ptr<LightObject>& lightObject);

	void ResizeCanvas(const Vector2D& newSize);
	const Vector2D& GetCanvasSize() const;

	RenderCamera& GetCamera() { return m_camera; }
	
	const Rendering::RenderTargetTexture& GetRenderTarget();
	void SetRenderTarget(Rendering::RenderTargetTexture* renderTarget) { m_renderTarget = renderTarget; }
	
	void SetViewportSize(const Vector2D& size) { m_viewportSize = size; }
	const Vector2D& GetViewportSize() const { return m_viewportSize; }

private:
	void InternalAddRenderStage(IRenderStage* pipeline);

private:
	RenderCamera m_camera;

	Vector2D m_canvasSize;
	Vector2D m_viewportSize;

	// @TODO currently only single render target support - also write engine-side RenderTarget
	Rendering::RenderTargetTexture* m_renderTarget = nullptr;

	// @TODO Make not vector or something
	std::vector<std::shared_ptr<RenderObject>> m_renderObjects;
	// @TODO Maybe move light stuff into its own area?
	std::vector<std::shared_ptr<LightObject>> m_lightObjects;

	std::vector<std::unique_ptr<IRenderStage>> m_renderStages;

	// #TODO
	// Turn this into a command structure. Something like:
	// UpdateRenderObject<UpdateTransformCommand>(renderObject);
	// Where UpdateTransformCommand:
	// 
	// class UpdateTransformCommand : public RenderCommand
	// {
	// public:
	//		UpdateTransformCommand(const RenderObject& renderObject, const Matrix4x4& transform);
	// 		virtual void Execute();
	// private:
	//		RenderObject& renderObject;
	// 		Matrix4x4 transform;
	// };


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
};

template <typename TRenderStageType, typename... Args>
void RenderEngine::AddRenderStage(Args... args)
{
	InternalAddRenderStage(new TRenderStageType(std::forward<Args>(args)...));
}
