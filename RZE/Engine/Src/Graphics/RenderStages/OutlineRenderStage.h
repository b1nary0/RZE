#pragma once

#include <Graphics/RenderData/DisplayColourData.h>
#include <Graphics/RenderData/SceneDepthData.h>
#include <Graphics/RenderStage.h>

#include <EngineCore/Resources/TypedResourceHandle.h>

#include <Rendering/BufferHandle.h>

#include <Utils/PrimitiveDefs.h>

#include <vector>

class VertexShader;
class PixelShader;
struct RenderOutline;

// Draws each of a view's RenderOutlines (RenderFrameRequests::Outlines) around the outer edge of its objects, over the finished
// image. Not added by the engine: an app adds it if its views have outlines (the Editor, for its selection).
// Two passes, only for a view with outlines:
// - Mask: the outlined objects alone, into a mask the size of the view's target (OutlineMaskState, kept per view).
//   Each pixel holds which outline and which of its shapes the nearest outlined object there is in, and whether that
//   object is in front of the scene's depth or behind it. Cutout pixels are clipped as in the shadow pass.
// - Outline: a full-screen pass that draws a shape's outline colour on the pixels within its width of it that aren't
//   the shape's own, so only the edge of the shape its objects make together is outlined. Where two shapes touch, the
//   line goes on the farther one's side (by the mask's depth), as against the background. Fainter by the outline's
//   OccludedOpacity where the shape there is behind the scene.
class OutlineRenderStage : public IRenderStage
{
public:
	// How many outlines a view draws; more are ignored. The mask holds an outline's index in 8 bits.
	static constexpr U32 k_maxOutlines = 8;
	// How many shapes a view's outlines make together; the ones past it are drawn as one. The mask holds a shape's ID
	// in its top 23 bits (Common/Outline.hlsli).
	static constexpr U32 k_maxShapes = (1u << 23) - 1;
	// Widest a line is drawn, in pixels; the outline pass searches this far around each pixel
	static constexpr float k_maxWidth = 4.0f;

	// Mirrors OutlineMaskBuffer in Pixel_OutlineMask.hlsl
	struct MaskBufferLayout
	{
		// The outline's index + 1; 0 in the mask means no outlined object there
		U32 OutlineID;
		// OutlinedObject::ShapeID
		U32 ShapeID;
		U32 _pad[2];
	};

	// Mirrors OutlineStyle in Pixel_Outline.hlsl: a RenderOutline's look
	struct OutlineStyleLayout
	{
		float Colour[3];
		float Width;
		float OccludedOpacity;
		float _pad[3];
	};

	// Mirrors OutlineBuffer in Pixel_Outline.hlsl
	struct OutlineBufferLayout
	{
		OutlineStyleLayout Outlines[k_maxOutlines];
		// The view's size, which the mask's reads are clamped to (the view is the target's top-left part)
		U32 ViewportSize[2];
		// How many pixels around each one are searched: enough for the widest outline
		Int32 SearchRadius;
		U32 _pad;
	};

	// One render object an outline goes around
	struct OutlinedObject
	{
		U64 LiveID;
		// MaskBufferLayout::OutlineID
		U32 OutlineID;
		// Its shape, numbered from 1 across the view's outlines, so no two outlines' shapes share one
		U32 ShapeID;
	};

	// The objects of the first outlineCount outlines, sorted by LiveID, each once (RenderFrameRequests::Outlines), with
	// their shapes numbered across the outlines. False if they make more than k_maxShapes shapes, the ones past it then
	// taking the last ID.
	static bool CollectOutlinedObjects(const std::vector<RenderOutline>& outlines, U32 outlineCount,
		std::vector<OutlinedObject>& outObjects);

public:
	OutlineRenderStage() = default;
	~OutlineRenderStage() override = default;

public:
	const char* GetName() const override { return "OutlineRenderStage"; }

	void Initialize() override;
	void Setup(RenderStageBuilder& builder) override;
	void Render(RenderContext& context) override;

private:
	// Draws m_outlinedObjects into mask, over maskDepth; false if none is drawn
	bool DrawMask(const RenderContext& context, const Rendering::RenderTargetHandle& mask,
		const Rendering::TextureBuffer2DHandle& maskDepth);
	// Draws the lines over the view's image, from the mask and its depth
	void DrawOutlines(const RenderContext& context, U32 outlineCount, const Rendering::RenderTargetHandle& mask,
		const Rendering::TextureBuffer2DHandle& maskDepth) const;

private:
	RenderInput<SceneDepthData> m_sceneDepthInput;
	RenderInOut<DisplayColourData> m_displayColour;

	// The forward pass's, so the mask's depths are the scene depth's, bit for bit, where an object is in front
	TypedResourceHandle<VertexShader> m_meshVertexShader;
	TypedResourceHandle<VertexShader> m_quadVertexShader;
	TypedResourceHandle<PixelShader> m_maskShader;
	TypedResourceHandle<PixelShader> m_outlineShader;

	Rendering::ConstantBufferHandle m_maskBuffer;
	Rendering::ConstantBufferHandle m_outlineBuffer;

	// The objects of the view being drawn (CollectOutlinedObjects), to look each render object up in. Kept only so its
	// memory is reused.
	std::vector<OutlinedObject> m_outlinedObjects;

	// Logged once: a view asked for more outlines or shapes, or wider lines, than are drawn
	bool m_hasWarnedOfLimits = false;
};
