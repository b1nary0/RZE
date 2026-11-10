#include <Graphics/RenderStages/OutlineRenderStage.h>

#include <Graphics/FullScreenPass.h>
#include <Graphics/IndexBuffer.h>
#include <Graphics/Material.h>
#include <Graphics/MaterialSlot.h>
#include <Graphics/MeshGeometry.h>
#include <Graphics/RenderEngine.h>
#include <Graphics/RenderView.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>
#include <Graphics/VertexBuffer.h>

#include <Rendering/Renderer.h>
#include <Rendering/Driver/GFXBuffer.h>
#include <Rendering/Graphics/RenderTarget.h>

#include <Utils/DebugUtils/Debug.h>
#include <Utils/Math/Frustum.h>
#include <Utils/Math/Vector4D.h>

#include <algorithm>
#include <cmath>
#include <tuple>

// HLSL's layouts: each style two registers, and the values after the array on a register of their own
static_assert(sizeof(OutlineRenderStage::OutlineStyleLayout) == 32, "OutlineStyle is two registers in Pixel_Outline.hlsl");
static_assert(sizeof(OutlineRenderStage::OutlineBufferLayout) == OutlineRenderStage::k_maxOutlines * 32 + 16, "OutlineBuffer in Pixel_Outline.hlsl");
// The mask holds an outline's index + 1 in 8 bits, then a bit, then a shape's ID in the 23 left (Common/Outline.hlsli)
static_assert(OutlineRenderStage::k_maxOutlines < 256, "An OutlineID must fit the mask's 8 bits");
static_assert(OutlineRenderStage::k_maxShapes == 0xFFFFFFFFu >> 9, "A ShapeID must fit the mask's top 23 bits");

namespace
{
	// Where Pixel_OutlineMask.hlsl reads the scene's depth: past the forward pass's textures, which it includes
	constexpr U32 k_sceneDepthSlot = 13;

	// The mask and its depth for one view, the size of its target
	struct OutlineMaskState : RenderStageData<OutlineMaskState>
	{
		[[maybe_unused]] static constexpr const char* k_name = "OutlineMaskState";

		// Per pixel, the nearest outlined object's OutlineID and whether it's in front of the scene (Common/Outline.hlsli)
		Rendering::RenderTargetHandle Mask;
		// Only for which outlined object is nearest, so the mask's own
		Rendering::TextureBuffer2DHandle Depth;
		U32 Width = 0;
		U32 Height = 0;
	};

	void CreateMaskTextures(OutlineMaskState& state)
	{
		Rendering::GFXTextureBufferParams maskParams = { 0 };
		maskParams.Width = state.Width;
		maskParams.Height = state.Height;
		maskParams.MipLevels = 1;
		maskParams.SampleCount = 1;
		maskParams.Format = Rendering::ETextureFormat::R32_UINT;
		maskParams.bIsRenderTarget = true;
		maskParams.bIsShaderResource = true;
		state.Mask = Rendering::Renderer::CreateTextureBuffer2D(maskParams);

		Rendering::GFXTextureBufferParams depthParams = { 0 };
		depthParams.Width = state.Width;
		depthParams.Height = state.Height;
		depthParams.bIsDepthTexture = true;
		// Also gives it the SRV that every texture buffer is expected to have
		depthParams.bIsShaderResource = true;
		state.Depth = Rendering::Renderer::CreateTextureBuffer2D(depthParams);
	}
}

void OutlineRenderStage::Initialize()
{
	// Same layout as ForwardRenderStage, whose vertex shader the mask is drawn with
	Rendering::ShaderInputLayout meshInputLayout =
	{
		{ "POSITION", Rendering::EDataFormat::R32G32B32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 0 },
		{ "NORMAL", Rendering::EDataFormat::R32G32B32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 12 },
		{ "UV", Rendering::EDataFormat::R32G32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 24 },
		{ "TANGENT", Rendering::EDataFormat::R32G32B32_FLOAT, Rendering::EDataClassification::PER_VERTEX, 32 }
	};
	m_meshVertexShader = LoadVertexShader("Vertex_NewRenderer", meshInputLayout);

	Rendering::ShaderInputLayout quadInputLayout = {};
	m_quadVertexShader = LoadVertexShader("Vertex_RenderTargetQuad", quadInputLayout);

	m_maskShader = LoadPixelShader("Pixel_OutlineMask");
	m_outlineShader = LoadPixelShader("Pixel_Outline");

	m_maskBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(MaskBufferLayout), 16, 1);
	m_outlineBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(OutlineBufferLayout), 16, 1);
}

void OutlineRenderStage::Setup(RenderStageBuilder& builder)
{
	m_sceneDepthInput = builder.Reads<SceneDepthData>();
	m_displayColour = builder.Modifies<DisplayColourData>();
}

void OutlineRenderStage::Render(RenderContext& context)
{
	OPTICK_EVENT();

	const RenderView& view = context.View;
	const std::vector<RenderOutline>& outlines = view.Frame.Outlines;
	if (outlines.empty())
	{
		return;
	}

	const U32 outlineCount = std::min(static_cast<U32>(outlines.size()), k_maxOutlines);
	const bool hasAllShapes = CollectOutlinedObjects(outlines, outlineCount, m_outlinedObjects);
	const bool isOverLimits = !hasAllShapes || outlines.size() > k_maxOutlines
		|| std::any_of(outlines.begin(), outlines.end(), [](const RenderOutline& outline) { return outline.Width > k_maxWidth; });
	if (isOverLimits && !m_hasWarnedOfLimits)
	{
		RZE_LOG_WARNING_ARGS("A view has %zu outlines, one wider than %.0f pixels, or more than %u shapes: only the first %u "
			"outlines are drawn, at most that wide, and the shapes past the last as one.",
			outlines.size(), k_maxWidth, k_maxShapes, k_maxOutlines);
		m_hasWarnedOfLimits = true;
	}
	if (m_outlinedObjects.empty())
	{
		return;
	}

	Rendering::Renderer::Begin("OutlineRenderStage");

	const Rendering::RenderTargetTexture& renderTarget = *view.Target;
	OutlineMaskState& state = context.Persistent().GetOrCreate<OutlineMaskState>([]() { return OutlineMaskState(); });
	
	// Handle resizing, so the mask and its depth are the same size as the view's target
	if (state.Width != renderTarget.GetWidth() || state.Height != renderTarget.GetHeight())
	{
		state.Width = renderTarget.GetWidth();
		state.Height = renderTarget.GetHeight();
		CreateMaskTextures(state);
	}

	if (DrawMask(context, state.Mask, state.Depth))
	{
		DrawOutlines(context, outlineCount, state.Mask, state.Depth);
	}

	Rendering::Renderer::End();
}

bool OutlineRenderStage::CollectOutlinedObjects(const std::vector<RenderOutline>& outlines, U32 outlineCount,
	std::vector<OutlinedObject>& outObjects)
{
	AssertExpr(outlineCount <= outlines.size());

	outObjects.clear();
	bool hasAllShapes = true;
	// Each outline's shapes are numbered after the one before's. 64 bits, as a Shape can be any U32.
	U64 firstShapeID = 1;
	for (U32 outlineIndex = 0; outlineIndex < outlineCount; ++outlineIndex)
	{
		U64 shapeCount = 0;
		for (const RenderOutlineObject& object : outlines[outlineIndex].Objects)
		{
			const U64 shapeID = firstShapeID + object.Shape;
			hasAllShapes = hasAllShapes && shapeID <= k_maxShapes;
			outObjects.push_back({ object.LiveID, outlineIndex + 1, static_cast<U32>(std::min<U64>(shapeID, k_maxShapes)) });
			shapeCount = std::max<U64>(shapeCount, static_cast<U64>(object.Shape) + 1);
		}
		firstShapeID += shapeCount;
	}

	// Each object once: in the first outline listing it, then the lowest shape it's listed in there
	std::sort(outObjects.begin(), outObjects.end(), [](const OutlinedObject& a, const OutlinedObject& b)
		{
			return std::tie(a.LiveID, a.OutlineID, a.ShapeID) < std::tie(b.LiveID, b.OutlineID, b.ShapeID);
		});
	outObjects.erase(std::unique(outObjects.begin(), outObjects.end(),
		[](const OutlinedObject& a, const OutlinedObject& b) { return a.LiveID == b.LiveID; }), outObjects.end());
	return hasAllShapes;
}

bool OutlineRenderStage::DrawMask(const RenderContext& context, const Rendering::RenderTargetHandle& mask,
	const Rendering::TextureBuffer2DHandle& maskDepth)
{
	// Render returns earlier for a view with nothing to outline
	AssertExpr(!m_outlinedObjects.empty());

	const RenderView& view = context.View;

	// Its own depth, so where outlined objects overlap the nearest one's outline is drawn
	Rendering::Renderer::SetColourTarget(mask, maskDepth);
	Rendering::Renderer::ClearRenderTarget(mask, Vector4D(0.0f, 0.0f, 0.0f, 0.0f));
	Rendering::Renderer::ClearDepthStencilBuffer(maskDepth);
	// After the targets, as it's the view's depth target until then
	Rendering::Renderer::SetTextureResource(m_sceneDepthInput.Get(context).Depth, k_sceneDepthSlot);

	// The forward pass and the prepass upload the same camera to the same buffer
	Rendering::Renderer::UploadDataToBuffer<RenderCamera>(m_meshVertexShader->GetCameraDataBuffer(), &view.Camera);

	Rendering::Renderer::SetVertexShader(m_meshVertexShader->GetPlatformObject());
	Rendering::Renderer::SetPixelShader(m_maskShader->GetPlatformObject());
	Rendering::Renderer::SetConstantBufferVS(m_meshVertexShader->GetCameraDataBuffer(), 0);
	Rendering::Renderer::SetConstantBufferVS(m_meshVertexShader->GetWorldMatrixBuffer(), 1);
	Rendering::Renderer::SetConstantBufferPS(m_maskBuffer, 0);

	Rendering::Renderer::SetViewport({ view.ViewportSize.X(), view.ViewportSize.Y() });
	Rendering::Renderer::SetInputLayout(m_meshVertexShader->GetPlatformObject());
	Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::TriangleList);

	// As in ForwardRenderStage: only uploaded when the draw's transform changes
	const Matrix4x4* uploadedTransform = nullptr;
	const Frustum frustum(view.Camera.ClipSpace);
	bool hasDrawn = false;

	for (const auto& renderObject : context.Scene.GetRenderObjects())
	{
		const U64 liveID = renderObject->GetLiveID();
		const auto outlined = std::lower_bound(m_outlinedObjects.begin(), m_outlinedObjects.end(), liveID,
			[](const OutlinedObject& object, U64 id) { return object.LiveID < id; });
		if (outlined == m_outlinedObjects.end() || outlined->LiveID != liveID || !frustum.Intersects(renderObject->GetBounds()))
		{
			continue;
		}

		const MaskBufferLayout maskData = { outlined->OutlineID, outlined->ShapeID, { 0, 0 } };
		Rendering::Renderer::UploadDataToBuffer<MaskBufferLayout>(m_maskBuffer, &maskData);

		for (const MeshDraw& draw : renderObject->GetDraws())
		{
			if (!frustum.Intersects(draw.Bounds))
			{
				continue;
			}

			// See ForwardRenderStage: uploads the whole ObjectBufferLayout (transform + inverse)
			if (draw.Transform != uploadedTransform)
			{
				Rendering::Renderer::UploadDataToBuffer<Matrix4x4>(m_meshVertexShader->GetWorldMatrixBuffer(), draw.Transform);
				uploadedTransform = draw.Transform;
			}

			const MeshGeometry& meshGeometry = *draw.Geometry;
			const MaterialInstance& materialInstance = draw.Slot->GetMaterial();

			// For cutout materials, as in PickRenderStage: the outline follows the cut-out shape
			Rendering::Renderer::SetConstantBufferPS(materialInstance.GetParamBuffer(), 1);
			if (const Texture2D* const opacityMap = materialInstance.GetTextureResource(TEXTURE_SLOT_OPACITY))
			{
				Rendering::Renderer::SetTextureResource(opacityMap->GetPlatformObject(), TEXTURE_SLOT_OPACITY);
			}

			const Rendering::IndexBufferHandle& indexBuffer = meshGeometry.GetIndexBuffer()->GetPlatformObject();
			Rendering::Renderer::SetVertexBuffer(meshGeometry.GetVertexBuffer()->GetPlatformObject(), 0);
			Rendering::Renderer::SetIndexBuffer(indexBuffer);

			Rendering::Renderer::DrawIndexed(indexBuffer);
			hasDrawn = true;
		}
	}

	// Later stages bind it as a depth target again
	Rendering::Renderer::UnsetTextureResource(k_sceneDepthSlot);
	return hasDrawn;
}

void OutlineRenderStage::DrawOutlines(const RenderContext& context, U32 outlineCount, const Rendering::RenderTargetHandle& mask,
	const Rendering::TextureBuffer2DHandle& maskDepth) const
{
	const RenderView& view = context.View;
	const std::vector<RenderOutline>& outlines = view.Frame.Outlines;

	OutlineBufferLayout params = {};
	float widest = 0.0f;
	for (U32 outlineIndex = 0; outlineIndex < outlineCount; ++outlineIndex)
	{
		const RenderOutline& outline = outlines[outlineIndex];
		OutlineStyleLayout& style = params.Outlines[outlineIndex];
		style.Colour[0] = outline.Colour.X();
		style.Colour[1] = outline.Colour.Y();
		style.Colour[2] = outline.Colour.Z();
		style.Width = std::clamp(outline.Width, 0.0f, k_maxWidth);
		style.OccludedOpacity = std::clamp(outline.OccludedOpacity, 0.0f, 1.0f);
		widest = std::max(widest, style.Width);
	}
	params.ViewportSize[0] = static_cast<U32>(view.ViewportSize.X());
	params.ViewportSize[1] = static_cast<U32>(view.ViewportSize.Y());
	// The line's edge fades over the half pixel past its width (Pixel_Outline.hlsl)
	params.SearchRadius = static_cast<Int32>(std::ceil(widest + 0.5f));

	Rendering::Renderer::UploadDataToBuffer<OutlineBufferLayout>(m_outlineBuffer, &params);
	Rendering::Renderer::SetConstantBufferPS(m_outlineBuffer, 0);

	Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::TriangleStrip);
	Rendering::Renderer::SetVertexShader(m_quadVertexShader->GetPlatformObject());
	Rendering::Renderer::SetViewport({ view.ViewportSize.X(), view.ViewportSize.Y() });
	Rendering::Renderer::SetColourTarget(m_displayColour.Get(context).Colour);

	// The line's soft edge blends with the image under it. The mask's depth is free to read: the colour target has none.
	Rendering::Renderer::SetBlendState(Rendering::EBlendState::AlphaBlend);
	DrawFullScreenPass(m_outlineShader, { mask, maskDepth });
	Rendering::Renderer::SetBlendState(Rendering::EBlendState::Opaque);
}
