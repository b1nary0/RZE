#pragma once

#include <Graphics/RenderBlackboard.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>

namespace Rendering
{
	class RenderTargetTexture;
}

struct RenderViewport
{
	Vector2D Size;
};

struct RenderCamera
{
	// @TODO integrate with viewport maybe
	Matrix4x4 ClipSpace;
	Vector3D Position;
	RenderViewport Viewport;
};

enum class ERenderViewKind
{
	Main,		// The view presented to the user; keeps history (e.g. eye adaptation) across frames
	Secondary	// An extra view such as a camera preview; doesn't run MainOnly stages
};

// One point of view to render the scene from. Its owner fills in the inputs; render stages read them
// (through RenderContext) and keep their per-view data in the view's blackboards.
class RenderView
{
	// The only code that touches the blackboards: the pipeline resets the frame data, and stages reach
	// the blackboards through RenderContext and their ports
	friend class RenderPipeline;
	friend class RenderContext;

public:
	explicit RenderView(ERenderViewKind kind)
		: m_kind(kind) {}

	RenderView(const RenderView&) = delete;
	RenderView& operator=(const RenderView&) = delete;

public:
	ERenderViewKind GetKind() const { return m_kind; }
	bool IsMainView() const { return m_kind == ERenderViewKind::Main; }

public:
	RenderCamera Camera;
	Vector2D ViewportSize;
	// Where the view renders to. Owned by whoever owns the view.
	Rendering::RenderTargetTexture* Target = nullptr;
	// EV on top of auto-exposure
	float ExposureCompensation = 0.0f;

private:
	ERenderViewKind m_kind;
	// Data contracts the stages publish while rendering this view; cleared at the start of each render
	RenderBlackboard m_frame;
	// State a stage keeps for this view across frames
	RenderBlackboard m_persistent;
};
