#pragma once

#include <Graphics/RenderEngine.h>
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
	RenderContext(RenderView& view, const RenderEngine::SceneData& scene, float deltaTime)
		: View(view)
		, Scene(scene)
		, DeltaTime(deltaTime)
		, m_view(view) {}

	RenderContext(const RenderContext&) = delete;
	RenderContext& operator=(const RenderContext&) = delete;

public:
	// Camera, viewport, target, exposure and kind of the view being rendered
	const RenderView& View;
	const RenderEngine::SceneData& Scene;
	// Seconds since the previous frame
	const float DeltaTime;

	// This view's state that lasts across frames. Each value should only be used by the stage that owns it.
	RenderBlackboard& Persistent() { return m_view.m_persistent; }

private:
	RenderBlackboard& FrameData() const { return m_view.m_frame; }

private:
	RenderView& m_view;
};
