#include <Graphics/RenderStages/ForwardRenderStage.h>

#include <Graphics/GraphicsLayouts.h>
#include <Graphics/IndexBuffer.h>
#include <Graphics/Material.h>
#include <Graphics/MaterialDefines.h>
#include <Graphics/MaterialSlot.h>
#include <Graphics/RenderEngine.h>
#include <Graphics/Shader.h>
#include <Graphics/Texture2D.h>
#include <Graphics/VertexBuffer.h>

#include <Rendering/Renderer.h>
#include <Rendering/Graphics/RenderTarget.h>

#include <Utils/Math/Frustum.h>

#include <algorithm>

namespace
{
	// Must match LightBuffer / ShadowBuffer / ViewLightingBuffer / IrradianceVolume / ShadowMap / PointShadowCubes /
	// AmbientOcclusionMap / ProbeSH / ReflectionProbes registers in Common/PixelResources.hlsli and the files it includes
	constexpr U32 k_lightBufferSlot = 2;
	constexpr U32 k_shadowBufferSlot = 3;
	constexpr U32 k_viewLightingBufferSlot = 4;
	constexpr U32 k_irradianceVolumeSlot = 5;
	constexpr U32 k_shadowMapSlot = 5;
	constexpr U32 k_pointShadowCubesSlot = 6;
	constexpr U32 k_ambientOcclusionSlot = 7;
	constexpr U32 k_probeSHRedSlot = 8;
	constexpr U32 k_probeSHGreenSlot = 9;
	constexpr U32 k_probeSHBlueSlot = 10;
	constexpr U32 k_probeVisibilitySlot = 11;
	constexpr U32 k_reflectionProbesSlot = 7;
	constexpr U32 k_reflectionCubesSlot = 12;
}

void ForwardRenderStage::Initialize()
{
	// Places vertices as DepthPrepassStage's shader does, to the bit (MeshClipPosition in Common/MeshVertex.hlsli)
	m_vertexShader = LoadVertexShader("Vertex_MeshInstanced", MeshInstanceFormats::Forward().InputLayout);

	m_viewLightingBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(GraphicsLayouts::ViewLightingLayout), 16, 1);

	m_batches.Initialize();
}

void ForwardRenderStage::Setup(RenderStageBuilder& builder)
{
	m_sceneLightingInput = builder.Reads<SceneLightingData>();
	m_sceneDepthInput = builder.Reads<SceneDepthData>();
	m_ambientOcclusionInput = builder.Reads<AmbientOcclusionData>();
	m_sceneIrradianceInput = builder.Reads<SceneIrradianceData>();
	m_sceneReflectionInput = builder.Reads<SceneReflectionData>();
	m_sceneColourOutput = builder.Writes<SceneColourData>();
}

void ForwardRenderStage::Render(RenderContext& context)
{
	OPTICK_EVENT();

	Rendering::Renderer::Begin("ForwardRenderStage");

	const RenderView& view = context.View;
	const RenderScene& scene = context.Scene;
	const SceneLightingData& sceneLightingData = m_sceneLightingInput.Get(context);

	// Linear scene colour; PostProcessRenderStage exposes and tonemaps it into the 8-bit target.
	// Alpha 0 marks the background, which it fills with the clear colour.
	SceneColourData sceneColourData;
	sceneColourData.Colour = view.Target->GetSceneTargetPlatformObject();
	// Already holds everything that isn't blended (DepthPrepassStage), so it isn't cleared
	sceneColourData.Depth = m_sceneDepthInput.Get(context).Depth;

	Rendering::Renderer::SetColourTarget(sceneColourData.Colour, sceneColourData.Depth);
	Rendering::Renderer::ClearRenderTarget(sceneColourData.Colour, Vector4D(0.0f, 0.0f, 0.0f, 0.0f));

	// Each pixel passes only for the surface the prepass left nearest there, so it's shaded once. Nothing needs
	// writing: the prepass wrote the same depths.
	Rendering::Renderer::SetDepthState(Rendering::EDepthState::ReadOnlyLessEqual);

	Rendering::Renderer::UploadDataToBuffer<RenderCamera>(m_vertexShader->GetCameraDataBuffer(), &view.Camera);

	GraphicsLayouts::ViewLightingLayout viewLighting;
	viewLighting.UniformLighting = view.Lighting == ERenderViewLighting::Uniform ? 1.0f : 0.0f;
	viewLighting.ShowIndirectDiffuse = view.Debug == ERenderViewDebug::IndirectDiffuse ? 1.0f : 0.0f;
	Rendering::Renderer::UploadDataToBuffer<GraphicsLayouts::ViewLightingLayout>(m_viewLightingBuffer, &viewLighting);

	Rendering::Renderer::SetVertexShader(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetConstantBufferVS(m_vertexShader->GetCameraDataBuffer(), 0);

	Rendering::Renderer::SetViewport({ view.ViewportSize.X(), view.ViewportSize.Y() });

	Rendering::Renderer::SetInputLayout(m_vertexShader->GetPlatformObject());
	Rendering::Renderer::SetPrimitiveTopology(Rendering::EPrimitiveTopology::TriangleList);

	Rendering::Renderer::SetConstantBufferPS(sceneLightingData.ShadowBuffer, k_shadowBufferSlot);
	Rendering::Renderer::SetTextureResource(sceneLightingData.DirectionalShadowMap, k_shadowMapSlot);
	Rendering::Renderer::SetTextureResource(sceneLightingData.PointShadowCubes, k_pointShadowCubesSlot);
	const AmbientOcclusionData& ambientOcclusion = m_ambientOcclusionInput.Get(context);
	Rendering::Renderer::SetTextureResource(ambientOcclusion.Visibility, k_ambientOcclusionSlot);
	const SceneIrradianceData& irradiance = m_sceneIrradianceInput.Get(context);
	Rendering::Renderer::SetConstantBufferPS(irradiance.VolumeBuffer, k_irradianceVolumeSlot);
	Rendering::Renderer::SetTextureResource(irradiance.SHRed, k_probeSHRedSlot);
	Rendering::Renderer::SetTextureResource(irradiance.SHGreen, k_probeSHGreenSlot);
	Rendering::Renderer::SetTextureResource(irradiance.SHBlue, k_probeSHBlueSlot);
	Rendering::Renderer::SetTextureResource(irradiance.Visibility, k_probeVisibilitySlot);
	const SceneReflectionData& reflection = m_sceneReflectionInput.Get(context);
	Rendering::Renderer::SetConstantBufferPS(reflection.ProbeBuffer, k_reflectionProbesSlot);
	Rendering::Renderer::SetTextureResource(reflection.Cubes, k_reflectionCubesSlot);

	// The same for every draw: bound once
	Rendering::Renderer::SetConstantBufferPS(sceneLightingData.LightBuffer, k_lightBufferSlot);
	Rendering::Renderer::SetConstantBufferPS(m_viewLightingBuffer, k_viewLightingBufferSlot);

	// Objects and draws entirely outside the view aren't drawn
	const Frustum frustum(view.Camera.ClipSpace);

	m_batches.Clear();
	m_transparentDraws.clear();

	scene.ForEachDrawIn(frustum, ERenderMobilityFilter::Any, [this, &view](const MeshDraw& draw)
	{
		const MaterialInstance& materialInstance = draw.Slot->GetMaterial();

		// Blended draws wait until everything they could show is drawn
		if (materialInstance.IsBlended())
		{
			TransparentDraw& transparentDraw = m_transparentDraws.emplace_back();
			transparentDraw.DistanceSquared = (draw.Bounds.GetCentre() - view.Camera.Position).LengthSq();
			transparentDraw.Draw = &draw;
			return;
		}

		// The material decides everything bound besides the geometry, its shader included
		m_batches.Add(draw, &materialInstance);
	});

	// After everything opaque, farthest first, each on its own so the order holds
	SortBackToFront(m_transparentDraws);
	for (const TransparentDraw& transparentDraw : m_transparentDraws)
	{
		m_batches.AddUngrouped(*transparentDraw.Draw);
	}

	m_batches.Upload();
	m_batches.BindInstances();

	// Consecutive batches often share a shader, so it's only set when it changes
	const PixelShader* boundPixelShader = nullptr;
	// The blended batches come last, so the first one switches to blending for the rest
	bool isBlending = false;
	// Sorted by distance, not by mode, so the blend state is only set when it changes from the last batch's
	Rendering::EBlendState boundBlendState = Rendering::EBlendState::Opaque;

	for (const MeshBatches::Batch& batch : m_batches.GetBatches())
	{
		const MaterialInstance& materialInstance = batch.Draw->Slot->GetMaterial();
		if (materialInstance.IsBlended())
		{
			if (!isBlending)
			{
				// Blended over what's behind, and tested against depth without writing it, so one see-through surface
				// doesn't hide another behind it. Strictly nearer, as before the prepass: a surface at exactly the
				// depth of the opaque one behind it stays hidden.
				// #TODO(Transparency) Over empty background (scene alpha 0) the blend leaves alpha 0, so PostProcess
				// paints the background colour there and the surface vanishes. Needs the background handled before
				// blending.
				Rendering::Renderer::SetDepthState(Rendering::EDepthState::ReadOnly);
				// The ambient occlusion is of what's behind them: they aren't in the depth it was computed from
				Rendering::Renderer::SetTextureResource(ambientOcclusion.Unoccluded, k_ambientOcclusionSlot);
				isBlending = true;
			}

			const Rendering::EBlendState blendState = ToBlendState(materialInstance.GetBlendMode());
			if (blendState != boundBlendState)
			{
				Rendering::Renderer::SetBlendState(blendState);
				boundBlendState = blendState;
			}
		}

		DrawBatch(batch, boundPixelShader);
	}

	if (isBlending)
	{
		Rendering::Renderer::SetBlendState(Rendering::EBlendState::Opaque);
	}

	Rendering::Renderer::SetDepthState(Rendering::EDepthState::ReadWrite);

	// The shadow maps can't stay bound as shader inputs while the next frame renders depth into them, nor the ambient
	// occlusion and the probes while the next frame's are written
	Rendering::Renderer::UnsetTextureResource(k_shadowMapSlot);
	Rendering::Renderer::UnsetTextureResource(k_pointShadowCubesSlot);
	Rendering::Renderer::UnsetTextureResource(k_ambientOcclusionSlot);
	Rendering::Renderer::UnsetTextureResource(k_probeSHRedSlot);
	Rendering::Renderer::UnsetTextureResource(k_probeSHGreenSlot);
	Rendering::Renderer::UnsetTextureResource(k_probeSHBlueSlot);
	Rendering::Renderer::UnsetTextureResource(k_probeVisibilitySlot);
	Rendering::Renderer::UnsetTextureResource(k_reflectionCubesSlot);

	m_sceneColourOutput.Publish(context, sceneColourData);

	Rendering::Renderer::End();
}

void ForwardRenderStage::SortBackToFront(std::vector<TransparentDraw>& draws)
{
	std::sort(draws.begin(), draws.end(), [](const TransparentDraw& a, const TransparentDraw& b)
		{
			return a.DistanceSquared > b.DistanceSquared;
		});
}

void ForwardRenderStage::DrawBatch(const MeshBatches::Batch& batch, const PixelShader*& boundPixelShader)
{
	const MeshDraw& draw = *batch.Draw;
	const MeshGeometry& meshGeometry = *draw.Geometry;

	// @TODO
	// This is god awful. Just in place while developing shader model.
	// Should get resolved once the system matures
	const MaterialInstance& materialInstance = draw.Slot->GetMaterial();
	const PixelShader* const pixelShader = materialInstance.GetPixelShader();

	if (pixelShader != boundPixelShader)
	{
		Rendering::Renderer::SetPixelShader(pixelShader->GetPlatformObject());
		boundPixelShader = pixelShader;
	}
	Rendering::Renderer::SetConstantBufferPS(materialInstance.GetParamBuffer(), 1);

	// @TODO Really need to get to texture infrastructure refactor soon - 2/6/2022
	for (U8 textureSlot = 0; textureSlot < TEXTURE_SLOT_COUNT; ++textureSlot)
	{
		if (const Texture2D* const texture = materialInstance.GetTextureResource(textureSlot))
		{
			// @TODO Should solve this better by providing an API that will provide a texture resource array
			Rendering::Renderer::SetTextureResource(texture->GetPlatformObject(), textureSlot);
		}
	}

	const Rendering::IndexBufferHandle& indexBuffer = meshGeometry.GetIndexBuffer()->GetPlatformObject();
	Rendering::Renderer::SetVertexBuffer(meshGeometry.GetVertexBuffer()->GetPlatformObject(), 0);
	Rendering::Renderer::SetIndexBuffer(indexBuffer);

	Rendering::Renderer::DrawIndexedInstanced(indexBuffer, batch.InstanceCount, batch.FirstInstance);
}
