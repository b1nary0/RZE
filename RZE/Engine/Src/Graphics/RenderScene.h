#pragma once

#include <Graphics/GraphicsDefines.h>
#include <Graphics/RenderBlackboard.h>
#include <Graphics/StaticMeshInstance.h>

#include <Rendering/BufferHandle.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>

#include <memory>
#include <vector>

class LightObject
{
	// Hands out m_sceneRevision
	friend class RenderScene;

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
	// RenderScene::GetRevision of the scene this light is in
	U64* m_sceneRevision = nullptr;
};

class RenderObject
{
	// Hands out m_sceneRevision
	friend class RenderScene;

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

	// What PickRenderStage writes for this object's pixels; 0 means nothing pickable
	void SetPickID(U32 pickID) { m_pickID = pickID; }
	U32 GetPickID() const { return m_pickID; }

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

	U32 m_pickID = 0;

	// RenderScene::GetRevision of the scene this object is in
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

// Fades a debug line out with distance from a point: fully visible within start, gone beyond end.
// Applied per pixel, so a long line fades along its length. end == 0 means no fade.
struct DebugLineFade
{
	Vector3D centre;
	float start = 0.0f;
	float end = 0.0f;
};

struct DebugLine
{
	Vector3D start;
	Vector3D end;
	Vector3D colour;
	// 1 is opaque; lower values blend with what's behind
	float alpha;
	DebugLineFade fade;
};

typedef std::vector<std::unique_ptr<RenderObject>> RenderObjectContainer;
typedef std::vector<std::unique_ptr<LightObject>> LightObjectContainer;
typedef std::vector<DebugLine> DebugLineContainer;

// What one scene draws: its render objects, lights and debug lines. Each GameScene's components put theirs in a
// scene like this, and any view can render any scene (RenderEngine::Render, RenderSecondaryView), so a scene can be
// rendered without being the one on screen, e.g. for a thumbnail.
class RenderScene
{
	// The only code that touches the persistent blackboard: stages reach it through RenderContext
	friend class RenderContext;

public:
	RenderScene() = default;
	~RenderScene() = default;
	RenderScene(const RenderScene&) = delete;
	RenderScene& operator=(const RenderScene&) = delete;

public:
	RenderObjectPtr CreateRenderObject(const StaticMeshInstance& staticMesh);
	void DestroyRenderObject(RenderObjectPtr& renderObject);

	LightObjectPtr CreateLightObject();
	void DestroyLightObject(LightObjectPtr& lightObject);

	// Destroys every render object (not the lights)
	void ClearObjects();

	// alpha below 1 blends the line with what's behind it; fade, if given, fades it out with distance. Lines are kept
	// until ClearDebugLines, which RenderEngine::Render does for the main scene once it's drawn them.
	void DrawLine(const Vector3D& start, const Vector3D& end, const Vector3D& colour = Vector3D(1.0f, 0.0f, 0.0f), float alpha = 1.0f, const DebugLineFade& fade = DebugLineFade());
	// Draws the 12 edges of an object-space box, transformed into world space by transform
	void DrawWireBox(const Vector3D& boundsMin, const Vector3D& boundsMax, const Matrix4x4& transform, const Vector3D& colour, float alpha = 1.0f);
	void ClearDebugLines() { m_debugLines.clear(); }

	// Destroys what stages keep for this scene (e.g. its shadow map); they make it again the next time it's rendered
	void ReleaseStageState() { m_persistent.Clear(); }

	const RenderObjectContainer& GetRenderObjects() const { return m_renderObjects; }
	const LightObjectContainer& GetLightObjects() const { return m_lightObjects; }
	const DebugLineContainer& GetDebugLines() const { return m_debugLines; }

	// Changes whenever a render object is added, removed, moved or given a new mesh, or a light changes direction.
	// Lets stages skip work whose inputs haven't changed (e.g. ShadowRenderStage's shadow map).
	U64 GetRevision() const { return m_revision; }

private:
	// @TODO Make not vector or something
	RenderObjectContainer m_renderObjects;
	// @TODO Maybe move light stuff into its own area?
	LightObjectContainer m_lightObjects;
	DebugLineContainer m_debugLines;
	U64 m_revision = 0;

	// State a stage keeps for this scene across frames, whichever view renders it (e.g. the shadow map)
	RenderBlackboard m_persistent;
};
