#pragma once

#include <Graphics/MeshDraw.h>
#include <Graphics/StaticMeshInstance.h>

#include <Utils/Math/BoundingBox.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/PrimitiveDefs.h>

#include <memory>
#include <vector>

class RenderScene;

// Something a scene draws: a mesh instance at a transform, with its world bounds and draws worked out when asked
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
	// What the object draws; null draws nothing. Its draws and bounds follow the new instance, and the old one is
	// released unless something else holds it.
	void SetMeshInstance(const std::shared_ptr<const StaticMeshInstance>& meshInstance);
	// Null when the object has nothing to draw
	const std::shared_ptr<const StaticMeshInstance>& GetMeshInstance() const { return m_meshInstance; }

	// Called every frame by RenderComponent, so an unchanged transform returns early
	void SetTransform(const Matrix4x4& transform);
	const Matrix4x4& GetTransform() const { return m_matrices.Transform; }

	// The mesh's box in the object's own space; empty without a mesh or sub-meshes. Only changes with the mesh.
	const BoundingBox& GetLocalBounds() const { return m_localBounds; }
	// GetLocalBounds placed in the world by the transform, as an axis-aligned box. Worked out when asked after the
	// object moves or changes mesh.
	const BoundingBox& GetBounds() const;
	// Everything the object draws, each with its world transform and bounds (StaticMeshInstance::BuildDraws). Worked
	// out when asked after the object moves or changes mesh; until then the draws, and pointers to them, stay valid.
	const std::vector<MeshDraw>& GetDraws() const;

	// What PickRenderStage writes for this object's pixels; 0 means nothing pickable
	void SetPickID(U64 pickID) { m_pickID = pickID; }
	U64 GetPickID() const { return m_pickID; }

	// Never handed out twice, to a render object or a light, so a RenderObjectPtr can tell once this object is gone
	U64 GetLiveID() const { return m_liveID; }

private:
	// It moved or changed mesh: its world bounds are stale, and so is its scene. oldBounds is GetBounds from before.
	void MarkChanged(const BoundingBox& oldBounds);

private:
	// @TODO This will be replaced after a batching system is implemented
	// Will pack all geometry into single vertex buffer and all materials into single material buffer and index away
	// or something i dunno im shit at my job. Also.. limited to just static mesh here, bad. Will end up with refactor
	// when skinned meshes are a thing
	// Held so the mesh its draws point into stays alive
	std::shared_ptr<const StaticMeshInstance> m_meshInstance;
	GraphicsLayouts::ObjectBufferLayout m_matrices;

	BoundingBox m_localBounds;
	// Worked out from the transform when asked
	mutable BoundingBox m_bounds;
	mutable bool m_isBoundsStale = true;
	mutable std::vector<MeshDraw> m_draws;
	mutable bool m_areDrawsStale = true;

	U64 m_pickID = 0;

	// The scene this object is in
	RenderScene* m_scene = nullptr;

	const U64 m_liveID;
};
