#include <StdAfx.h>
#include <Graphics/RenderStages/IrradianceProbeStage.h>

#include <Graphics/GraphicsLayouts.h>
#include <Graphics/IrradianceProbeGrid.h>
#include <Graphics/RenderEngine.h>
#include <Graphics/Shader.h>

#include <Rendering/Renderer.h>
#include <Rendering/Driver/GFXBuffer.h>

#include <Utils/Math/BoundingBox.h>

#include <algorithm>
#include <cmath>

namespace
{
	// Texels across a capture face: 6 x 64 = 384 samples a probe, about what DDGI traces per probe
	constexpr U32 k_faceSize = 8;
	constexpr U32 k_probeWidth = k_faceSize * ProbeCapture::k_faceCount;
	// #TODO(Probes) From Engine.ini's [Rendering] ProbeTargetCount, with the capture budget (step 8)
	constexpr U32 k_targetProbeCount = 4096;
	// Probes captured a frame, side by side in the scratch target, then copied into the atlas
	constexpr U32 k_capturesPerFrame = 64;
	// Probes relit a frame, round-robin. #TODO(Probes) Nearest the camera first, on a time budget (step 8)
	constexpr U32 k_relightsPerFrame = 512;
	// The atlas has a row of probes per k_faceSize texels, as many to a row as a frame captures
	constexpr U32 k_atlasProbesPerRow = k_capturesPerFrame;
	constexpr U32 k_atlasWidth = k_atlasProbesPerRow * k_probeWidth;
	constexpr U32 k_atlasHeight = (IrradianceProbeGrid::k_maxProbes + k_atlasProbesPerRow - 1) / k_atlasProbesPerRow * k_faceSize;
	// How far along its normal a surface reads the probes, in spacings
	constexpr float k_normalBiasSpacings = 0.25f;
	// Each probe's visibility map: texels a side, plus a border all round (VISIBILITY_SIZE in Common/IrradianceVolume.hlsli)
	constexpr U32 k_visibilitySize = 8;
	constexpr U32 k_visibilityStride = k_visibilitySize + 2;
	constexpr U32 k_visibilityProbesPerRow = 64;
	constexpr U32 k_visibilityAtlasWidth = k_visibilityProbesPerRow * k_visibilityStride;
	constexpr U32 k_visibilityAtlasHeight = (IrradianceProbeGrid::k_maxProbes + k_visibilityProbesPerRow - 1) / k_visibilityProbesPerRow * k_visibilityStride;
	// Distances in the visibility maps are clamped to this many spacings: further than any surface a probe lights
	// (the far corner of its cell is about 1.7 spacings away), so what matters is told apart
	constexpr float k_visibilityMaxDistanceSpacings = 2.0f;

	// Must match the registers in Compute_ProbeRelight.hlsl and Common/LightResources.hlsli
	constexpr U32 k_lightBufferSlot = 2;
	constexpr U32 k_shadowBufferSlot = 3;
	constexpr U32 k_volumeBufferSlot = 5;
	constexpr U32 k_relightParamsSlot = 6;
	constexpr U32 k_shadowMapSlot = 5;
	constexpr U32 k_pointShadowCubesSlot = 6;
	constexpr U32 k_shRedSlot = 8;
	constexpr U32 k_visibilitySlot = 11;
	constexpr U32 k_captureAtlasSlot = 13;

	// A scene's probes, what they've captured and their light, kept in its ScenePersistent() blackboard
	struct IrradianceProbeState : RenderStageData<IrradianceProbeState>
	{
		static constexpr const char* k_name = "IrradianceProbeState";

		IrradianceProbeGrid Grid;
		// What Grid was fitted to
		BoundingBox FittedBounds;
		// Every probe's capture, at its index's place (k_atlasProbesPerRow to a row of k_faceSize texels)
		Rendering::TextureBuffer2DHandle Atlas;
		// One frame's captures, side by side, and their depth
		Rendering::TextureBuffer2DHandle Scratch;
		Rendering::TextureBuffer2DHandle ScratchDepth;
		// The probes' spherical harmonics, a texture array per channel (red, green, blue), and last frame's, which the
		// relighting reads while it writes these. Made for Grid.
		Rendering::TextureBuffer2DHandle SH[3];
		Rendering::TextureBuffer2DHandle PreviousSH[3];
		// Every probe's visibility map, at its index's place (k_visibilityProbesPerRow to a row)
		Rendering::TextureBuffer2DHandle Visibility;
		// The first probe not yet captured
		U32 NextCapture = 0;
		// The first probe of the next relighting batch, once every probe is captured
		U32 NextRelight = 0;
		// Probes 0 to this - 1 have been relit since they were captured (IrradianceVolumeLayout::ValidProbeCount)
		U32 RelitCount = 0;
		// Since the captures last restarted, for the log when they finish
		U32 CaptureFrames = 0;
		U64 CaptureDraws = 0;
	};

	Rendering::TextureBuffer2DHandle CreateCaptureTexture(U32 width, U32 height, bool isRenderTarget)
	{
		Rendering::GFXTextureBufferParams params = { 0 };
		params.bIsShaderResource = true;
		params.bIsRenderTarget = isRenderTarget;
		params.Width = width;
		params.Height = height;
		params.MipLevels = 1;
		params.SampleCount = 1;
		params.Format = Rendering::ETextureFormat::RG32_UINT;
		return Rendering::Renderer::CreateTextureBuffer2D(params);
	}

	Rendering::TextureBuffer2DHandle CreateDepthTexture(U32 width, U32 height)
	{
		Rendering::GFXTextureBufferParams params = { 0 };
		params.bIsDepthTexture = true;
		params.Width = width;
		params.Height = height;
		params.MipLevels = 1;
		params.SampleCount = 1;
		return Rendering::Renderer::CreateTextureBuffer2D(params);
	}

	// A probe a texel, a slice per layer of the grid. Always an array (at least 2 slices), as the shaders read one.
	Rendering::TextureBuffer2DHandle CreateSHTexture(U32 width, U32 height, U32 slices)
	{
		Rendering::GFXTextureBufferParams params = { 0 };
		params.bIsShaderResource = true;
		params.bIsUnorderedAccess = true;
		params.Width = width;
		params.Height = height;
		params.ArraySize = std::max(slices, 2u);
		params.MipLevels = 1;
		params.SampleCount = 1;
		params.Format = Rendering::ETextureFormat::RGBA16_FLOAT;
		return Rendering::Renderer::CreateTextureBuffer2D(params);
	}

	// Mean and mean squared distance, written by Compute_ProbeVisibility and sampled with filtering
	Rendering::TextureBuffer2DHandle CreateVisibilityTexture(U32 width, U32 height)
	{
		Rendering::GFXTextureBufferParams params = { 0 };
		params.bIsShaderResource = true;
		params.bIsUnorderedAccess = true;
		params.Width = width;
		params.Height = height;
		params.MipLevels = 1;
		params.SampleCount = 1;
		params.Format = Rendering::ETextureFormat::RG16_FLOAT;
		return Rendering::Renderer::CreateTextureBuffer2D(params);
	}

	IrradianceProbeState CreateState()
	{
		IrradianceProbeState state;
		state.Atlas = CreateCaptureTexture(k_atlasWidth, k_atlasHeight, false);
		state.Visibility = CreateVisibilityTexture(k_visibilityAtlasWidth, k_visibilityAtlasHeight);
		state.Scratch = CreateCaptureTexture(k_atlasWidth, k_faceSize, true);
		state.ScratchDepth = CreateDepthTexture(k_atlasWidth, k_faceSize);
		return state;
	}

	Vector3D GetProbePosition(const IrradianceProbeGrid& grid, U32 probe)
	{
		const U32 x = probe % grid.CountX;
		const U32 y = (probe / grid.CountX) % grid.CountY;
		const U32 z = probe / (grid.CountX * grid.CountY);
		return grid.GetProbePosition(x, y, z);
	}

	GraphicsLayouts::IrradianceVolumeLayout MakeVolumeLayout(const IrradianceProbeGrid& grid, U32 validProbeCount)
	{
		GraphicsLayouts::IrradianceVolumeLayout layout;
		layout.Origin[0] = grid.Origin.X();
		layout.Origin[1] = grid.Origin.Y();
		layout.Origin[2] = grid.Origin.Z();
		layout.Spacing = grid.Spacing;
		layout.Counts[0] = grid.CountX;
		layout.Counts[1] = grid.CountY;
		layout.Counts[2] = grid.CountZ;
		layout.ValidProbeCount = validProbeCount;
		layout.NormalBias = grid.Spacing * k_normalBiasSpacings;
		layout.VisibilityProbesPerRow = k_visibilityProbesPerRow;
		layout.VisibilityMaxDistance = grid.Spacing * k_visibilityMaxDistanceSpacings;
		return layout;
	}
}

IrradianceProbeStage::IrradianceProbeStage()
	: m_capture(k_faceSize)
{
}

void IrradianceProbeStage::Initialize()
{
	m_capture.Initialize();
	m_relightShader = LoadComputeShader("Compute_ProbeRelight");
	m_visibilityShader = LoadComputeShader("Compute_ProbeVisibility");
	m_relightParamsBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(RelightParamsLayout), 16, 1);
	m_volumeBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(GraphicsLayouts::IrradianceVolumeLayout), 16, 1);
	m_placeholderSH = CreateSHTexture(1, 1, 2);
	m_placeholderVisibility = CreateVisibilityTexture(1, 1);
}

void IrradianceProbeStage::Setup(RenderStageBuilder& builder)
{
	m_sceneLightingInput = builder.Reads<SceneLightingData>();
	m_sceneIrradianceOutput = builder.Writes<SceneIrradianceData>();
}

void IrradianceProbeStage::Render(RenderContext& context)
{
	OPTICK_EVENT();

	const RenderView& view = context.View;
	const RenderScene& scene = context.Scene;

	SceneIrradianceData irradiance;
	irradiance.VolumeBuffer = m_volumeBuffer;
	irradiance.SHRed = m_placeholderSH;
	irradiance.SHGreen = m_placeholderSH;
	irradiance.SHBlue = m_placeholderSH;
	irradiance.Visibility = m_placeholderVisibility;
	GraphicsLayouts::IrradianceVolumeLayout volume;

	// #TODO(Probes) Once per scene per frame, whichever view renders it (step 8). For now the main view does the work,
	// and other views of its scene read what it made.
	const bool updates = view.IsMainView() && view.Lighting == ERenderViewLighting::Scene;
	const IrradianceProbeState* existingState = context.ScenePersistent().TryGet<IrradianceProbeState>();
	if (updates || (existingState != nullptr && view.Lighting == ERenderViewLighting::Scene))
	{
		IrradianceProbeState& state = context.ScenePersistent().GetOrCreate<IrradianceProbeState>(&CreateState);
		Rendering::Renderer::Begin("IrradianceProbeStage");

		if (updates)
		{
			// #TODO(Probes) Refit only when the bounds move by more than a spacing, and recapture near changed objects (step 8)
			const BoundingBox& bounds = scene.GetBounds();
			if (bounds != state.FittedBounds)
			{
				state.FittedBounds = bounds;
				state.Grid = IrradianceProbeGrid::Fit(bounds, k_targetProbeCount);
				state.NextCapture = 0;
				state.NextRelight = 0;
				state.RelitCount = 0;
				state.CaptureFrames = 0;
				state.CaptureDraws = 0;
				for (int channel = 0; channel < 3; ++channel)
				{
					state.SH[channel] = state.Grid.IsEmpty() ? Rendering::TextureBuffer2DHandle() : CreateSHTexture(state.Grid.CountX, state.Grid.CountY, state.Grid.CountZ);
					state.PreviousSH[channel] = state.Grid.IsEmpty() ? Rendering::TextureBuffer2DHandle() : CreateSHTexture(state.Grid.CountX, state.Grid.CountY, state.Grid.CountZ);
				}
			}

			const U32 probeCount = state.Grid.GetProbeCount();
			if (state.NextCapture < probeCount)
			{
				// Far enough to see across the whole grid from any probe in it
				const float gridDiagonal = std::sqrt(static_cast<float>(state.Grid.CountX * state.Grid.CountX + state.Grid.CountY * state.Grid.CountY + state.Grid.CountZ * state.Grid.CountZ));
				const float farDistance = (gridDiagonal + 1.0f) * state.Grid.Spacing;

				Rendering::Renderer::SetColourTarget(state.Scratch, state.ScratchDepth);
				Rendering::Renderer::ClearRenderTarget(state.Scratch, Vector4D(0.0f, 0.0f, 0.0f, 0.0f));
				Rendering::Renderer::ClearDepthStencilBuffer(state.ScratchDepth);

				const U32 batchCount = std::min(k_capturesPerFrame, probeCount - state.NextCapture);
				for (U32 slot = 0; slot < batchCount; ++slot)
				{
					const U32 probe = state.NextCapture + slot;
					state.CaptureDraws += m_capture.Capture(scene, GetProbePosition(state.Grid, probe), farDistance, slot * k_probeWidth, 0);
				}

				// Copied out of the scratch target, so it can't still be bound to be drawn into
				Rendering::Renderer::ClearRenderTargets();
				for (U32 slot = 0; slot < batchCount; ++slot)
				{
					const U32 probe = state.NextCapture + slot;
					const U32 atlasX = (probe % k_atlasProbesPerRow) * k_probeWidth;
					const U32 atlasY = (probe / k_atlasProbesPerRow) * k_faceSize;
					Rendering::Renderer::CopyTextureRegion(state.Scratch, slot * k_probeWidth, 0, k_probeWidth, k_faceSize, state.Atlas, atlasX, atlasY);
				}

				// The batch's visibility maps, from what it just captured
				const GraphicsLayouts::IrradianceVolumeLayout gridVolume = MakeVolumeLayout(state.Grid, 0);
				Rendering::Renderer::UploadDataToBuffer<GraphicsLayouts::IrradianceVolumeLayout>(m_volumeBuffer, &gridVolume);
				const RelightParamsLayout visibilityParams = { state.NextCapture, probeCount, k_faceSize, k_atlasProbesPerRow };
				Rendering::Renderer::UploadDataToBuffer<RelightParamsLayout>(m_relightParamsBuffer, &visibilityParams);

				Rendering::Renderer::SetComputeShader(m_visibilityShader->GetPlatformObject());
				Rendering::Renderer::SetConstantBufferCS(m_volumeBuffer, k_volumeBufferSlot);
				Rendering::Renderer::SetConstantBufferCS(m_relightParamsBuffer, k_relightParamsSlot);
				Rendering::Renderer::SetTextureResourceCS(state.Atlas, k_captureAtlasSlot);
				Rendering::Renderer::SetUnorderedAccess(state.Visibility, 0);
				Rendering::Renderer::Dispatch(batchCount);
				Rendering::Renderer::UnsetUnorderedAccess(0);
				Rendering::Renderer::UnsetTextureResourceCS(k_captureAtlasSlot);

				state.NextCapture += batchCount;
				++state.CaptureFrames;
				if (state.NextCapture == probeCount)
				{
					RZE_LOG_ARGS("Irradiance probes: captured %u (%u x %u x %u, %.2f m apart) over %u frames, %.1f draws a face",
						probeCount, state.Grid.CountX, state.Grid.CountY, state.Grid.CountZ, state.Grid.Spacing, state.CaptureFrames,
						static_cast<double>(state.CaptureDraws) / (static_cast<double>(probeCount) * ProbeCapture::k_faceCount));
				}
			}
			// #TODO(Probes) Relight while capturing, once the capture order is by distance from the camera (step 8)
			else if (probeCount > 0)
			{
				const SceneLightingData& sceneLighting = m_sceneLightingInput.Get(context);
				const U32 batchCount = std::min(k_relightsPerFrame, probeCount);

				// Last frame's light, for the bounce, as the relighting writes this frame's over it
				for (int channel = 0; channel < 3; ++channel)
				{
					Rendering::Renderer::CopyTexture(state.SH[channel], state.PreviousSH[channel]);
				}

				const GraphicsLayouts::IrradianceVolumeLayout previousVolume = MakeVolumeLayout(state.Grid, state.RelitCount);
				Rendering::Renderer::UploadDataToBuffer<GraphicsLayouts::IrradianceVolumeLayout>(m_volumeBuffer, &previousVolume);
				const RelightParamsLayout params = { state.NextRelight, probeCount, k_faceSize, k_atlasProbesPerRow };
				Rendering::Renderer::UploadDataToBuffer<RelightParamsLayout>(m_relightParamsBuffer, &params);

				Rendering::Renderer::SetComputeShader(m_relightShader->GetPlatformObject());
				Rendering::Renderer::SetConstantBufferCS(sceneLighting.LightBuffer, k_lightBufferSlot);
				Rendering::Renderer::SetConstantBufferCS(sceneLighting.ShadowBuffer, k_shadowBufferSlot);
				Rendering::Renderer::SetConstantBufferCS(m_volumeBuffer, k_volumeBufferSlot);
				Rendering::Renderer::SetConstantBufferCS(m_relightParamsBuffer, k_relightParamsSlot);
				Rendering::Renderer::SetTextureResourceCS(sceneLighting.DirectionalShadowMap, k_shadowMapSlot);
				Rendering::Renderer::SetTextureResourceCS(sceneLighting.PointShadowCubes, k_pointShadowCubesSlot);
				for (U32 channel = 0; channel < 3; ++channel)
				{
					Rendering::Renderer::SetTextureResourceCS(state.PreviousSH[channel], k_shRedSlot + channel);
					Rendering::Renderer::SetUnorderedAccess(state.SH[channel], channel);
				}
				Rendering::Renderer::SetTextureResourceCS(state.Visibility, k_visibilitySlot);
				Rendering::Renderer::SetTextureResourceCS(state.Atlas, k_captureAtlasSlot);

				Rendering::Renderer::Dispatch(batchCount);

				// The shadow maps are drawn into and the probes copied over next frame
				for (U32 channel = 0; channel < 3; ++channel)
				{
					Rendering::Renderer::UnsetUnorderedAccess(channel);
					Rendering::Renderer::UnsetTextureResourceCS(k_shRedSlot + channel);
				}
				Rendering::Renderer::UnsetTextureResourceCS(k_shadowMapSlot);
				Rendering::Renderer::UnsetTextureResourceCS(k_pointShadowCubesSlot);
				Rendering::Renderer::UnsetTextureResourceCS(k_visibilitySlot);
				Rendering::Renderer::UnsetTextureResourceCS(k_captureAtlasSlot);

				state.NextRelight = (state.NextRelight + batchCount) % probeCount;
				// The first round goes through the probes in order, so the relit ones are always the first RelitCount
				state.RelitCount = std::min(state.RelitCount + batchCount, probeCount);
			}
		}

		if (state.RelitCount > 0)
		{
			volume = MakeVolumeLayout(state.Grid, state.RelitCount);
			irradiance.SHRed = state.SH[0];
			irradiance.SHGreen = state.SH[1];
			irradiance.SHBlue = state.SH[2];
			irradiance.Visibility = state.Visibility;
		}

		Rendering::Renderer::End();
	}

	// For the forward pass of this view, after the relighting's own upload
	Rendering::Renderer::UploadDataToBuffer<GraphicsLayouts::IrradianceVolumeLayout>(m_volumeBuffer, &volume);
	m_sceneIrradianceOutput.Publish(context, irradiance);
}
