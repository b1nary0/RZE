#pragma once

#include <Graphics/AmbientOcclusion.h>
#include <Graphics/AntiAliasing.h>
#include <Graphics/RenderBlackboard.h>

#include <Utils/Math/Matrix4x4.h>
#include <Utils/Math/Vector2D.h>
#include <Utils/Math/Vector3D.h>
#include <Utils/PrimitiveDefs.h>

#include <memory>
#include <optional>

namespace Rendering
{
	class RenderTargetTexture;
	struct TexelReadback;
}

// Asks which object is drawn at one pixel of the main view; served by PickRenderStage
struct RenderPickRequest
{
	// Relative to the viewport's top-left
	U32 X = 0;
	U32 Y = 0;
	// Receives the RenderObject pick ID there, or 0 for nothing, a frame or two later
	std::shared_ptr<Rendering::TexelReadback> Result;
	// If set, receives the depth of the nearest surface there at the same time, pickable or not; read it with DecodeDepth
	std::shared_ptr<Rendering::TexelReadback> DepthResult;

	// A DepthResult's depth: 0 at the near plane, 1 at the far plane or where nothing is drawn
	static float DecodeDepth(U64 texel);
};

// Uploaded as is as the shaders' CameraBuffer (Common/VertexResources.hlsli), which reads the members up to Position, so
// those must match it. The view's size is RenderView::ViewportSize.
struct RenderCamera
{
	Matrix4x4 ClipSpace;
	Vector3D Position;
	// The perspective part of ClipSpace on its own, for passes that rebuild positions from depth (ambient occlusion).
	// Left as it is (not a perspective), those passes skip the view.
	Matrix4x4 Projection;
};

enum class ERenderViewKind
{
	Main,		// The view presented to the user; keeps history (e.g. eye adaptation) across frames
	Secondary	// An extra view such as a camera preview; doesn't run MainOnly stages
};

// How a view's surfaces are lit
enum class ERenderViewLighting
{
	Scene,		// By the scene's directional light and its shadows, over a sky and ground ambient
	Uniform		// Evenly from every direction, ignoring the scene's light, e.g. so a mesh thumbnail shows all of the mesh
};

// What a view shows: the lit scene, or one of the passes that make it, for checking that pass
enum class ERenderViewDebug
{
	Lit,
	// The ambient occlusion alone, white where nothing occludes (AmbientOcclusionStage)
	AmbientOcclusion
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
	// Exposes to the scene's measured brightness; off, the exposure is a fixed 1, for lighting whose brightness is
	// known (e.g. Uniform)
	bool AutoExposure = true;
	// EV on top of the exposure
	float ExposureCompensation = 0.0f;
	// How much it smooths jagged edges after tonemapping (FXAA, PostProcessRenderStage)
	EAntiAliasing AntiAliasing = EAntiAliasing::Medium;
	// How much creases and corners darken the indirect light (GTAO, AmbientOcclusionStage)
	EAmbientOcclusion AmbientOcclusion = EAmbientOcclusion::Medium;
	ERenderViewDebug Debug = ERenderViewDebug::Lit;
	ERenderViewLighting Lighting = ERenderViewLighting::Scene;
	// Debug lines (the editor's grid, camera frustums, bounds boxes); off for clean captures such as thumbnails
	bool DrawsDebug = true;
	// Served by the main view's next render, then cleared
	std::optional<RenderPickRequest> Pick;

private:
	ERenderViewKind m_kind;
	// Data contracts the stages publish while rendering this view; cleared at the start of each render
	RenderBlackboard m_frame;
	// State a stage keeps for this view across frames
	RenderBlackboard m_persistent;
};
