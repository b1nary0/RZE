#pragma once

#include <Graphics/RenderScene.h>
#include <Graphics/RenderView.h>

template <typename T> class RenderInput;
template <typename T> class RenderOutput;
template <typename T> class RenderInOut;

// Everything a render stage can use while rendering one view. Built by RenderPipeline for each render.
//
// The view's inputs are read-only. Data from other stages is reached through the stage's ports
// (RenderPorts.h), not through the context, so a stage can only touch what it declared in Setup().
class RenderContext
{
	template <typename T> friend class RenderInput;
	template <typename T> friend class RenderOutput;
	template <typename T> friend class RenderInOut;

public:
	RenderContext(RenderView& view, RenderScene& scene, float deltaTime)
		: View(view)
		, Scene(scene)
		, DeltaTime(deltaTime)
		, m_view(view)
		, m_scene(scene) {}

	RenderContext(const RenderContext&) = delete;
	RenderContext& operator=(const RenderContext&) = delete;

public:
	// Camera, viewport, target, exposure and kind of the view being rendered
	const RenderView& View;
	// The objects, lights and debug lines being drawn
	const RenderScene& Scene;
	// Seconds since the previous frame
	const float DeltaTime;

	// This view's state that lasts across frames. Each value should only be used by the stage that owns it.
	RenderBlackboard& Persistent() { return m_view.m_persistent; }
	// The scene's state that lasts across frames, shared by every view that renders it (e.g. the shadow map). Each
	// value should only be used by the stage that owns it.
	RenderBlackboard& ScenePersistent() { return m_scene.m_persistent; }

private:
	RenderBlackboard& FrameData() const { return m_view.m_frame; }

private:
	RenderView& m_view;
	RenderScene& m_scene;
};
