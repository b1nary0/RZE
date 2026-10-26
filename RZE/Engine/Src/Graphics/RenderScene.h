#pragma once

#include <Graphics/GraphicsDefines.h>
#include <Graphics/RenderBlackboard.h>
#include <Graphics/SceneChangeLog.h>
#include <Graphics/StaticMeshInstance.h>

#include <Utils/Math/BoundingBox.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/Math/Vector4D.h>

#include <memory>
#include <vector>

// How a light shines, which decides how it's lit and shadowed. The shaders branch on the values (Light::Type in
// Common/PixelResources.hlsli), so new types go at the end.
enum class ELightType
{
	// From one direction everywhere, like the sun; position doesn't matter
	Directional,
	// From a point in every direction, fading out by its range
	Point
};

// A light in a scene: the data every type shares, read by its type. It has no GPU resources of its own:
// ShadowRenderStage puts all of a scene's lights into one buffer.
class LightObject
{
	// Hands out m_sceneRevision
	friend class RenderScene;

public:
	// Alive until destroyed (RenderSceneLifetime)
	explicit LightObject(ELightType type);
	~LightObject();

	LightObject(const LightObject&) = delete;
	LightObject& operator=(const LightObject&) = delete;

	// Never handed out twice, so a LightObjectPtr can tell once this light is gone
	U64 GetLiveID() const { return m_liveID; }

	// The setters below mark the light's shadows changed when the value moves them (see MarkShadowsChanged)
	void SetType(ELightType type);
	ELightType GetType() const { return m_type; }

	// World space; only used by point lights
	void SetPosition(const Vector3D& position);
	const Vector3D& GetPosition() const { return m_position; }

	// World space, normalized; the direction light travels. Only used by directional lights.
	void SetDirection(const Vector3D& direction);
	const Vector3D& GetDirection() const { return m_direction; }

	// Distance in metres at which a point light has faded to nothing; only used by point lights
	void SetRange(float range);
	float GetRange() const { return m_range; }

	// The fraction of its strength a point light gives at rangeRatio (distance / range): 1 at the light, falling to 0
	// at its range. Mirrors LightAttenuation in Common/Lighting.hlsli.
	static float CalculatePointFalloff(float rangeRatio);
	// The rangeRatio within which a point light of this strength and colour (sRGB) lights a surface facing it more
	// than the ambient light does, so it's the main light there; 0 if it never does. Grows with strength, up to 1.
	static float CalculateOutshinesAmbientRatio(float strength, const Vector4D& colour);

	void SetStrength(float strength) { m_strength = strength; }
	float GetStrength() const { return m_strength; }

	// sRGB
	void SetColour(const Vector4D& colour) { m_colour = colour; }
	const Vector4D& GetColour() const { return m_colour; }

	// RenderScene::GetRevision when the light was created or its shadows last moved, so a shadow map rendered at an
	// older revision is out of date. 0 for a light in no scene.
	U64 GetShadowRevision() const { return m_shadowRevision; }

private:
	// For changes that move the light's shadows, so its shadow map is made again: a new scene revision, which becomes
	// the light's shadow revision. Colour and strength don't move them.
	void MarkShadowsChanged();

private:
	ELightType m_type;
	Vector3D m_position;
	Vector3D m_direction = Vector3D(0.0f, -1.0f, 0.0f);
	Vector4D m_colour = Vector4D(1.0f);
	float m_strength = 1.0f;
	float m_range = 10.0f;

	// RenderScene::GetRevision of the scene this light is in
	U64* m_sceneRevision = nullptr;
	U64 m_shadowRevision = 0;

	const U64 m_liveID;
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
	// What the object draws; null draws nothing. Its draws and bounds follow the new instance, and the old one is
	// released unless something else holds it.
	void SetMeshInstance(const std::shared_ptr<const StaticMeshInstance>& meshInstance);
	// Null when the object has nothing to draw
	const std::shared_ptr<const StaticMeshInstance>& GetMeshInstance() const { return m_meshInstance; }

	// Called every frame by RenderComponent, so an unchanged transform returns early
	void SetTransform(const Matrix4x4& transform);
	const Matrix4x4& GetTransform() const { return m_matrixMem.transform; }

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

	// Never handed out twice, so a RenderObjectPtr can tell once this object is gone
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
	MatrixMem m_matrixMem;

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
	// How opaque it's drawn where it's behind the scene, so e.g. a light's range still shows through the walls it
	// passes behind; 0 hides those parts
	float occludedAlpha = 0.0f;
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
	// meshInstance may be null (an object with nothing to draw yet)
	RenderObjectPtr CreateRenderObject(const std::shared_ptr<const StaticMeshInstance>& meshInstance);
	void DestroyRenderObject(RenderObjectPtr& renderObject);

	LightObjectPtr CreateLightObject(ELightType type);
	void DestroyLightObject(LightObjectPtr& lightObject);

	// The first directional light, or null without one
	const LightObject* GetDirectionalLight() const;

	// Destroys every render object (not the lights)
	void ClearObjects();

	// alpha below 1 blends the line with what's behind it; fade, if given, fades it out with distance. Lines are kept
	// until ClearDebugLines, which RenderEngine::Render does for the main scene once it's drawn them.
	void DrawLine(const Vector3D& start, const Vector3D& end, const Vector3D& colour = Vector3D(1.0f, 0.0f, 0.0f), float alpha = 1.0f, const DebugLineFade& fade = DebugLineFade());
	// Draws the 12 edges of an object-space box, transformed into world space by transform
	void DrawWireBox(const BoundingBox& box, const Matrix4x4& transform, const Vector3D& colour, float alpha = 1.0f);
	// Draws a world-space sphere as three circles, one around each world axis. occludedAlpha is how opaque its parts
	// behind the scene are (DebugLine::occludedAlpha).
	void DrawWireSphere(const Vector3D& centre, float radius, const Vector3D& colour, float alpha = 1.0f, float occludedAlpha = 0.0f);
	void ClearDebugLines() { m_debugLines.clear(); }

	// Destroys what stages keep for this scene (e.g. its shadow map); they make it again the next time it's rendered
	void ReleaseStageState() { m_persistent.Clear(); }

	const RenderObjectContainer& GetRenderObjects() const { return m_renderObjects; }
	const LightObjectContainer& GetLightObjects() const { return m_lightObjects; }
	const DebugLineContainer& GetDebugLines() const { return m_debugLines; }

	// Changes whenever a render object is added, removed, moved or given a new mesh, or a light is added, removed or
	// changed in a way that moves its shadows (LightObject::GetShadowRevision). Lets stages skip work whose inputs
	// haven't changed (e.g. ShadowRenderStage's shadow maps).
	U64 GetRevision() const { return m_revision; }
	// GetRevision when a render object was last added, removed, moved or given a new mesh
	U64 GetObjectsRevision() const { return m_objectsRevision; }
	// Whether a render object was added, removed, moved or given a new mesh within region (world space) after
	// sinceRevision (a GetRevision), counting both where it was and where it is. Can answer true for changes elsewhere
	// once many objects have changed since (SceneChangeLog), never false for one within.
	bool HaveObjectsChangedWithin(const BoundingBox& region, U64 sinceRevision) const { return m_changeLog.HasChangedWithin(region, sinceRevision); }

	// Axis-aligned world-space box around every render object; empty without any. Kept until one is added, removed,
	// moved or given a new mesh, then worked out again from the objects' boxes when next asked.
	const BoundingBox& GetBounds() const;

private:
	friend class RenderObject;
	// A render object was added, removed, moved or given a new mesh: it was within oldBounds and is now within
	// newBounds (either empty if it wasn't or isn't anywhere)
	void OnRenderObjectChanged(const BoundingBox& oldBounds, const BoundingBox& newBounds);
	// Every render object changed (e.g. they were all removed)
	void OnAllRenderObjectsChanged();

private:
	// @TODO Make not vector or something
	RenderObjectContainer m_renderObjects;
	// @TODO Maybe move light stuff into its own area?
	LightObjectContainer m_lightObjects;
	DebugLineContainer m_debugLines;
	U64 m_revision = 0;
	U64 m_objectsRevision = 0;
	// Where render objects changed, by revision
	SceneChangeLog m_changeLog;

	mutable BoundingBox m_bounds;
	mutable bool m_isBoundsStale = true;

	// State a stage keeps for this scene across frames, whichever view renders it (e.g. the shadow map)
	RenderBlackboard m_persistent;
};
