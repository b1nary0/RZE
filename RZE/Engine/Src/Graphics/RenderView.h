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
#include <vector>

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

// One render object a RenderOutline goes around
struct RenderOutlineObject
{
	// RenderObject::GetLiveID, so one that's gone matches nothing
	U64 LiveID = 0;
	// Which of the outline's shapes it's part of, numbered from 0 in each outline
	U32 Shape = 0;
};

// A line around the outer edge of some of the scene's objects, drawn by OutlineRenderStage over the view's finished
// image (e.g. the Editor's selection). Its objects make shapes: the objects with the same Shape make one, and only its
// outer edge is drawn, not where they overlap. Each shape's edge is drawn, also where it's in front of another of the
// view's outlined shapes.
struct RenderOutline
{
	// After tonemapping, as debug lines' are
	Vector3D Colour;
	// In pixels, up to OutlineRenderStage::k_maxWidth
	float Width = 2.0f;
	// How opaque the line is where the objects are behind the scene; 0 hides those parts
	float OccludedOpacity = 0.0f;
	// The render objects it goes around, each in one of its shapes
	std::vector<RenderOutlineObject> Objects;
};

// What a view is asked for in its next render alone. Whoever renders the view clears them afterwards
// (RenderView::ClearFrameRequests), so each frame's requests start empty and are added to.
struct RenderFrameRequests
{
	// The object at one pixel, served by PickRenderStage in the main view
	std::optional<RenderPickRequest> Pick;
	// Drawn by OutlineRenderStage, where the app adds it. An object listed more than once is outlined by the first
	// outline listing it, in the lowest of the shapes it's listed in there. Up to OutlineRenderStage::k_maxOutlines.
	std::vector<RenderOutline> Outlines;
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
	AmbientOcclusion,
	// The light arriving indirectly, from the probes or the sky, as it would light a white surface
	IndirectDiffuse
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

	// Once the view has rendered, by whoever rendered it
	void ClearFrameRequests() { Frame = RenderFrameRequests(); }

	// Destroys what stages keep for this view (e.g. its shadow maps and targets) and what the last frame published;
	// they make them again the next time it's rendered. A view that outlives the renderer has to, before it shuts down.
	void ReleaseStageState();

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
	// How it smooths jagged edges after tonemapping (AntiAliasingStage)
	AntiAliasingMode AntiAliasing;
	// How much creases and corners darken the indirect light (GTAO, AmbientOcclusionStage)
	EAmbientOcclusion AmbientOcclusion = EAmbientOcclusion::Medium;
	// Indirect light from the scene's irradiance probes (IrradianceProbeStage); off, the hemisphere ambient. Off for
	// views that render once, such as captures: probes take frames to capture and settle.
	bool GlobalIllumination = true;
	ERenderViewDebug Debug = ERenderViewDebug::Lit;
	ERenderViewLighting Lighting = ERenderViewLighting::Scene;
	// Debug lines (the editor's grid, camera frustums, bounds boxes); off for clean captures such as thumbnails
	bool DrawsDebug = true;
	// For the next render only (RenderFrameRequests)
	RenderFrameRequests Frame;

private:
	ERenderViewKind m_kind;
	// Data contracts the stages publish while rendering this view; cleared at the start of each render
	RenderBlackboard m_frameBlackboard;
	// State a stage keeps for this view across frames
	RenderBlackboard m_persistentBlackboard;
};
