#pragma once

#include <Graphics/StaticMeshInstance.h>

#include <Graphics/GraphicsDefines.h>
#include <Graphics/RenderView.h>

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
class RenderPipeline;

// @TODO Move to own file
class LightObject
{
	// Hands out m_sceneRevision
	friend class RenderEngine;

public:
	struct PropertyBufferLayout
	{
		Vector3D direction; // World-space, normalized; the direction light travels
		float _pad0; // Matches HLSL cbuffer packing: float4 can't straddle a 16-byte boundary
		Vector4D colour;
		float strength;
	};

public:
	LightObject() = default;
	~LightObject() = default;

	void Initialize();

	void SetDirection(const Vector3D& direction)
	{
		if (direction != m_data.direction)
		{
			m_data.direction = direction;
			MarkSceneChanged();
		}
	}
	const Vector3D& GetDirection() const { return m_data.direction; }

	void SetStrength(float strength) { m_data.strength = strength; }
	float GetStrength() const { return m_data.strength; }

	void SetColour(const Vector4D& colour) { m_data.colour = colour; }
	const Vector4D& GetColour() const { return m_data.colour; }

	const PropertyBufferLayout& GetData() { return m_data; }

	const Rendering::ConstantBufferHandle& GetPropertyBuffer() { return m_propertyBuffer; }

private:
	// Only the direction counts: colour and strength don't change anything that's cached (e.g. the shadow map)
	void MarkSceneChanged() { if (m_sceneRevision != nullptr) { ++*m_sceneRevision; } }

private:
	PropertyBufferLayout m_data;

private:
	Rendering::ConstantBufferHandle m_propertyBuffer;
	// RenderEngine::SceneData::revision of the scene this light is in
	U64* m_sceneRevision = nullptr;
};

class RenderObject
{
	// Hands out m_sceneRevision
	friend class RenderEngine;

public:
	RenderObject() = default;
	~RenderObject() = default;

public:
	struct MatrixMem
	{
		Matrix4x4 transform;
		Matrix4x4 invTransform;
	};

	// World-space bounds of one submesh, kept up to date as the object's mesh or transform changes
	struct SubMeshBounds
	{
		// The submesh's object-space bounding box, transformed into world space
		Vector3D Corners[8];
		// Axis-aligned box around Corners
		Vector3D Min;
		Vector3D Max;
	};

public:
	void SetStaticMesh(const StaticMeshInstance& staticMesh);
	const StaticMeshInstance& GetStaticMesh() { return m_staticMesh; }

	// Called every frame by RenderComponent, so an unchanged transform returns early
	void SetTransform(const Matrix4x4& transform);
	const Matrix4x4& GetTransform() const { return m_matrixMem.transform; }

	// Indexed like GetStaticMesh().GetSubMeshes()
	const std::vector<SubMeshBounds>& GetSubMeshBounds() const { return m_subMeshBounds; }
	// Axis-aligned world-space box around every submesh
	const Vector3D& GetBoundsMin() const { return m_boundsMin; }
	const Vector3D& GetBoundsMax() const { return m_boundsMax; }

private:
	void UpdateWorldBounds();
	void MarkSceneChanged() { if (m_sceneRevision != nullptr) { ++*m_sceneRevision; } }

private:
	// @TODO This will be replaced after a batching system is implemented
	// Will pack all geometry into single vertex buffer and all materials into single material buffer and index away
	// or something i dunno im shit at my job. Also.. limited to just static mesh here, bad. Will end up with refactor
	// when skinned meshes are a thing
	StaticMeshInstance m_staticMesh;
	MatrixMem m_matrixMem;

	std::vector<SubMeshBounds> m_subMeshBounds;
	Vector3D m_boundsMin;
	Vector3D m_boundsMax;

	// RenderEngine::SceneData::revision of the scene this object is in
	U64* m_sceneRevision = nullptr;
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

struct DebugLine
{
	Vector3D start;
	Vector3D end;
	Vector3D colour;
};

typedef std::vector<std::unique_ptr<RenderObject>> RenderObjectContainer;
typedef std::vector<std::unique_ptr<LightObject>> LightObjectContainer;
typedef std::vector<DebugLine> DebugLineContainer;

class RenderEngine
{
public:
	struct SceneData
	{
		// @TODO Make not vector or something
		RenderObjectContainer renderObjects;
		// @TODO Maybe move light stuff into its own area?
		LightObjectContainer lightObjects;

		DebugLineContainer debugLines;

		// Changes whenever a render object is added, removed, moved or given a new mesh, or a light changes
		// direction. Lets stages skip work whose inputs haven't changed (e.g. ShadowRenderStage's shadow map).
		U64 revision = 0;
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
	// Renders the main view as one frame
	void Render(const char* frameName);
	// Finish() does all the work that the main render phase needs including device present
	void Finish();
	void Shutdown();

	void ClearObjects();
	// Destroys all render stages, releasing any resources they hold. Must be called before ResourceHandler::ShutDown().
	void ReleaseRenderStages();

public:
	template <typename TRenderStageType, typename... Args>
	void AddRenderStage(Args... args);

	RenderObjectPtr CreateRenderObject(const StaticMeshInstance& staticMesh);
	void DestroyRenderObject(RenderObjectPtr& renderObject);

	LightObjectPtr CreateLightObject();
	void DestroyLightObject(LightObjectPtr& lightObject);

	void DrawLine(const Vector3D& start, const Vector3D& end, const Vector3D& colour = Vector3D(1.0f, 0.0f, 0.0f));
	// Draws the 12 edges of an object-space box, transformed into world space by transform
	void DrawWireBox(const Vector3D& boundsMin, const Vector3D& boundsMax, const Matrix4x4& transform, const Vector3D& colour);

	void ResizeCanvas(const Vector2D& newSize);
	const Vector2D& GetCanvasSize() const;

	// The view presented to the user. Its owner (the app and the active camera) fills in its inputs.
	RenderView& GetMainView() { return m_mainView; }

	// Renders the current scene into a secondary view (e.g. a camera preview) as part of the current frame.
	// The view must have a Target.
	void RenderSecondaryView(const char* frameName, RenderView& view);

private:
	void InternalAddRenderStage(IRenderStage* stage);

private:
	SceneData m_sceneData;

	Vector2D m_canvasSize;

	RenderView m_mainView { ERenderViewKind::Main };
	std::unique_ptr<RenderPipeline> m_pipeline;
};

template <typename TRenderStageType, typename... Args>
void RenderEngine::AddRenderStage(Args... args)
{
	InternalAddRenderStage(new TRenderStageType(std::forward<Args>(args)...));
}
