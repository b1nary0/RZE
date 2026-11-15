#pragma once

#include <Graphics/RenderScene.h>
#include <Graphics/RenderView.h>
#include <Graphics/RenderWorkCounts.h>
#include <Graphics/VisibleDrawsCache.h>

template <typename T> class RenderInput;
template <typename T> class RenderOutput;
template <typename T> class RenderInOut;

// Adds amount to one of the context's RenderWorkCounts fields, e.g. RZE_COUNT_WORK(context, ShadowDraws, 3). Without
// render stats it's gone, with the amount: nothing in it is worked out, so don't count with a call that has to run.
#ifdef RZE_RENDER_STATS
#define RZE_COUNT_WORK(context, field, amount) do { (context).WorkCounts.field += (amount); } while (0)
#else
#define RZE_COUNT_WORK(context, field, amount) do {} while (0)
#endif

// Everything a render stage can use while rendering one view. Built by RenderPipeline for each render.
//
// The view's inputs are read-only. Data from other stages is reached through the stage's ports
// (RenderPorts.h), not through the context, so a stage can only touch what it declared in Setup(). What the scene
// shows of a frustum (GetVisibleDraws) is the scene's, not a stage's: whichever stage asks first culls it.
class RenderContext
{
	template <typename T> friend class RenderInput;
	template <typename T> friend class RenderOutput;
	template <typename T> friend class RenderInOut;

public:
#ifdef RZE_RENDER_STATS
	RenderContext(RenderView& view, RenderScene& scene, float deltaTime, U64 frameIndex, RenderWorkCounts& workCounts, VisibleDrawsCache& visibleDraws)
#else
	RenderContext(RenderView& view, RenderScene& scene, float deltaTime, U64 frameIndex, VisibleDrawsCache& visibleDraws)
#endif
		: View(view)
		, Scene(scene)
		, DeltaTime(deltaTime)
		, FrameIndex(frameIndex)
#ifdef RZE_RENDER_STATS
		, WorkCounts(workCounts)
#endif
		, m_view(view)
		, m_scene(scene)
		, m_visibleDraws(visibleDraws) {}

	RenderContext(const RenderContext&) = delete;
	RenderContext& operator=(const RenderContext&) = delete;

public:
	// Camera, viewport, target, exposure and kind of the view being rendered
	const RenderView& View;
	// The objects, lights and debug lines being drawn
	const RenderScene& Scene;
	// Seconds since the previous frame
	const float DeltaTime;
	// Counts frames: the same for every view rendered in one frame, so a stage can do a scene's work once a frame
	// whichever view draws it first
	const U64 FrameIndex;
#ifdef RZE_RENDER_STATS
	// What the stage did that a scene change caused (probe captures, shadow maps), added to as it's recorded
	RenderWorkCounts& WorkCounts;
#endif

	// This view's state that lasts across frames. Each value should only be used by the stage that owns it.
	RenderBlackboard& Persistent() { return m_view.m_persistentBlackboard; }
	// The scene's state that lasts across frames, shared by every view that renders it (e.g. the shadow map). Each
	// value should only be used by the stage that owns it.
	RenderBlackboard& ScenePersistent() { return m_scene.m_persistent; }

	// The scene's draws request sees, culled once per view for each different request, so stages asking for the same
	// one share the result. Valid for the rest of the view's render.
	VisibleDraws GetVisibleDraws(const CullRequest& request) { return m_visibleDraws.Get(m_scene, request); }

private:
	RenderBlackboard& FrameData() const { return m_view.m_frameBlackboard; }

private:
	RenderView& m_view;
	RenderScene& m_scene;
	VisibleDrawsCache& m_visibleDraws;
};
