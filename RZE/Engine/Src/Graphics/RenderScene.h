#pragma once

#include <Graphics/GraphicsDefines.h>
#include <Graphics/RenderBlackboard.h>
#include <Graphics/StaticMeshInstance.h>

#include <Rendering/BufferHandle.h>

#include <Utils/Math/BoundingBox.h>
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
	// Alive until destroyed (RenderSceneLifetime)
	LightObject();
	~LightObject();

	LightObject(const LightObject&) = delete;
	LightObject& operator=(const LightObject&) = delete;

	void Initialize();

	// Never reused, so a LightObjectPtr can tell once this light is gone
	U32 GetLiveID() const { return m_liveID; }

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

	const U32 m_liveID;
};

class RenderObject
{
	// Hands out m_scene
	friend class RenderScene;

public:
	// Alive until destroyed (RenderSceneLifetime)
	RenderObject();
	~RenderObject();

	RenderObject(const RenderObject&) = delete;
	RenderObject& operator=(const RenderObject&) = delete;

public:
	struct MatrixMem
	{
		Matrix4x4 transform;
		Matrix4x4 invTransform;
	};

	// World-space bounds of one placed submesh, kept up to date as the object's mesh or transform changes
	struct SubMeshBounds
	{
		// The submesh's object-space bounding box's corners, transformed into world space
		BoundingBox::Corners Corners;
		// Axis-aligned box around Corners
		BoundingBox Bounds;
	};

public:
	void SetStaticMesh(const StaticMeshInstance& staticMesh);
	const StaticMeshInstance& GetStaticMesh() { return m_staticMesh; }
	const StaticMeshInstance& GetStaticMesh() const { return m_staticMesh; }

	// Called every frame by RenderComponent, so an unchanged transform returns early
	void SetTransform(const Matrix4x4& transform);
	const Matrix4x4& GetTransform() const { return m_matrixMem.transform; }

	// The mesh's box in the object's own space; empty without any submeshes. Only changes with the mesh.
	const BoundingBox& GetLocalBounds() const { return m_localBounds; }
	// GetLocalBounds placed in the world by the transform, as an axis-aligned box. Worked out when asked after the
	// object moves or changes mesh.
	const BoundingBox& GetBounds() const;
	// Indexed like GetStaticMesh().GetPlacements(); tighter than GetBounds, for culling submesh by submesh. Worked out
	// when asked after the object moves or changes mesh.
	const std::vector<SubMeshBounds>& GetSubMeshBounds() const;
	// Where a placement is drawn: the object's transform times the placement's. Its inverse follows it in memory,
	// like MatrixMem, so it uploads as the shaders' ObjectBuffer. Placements at the mesh's origin all return
	// GetTransform() itself, so comparing addresses tells a stage when it needs to upload again.
	const Matrix4x4& GetPlacementTransform(size_t placementIndex) const;

	// What PickRenderStage writes for this object's pixels; 0 means nothing pickable
	void SetPickID(U32 pickID) { m_pickID = pickID; }
	U32 GetPickID() const { return m_pickID; }

	// Never reused, so a RenderObjectPtr can tell once this object is gone
	U32 GetLiveID() const { return m_liveID; }

private:
	// It moved or changed mesh: its world bounds are stale, and so is its scene
	void MarkChanged();
	// Works out m_subMeshBounds and m_placementMatrices if they're stale
	void UpdatePlacements() const;

private:
	// @TODO This will be replaced after a batching system is implemented
	// Will pack all geometry into single vertex buffer and all materials into single material buffer and index away
	// or something i dunno im shit at my job. Also.. limited to just static mesh here, bad. Will end up with refactor
	// when skinned meshes are a thing
	StaticMeshInstance m_staticMesh;
	MatrixMem m_matrixMem;

	BoundingBox m_localBounds;
	// Worked out from the transform when asked
	mutable BoundingBox m_bounds;
	mutable bool m_isBoundsStale = true;
	mutable std::vector<SubMeshBounds> m_subMeshBounds;
	// Indexed like the placements; only filled in for those away from the mesh's origin
	mutable std::vector<MatrixMem> m_placementMatrices;
	mutable bool m_arePlacementsStale = true;

	U32 m_pickID = 0;

	// The scene this object is in
	RenderScene* m_scene = nullptr;

	const U32 m_liveID;
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
	void DrawWireBox(const BoundingBox& box, const Matrix4x4& transform, const Vector3D& colour, float alpha = 1.0f);
	void ClearDebugLines() { m_debugLines.clear(); }

	// Destroys what stages keep for this scene (e.g. its shadow map); they make it again the next time it's rendered
	void ReleaseStageState() { m_persistent.Clear(); }

	const RenderObjectContainer& GetRenderObjects() const { return m_renderObjects; }
	const LightObjectContainer& GetLightObjects() const { return m_lightObjects; }
	const DebugLineContainer& GetDebugLines() const { return m_debugLines; }

	// Changes whenever a render object is added, removed, moved or given a new mesh, or a light changes direction.
	// Lets stages skip work whose inputs haven't changed (e.g. ShadowRenderStage's shadow map).
	U64 GetRevision() const { return m_revision; }

	// Axis-aligned world-space box around every render object; empty without any. Kept until one is added, removed,
	// moved or given a new mesh, then worked out again from the objects' boxes when next asked.
	const BoundingBox& GetBounds() const;

private:
	friend class RenderObject;
	// A render object was added, removed, moved or given a new mesh
	void OnRenderObjectChanged();

private:
	// @TODO Make not vector or something
	RenderObjectContainer m_renderObjects;
	// @TODO Maybe move light stuff into its own area?
	LightObjectContainer m_lightObjects;
	DebugLineContainer m_debugLines;
	U64 m_revision = 0;

	mutable BoundingBox m_bounds;
	mutable bool m_isBoundsStale = true;

	// State a stage keeps for this scene across frames, whichever view renders it (e.g. the shadow map)
	RenderBlackboard m_persistent;
};
