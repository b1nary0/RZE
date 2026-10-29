#pragma once

#include <Graphics/DebugLine.h>
#include <Graphics/GraphicsDefines.h>
#include <Graphics/LightObject.h>
#include <Graphics/ReflectionProbeObject.h>
#include <Graphics/RenderBlackboard.h>
#include <Graphics/RenderObject.h>
#include <Graphics/SceneChangeLog.h>
#include <Graphics/StaticMeshInstance.h>

#include <Utils/Math/BoundingBox.h>
#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector3D.h>

#include <memory>
#include <vector>

typedef std::vector<std::unique_ptr<RenderObject>> RenderObjectContainer;
typedef std::vector<std::unique_ptr<LightObject>> LightObjectContainer;
typedef std::vector<std::unique_ptr<ReflectionProbeObject>> ReflectionProbeContainer;

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

	ReflectionProbeObjectPtr CreateReflectionProbe();
	void DestroyReflectionProbe(ReflectionProbeObjectPtr& reflectionProbe);

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
	const ReflectionProbeContainer& GetReflectionProbes() const { return m_reflectionProbes; }
	const DebugLineContainer& GetDebugLines() const { return m_debugLines; }

	// Changes whenever a render object is added, removed, moved or given a new mesh, a light is added, removed or
	// changes how it lights the scene (LightObject::GetLightingRevision), or a reflection probe is added, removed,
	// moved or resized. Lets stages skip work whose inputs haven't changed (e.g. ShadowRenderStage's shadow maps).
	U64 GetRevision() const { return m_revision; }
	// GetRevision when a render object was last added, removed, moved, given a new mesh or a new mobility
	U64 GetObjectsRevision() const { return m_objectsRevision; }
	// The same for Static render objects alone (ERenderMobility)
	U64 GetStaticObjectsRevision() const { return m_staticObjectsRevision; }
	// Where Static and Movable render objects were added, removed, moved or given a new mesh, by revision (both where
	// each was and where it is), for anything kept from part of the scene that needs to find which part changed. An
	// object changing mobility is in both.
	const SceneChangeLog& GetStaticChanges() const { return m_staticChanges; }
	const SceneChangeLog& GetMovableChanges() const { return m_movableChanges; }
	// Where lights were added, removed or changed how they light the scene, by revision: where a point light reached
	// before and after (LightObject::GetReach), or everywhere for a directional light. For anything lit with the
	// lights that's kept (e.g. probes), so it can tell which part to light again.
	const SceneChangeLog& GetLightChanges() const { return m_lightChanges; }

	// Axis-aligned world-space box around every render object; empty without any. Kept until one is added, removed,
	// moved or given a new mesh, then worked out again from the objects' boxes when next asked.
	const BoundingBox& GetBounds() const;
	// The same around the Static render objects alone, so what's fitted to it (the probe grid) doesn't follow what
	// moves
	const BoundingBox& GetStaticBounds() const;

private:
	friend class RenderObject;
	// A render object of this mobility was added, removed, moved or given a new mesh: it was within oldBounds and is
	// now within newBounds (either empty if it wasn't or isn't anywhere)
	void OnRenderObjectChanged(ERenderMobility mobility, const BoundingBox& oldBounds, const BoundingBox& newBounds);
	// A render object within bounds became Static or Movable, so it's gone from one kind and arrived in the other
	void OnRenderObjectMobilityChanged(const BoundingBox& bounds);
	// Every render object changed (e.g. they were all removed)
	void OnAllRenderObjectsChanged();

private:
	// @TODO Make not vector or something
	RenderObjectContainer m_renderObjects;
	// @TODO Maybe move light stuff into its own area?
	LightObjectContainer m_lightObjects;
	ReflectionProbeContainer m_reflectionProbes;
	DebugLineContainer m_debugLines;
	U64 m_revision = 0;
	U64 m_objectsRevision = 0;
	U64 m_staticObjectsRevision = 0;
	// Where Static and Movable render objects changed, by revision
	SceneChangeLog m_staticChanges;
	SceneChangeLog m_movableChanges;
	// Where lights changed, by revision; each light logs its own changes into it
	SceneChangeLog m_lightChanges;

	mutable BoundingBox m_bounds;
	mutable bool m_isBoundsStale = true;
	mutable BoundingBox m_staticBounds;
	mutable bool m_isStaticBoundsStale = true;

	// State a stage keeps for this scene across frames, whichever view renders it (e.g. the shadow map)
	RenderBlackboard m_persistent;
};
