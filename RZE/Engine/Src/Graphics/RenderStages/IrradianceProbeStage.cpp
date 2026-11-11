#include <Graphics/RenderStages/IrradianceProbeStage.h>

#include <Graphics/GlobalIllumination.h>
#include <Graphics/GraphicsLayouts.h>
#include <Graphics/IrradianceProbeGrid.h>
#include <Graphics/IrradianceProbeScheduler.h>
#include <Graphics/RenderEngine.h>
#include <Graphics/Shader.h>

#include <Rendering/Renderer.h>
#include <Rendering/TextureReadback.h>
#include <Rendering/Driver/GFXBuffer.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace
{
	// Texels across a capture face: 6 x 64 = 384 samples a probe, about what DDGI traces per probe
	constexpr U32 k_faceSize = 8;
	constexpr U32 k_probeWidth = k_faceSize * ProbeCapture::k_faceCount;
	// The most probes captured a frame: one batch's worth (ProbeCapture::k_maxBatchFaces), side by side in the scratch
	// target
	constexpr U32 k_maxCapturesPerFrame = ProbeCapture::k_maxBatchFaces / ProbeCapture::k_faceCount;
	// The capture atlas has a row of probes per k_faceSize texels
	constexpr U32 k_atlasProbesPerRow = 64;
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

	// Must match the registers in the probe shaders and Common/LightResources.hlsli
	constexpr U32 k_lightBufferSlot = 2;
	constexpr U32 k_shadowBufferSlot = 3;
	constexpr U32 k_volumeBufferSlot = 5;
	constexpr U32 k_probeListSlot = 6;
	constexpr U32 k_shadowMapSlot = 5;
	constexpr U32 k_pointShadowCubesSlot = 6;
	constexpr U32 k_shRedSlot = 8;
	constexpr U32 k_visibilitySlot = 11;
	constexpr U32 k_captureAtlasSlot = 13;
	// UAVs of Compute_ProbeVisibility
	constexpr U32 k_visibilityOutput = 0;
	constexpr U32 k_activeOutput = 1;

	constexpr U64 k_neverFrame = ~0ull;

	// Which of a frame's captures are active, as Compute_ProbeVisibility said (a texel each, in list order), on its way
	// back from the GPU
	struct ActivityReadback
	{
		std::shared_ptr<Rendering::TextureReadback> Readback;
		U32 Count = 0;
		U32 Probes[k_maxCapturesPerFrame] = {};
	};

	// A scene's probes, what they've captured and their light, kept in its ScenePersistent() blackboard
	struct IrradianceProbeState : RenderStageData<IrradianceProbeState>
	{
		[[maybe_unused]] static constexpr const char* k_name = "IrradianceProbeState";

		// The grid, and which probes to capture and relight
		IrradianceProbeScheduler Scheduler;
		// Every probe's capture, at its index's place (k_atlasProbesPerRow to a row of k_faceSize texels)
		Rendering::TextureBuffer2DHandle Atlas;
		// One frame's captures, side by side, and their depth
		Rendering::TextureBuffer2DHandle Scratch;
		Rendering::TextureBuffer2DHandle ScratchDepth;
		// The probes' spherical harmonics, a texture array per channel (red, green, blue), and last frame's, which the
		// relighting reads while it writes these. Made for the scheduler's grid, cleared to 0: black until relit.
		Rendering::TextureBuffer2DHandle SH[3];
		Rendering::TextureBuffer2DHandle PreviousSH[3];
		// Every probe's visibility map, at its index's place (k_visibilityProbesPerRow to a row). Cleared to 0 when the
		// grid is fitted, so a probe reads as inactive until it's captured.
		Rendering::TextureBuffer2DHandle Visibility;
		// Compute_ProbeVisibility's word on which of the last captures are active (ActivityReadback), and those read
		// back but not yet handed to the scheduler
		Rendering::TextureBuffer2DHandle Activity;
		std::vector<ActivityReadback> PendingActivity;

		// The frame the probes were last updated in (RenderContext::FrameIndex), so it's once a frame, and whether
		// any were relit then
		U64 UpdatedFrame = k_neverFrame;
		bool WereRelit = false;
		// Since the grid was fitted, for the log when every probe has been captured
		U32 CaptureFrames = 0;
		U64 CaptureDraws = 0;
		bool HasLoggedCaptures = false;
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

	// A probe a texel, a slice per layer of the grid. Always an array (at least 2 slices), as the shaders read one. A
	// render target too, only so it can be cleared.
	Rendering::TextureBuffer2DHandle CreateSHTexture(U32 width, U32 height, U32 slices)
	{
		Rendering::GFXTextureBufferParams params = { 0 };
		params.bIsShaderResource = true;
		params.bIsUnorderedAccess = true;
		params.bIsRenderTarget = true;
		params.Width = width;
		params.Height = height;
		params.ArraySize = std::max(slices, 2u);
		params.MipLevels = 1;
		params.SampleCount = 1;
		params.Format = Rendering::ETextureFormat::RGBA16_FLOAT;
		return Rendering::Renderer::CreateTextureBuffer2D(params);
	}

	// A texel for each of a frame's captures, written by Compute_ProbeVisibility. A shader resource only because every
	// texture has a view.
	Rendering::TextureBuffer2DHandle CreateActivityTexture()
	{
		Rendering::GFXTextureBufferParams params = { 0 };
		params.bIsShaderResource = true;
		params.bIsUnorderedAccess = true;
		params.Width = k_maxCapturesPerFrame;
		params.Height = 1;
		params.MipLevels = 1;
		params.SampleCount = 1;
		params.Format = Rendering::ETextureFormat::R32_UINT;
		return Rendering::Renderer::CreateTextureBuffer2D(params);
	}

	// Mean and mean squared distance, written by Compute_ProbeVisibility and sampled with filtering. A render target
	// too, only so it can be cleared.
	Rendering::TextureBuffer2DHandle CreateVisibilityTexture(U32 width, U32 height)
	{
		Rendering::GFXTextureBufferParams params = { 0 };
		params.bIsShaderResource = true;
		params.bIsUnorderedAccess = true;
		params.bIsRenderTarget = true;
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
		state.Scratch = CreateCaptureTexture(k_maxCapturesPerFrame * k_probeWidth, k_faceSize, true);
		state.ScratchDepth = CreateDepthTexture(k_maxCapturesPerFrame * k_probeWidth, k_faceSize);
		state.Visibility = CreateVisibilityTexture(k_visibilityAtlasWidth, k_visibilityAtlasHeight);
		state.Activity = CreateActivityTexture();
		return state;
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

	// Every probe's textures for a newly fitted grid, cleared so nothing reads as lit or captured, and the capture log
	// started over
	void ResetForGrid(IrradianceProbeState& state)
	{
		const IrradianceProbeGrid& grid = state.Scheduler.GetGrid();
		state.CaptureFrames = 0;
		state.CaptureDraws = 0;
		state.HasLoggedCaptures = false;
		// About the old grid's probes
		state.PendingActivity.clear();

		const Vector4D zero(0.0f, 0.0f, 0.0f, 0.0f);
		for (int channel = 0; channel < 3; ++channel)
		{
			if (grid.IsEmpty())
			{
				state.SH[channel] = Rendering::TextureBuffer2DHandle();
				state.PreviousSH[channel] = Rendering::TextureBuffer2DHandle();
				continue;
			}
			state.SH[channel] = CreateSHTexture(grid.CountX, grid.CountY, grid.CountZ);
			state.PreviousSH[channel] = CreateSHTexture(grid.CountX, grid.CountY, grid.CountZ);
			Rendering::Renderer::ClearRenderTarget(state.SH[channel], zero);
			Rendering::Renderer::ClearRenderTarget(state.PreviousSH[channel], zero);
		}
		Rendering::Renderer::ClearRenderTarget(state.Visibility, zero);
	}

	// Hands the scheduler which of the earlier captures are active, those that have come back from the GPU
	void TakeActivity(IrradianceProbeState& state)
	{
		std::vector<ActivityReadback>& pending = state.PendingActivity;
		while (!pending.empty() && pending.front().Readback->IsReady.load(std::memory_order_acquire))
		{
			const ActivityReadback& activity = pending.front();
			const U32* texels = reinterpret_cast<const U32*>(activity.Readback->Data.data());
			for (U32 slot = 0; slot < activity.Count; ++slot)
			{
				state.Scheduler.SetActive(activity.Probes[slot], texels[slot] != 0);
			}
			pending.erase(pending.begin());
		}
	}
}

IrradianceProbeStage::IrradianceProbeStage(const GlobalIlluminationSettings* settings)
	: m_settings(settings)
	, m_capture(k_faceSize, ProbeCapture::EProbeFaces::Probe)
{
	AssertNotNull(m_settings);
}

void IrradianceProbeStage::Initialize()
{
	m_capture.Initialize();
	m_relightShader = LoadComputeShader("Compute_ProbeRelight");
	m_visibilityShader = LoadComputeShader("Compute_ProbeVisibility");
	m_probeListBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(GraphicsLayouts::ProbeListLayout), 16, 1);
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
	const GlobalIlluminationSettings& settings = *m_settings;

	SceneIrradianceData irradiance;
	irradiance.VolumeBuffer = m_volumeBuffer;
	irradiance.SHRed = m_placeholderSH;
	irradiance.SHGreen = m_placeholderSH;
	irradiance.SHBlue = m_placeholderSH;
	irradiance.Visibility = m_placeholderVisibility;
	GraphicsLayouts::IrradianceVolumeLayout volume;

	if (view.GlobalIllumination && view.Lighting == ERenderViewLighting::Scene)
	{
		IrradianceProbeState& state = context.ScenePersistent().GetOrCreate<IrradianceProbeState>(&CreateState);

		if (state.UpdatedFrame != context.FrameIndex)
		{
			state.UpdatedFrame = context.FrameIndex;
			Rendering::Renderer::Begin("IrradianceProbeStage");

			IrradianceProbeScheduler& scheduler = state.Scheduler;
			const Vector3D& cameraPosition = view.Camera.Position;
			if (scheduler.FitGrid(scene, settings.TargetProbeCount, cameraPosition))
			{
				ResetForGrid(state);
			}
			TakeActivity(state);
			scheduler.QueueChangedProbes(scene, cameraPosition, context.FrameIndex, settings.RecaptureSettleFrames);
			scheduler.QueueChangedLights(scene, cameraPosition, settings.SettleRelights);

			const IrradianceProbeGrid& grid = scheduler.GetGrid();
			const U32 probeCount = grid.GetProbeCount();

			GraphicsLayouts::ProbeListLayout captured;
			captured.FaceSize = k_faceSize;
			captured.AtlasProbesPerRow = k_atlasProbesPerRow;
			if (scheduler.HasCaptureWaiting())
			{
				// Far enough to see across the whole grid from any probe in it
				const float gridDiagonal = std::sqrt(static_cast<float>(grid.CountX * grid.CountX + grid.CountY * grid.CountY + grid.CountZ * grid.CountZ));
				const float farDistance = (gridDiagonal + 1.0f) * grid.Spacing;

				Rendering::Renderer::SetColourTarget(state.Scratch, state.ScratchDepth);
				Rendering::Renderer::ClearRenderTarget(state.Scratch, Vector4D(0.0f, 0.0f, 0.0f, 0.0f));
				Rendering::Renderer::ClearDepthStencilBuffer(state.ScratchDepth);

				// Nearest first, all drawn together
				const U32 capturesPerFrame = std::min(settings.CapturesPerFrame, k_maxCapturesPerFrame);
				m_capture.BeginBatch(k_maxCapturesPerFrame * k_probeWidth, k_faceSize);
				while (scheduler.HasCaptureWaiting() && captured.Count < capturesPerFrame)
				{
					const U32 probe = scheduler.TakeCapture();
					m_capture.AddProbe(grid.GetProbePosition(probe), farDistance, captured.Count * k_probeWidth, 0);
					captured.Probes[captured.Count++] = probe;
				}
				const U32 draws = m_capture.Flush(scene);

				// Copied out of the scratch target, so it can't still be bound to be drawn into
				Rendering::Renderer::ClearRenderTargets();
				for (U32 slot = 0; slot < captured.Count; ++slot)
				{
					const U32 probe = captured.Probes[slot];
					const U32 atlasX = (probe % k_atlasProbesPerRow) * k_probeWidth;
					const U32 atlasY = (probe / k_atlasProbesPerRow) * k_faceSize;
					Rendering::Renderer::CopyTextureRegion(state.Scratch, slot * k_probeWidth, 0, k_probeWidth, k_faceSize, state.Atlas, atlasX, atlasY);
					scheduler.MarkCaptured(probe);
				}

				// Their visibility maps, from what they just captured, and whether they're active, read back for the
				// scheduler
				const GraphicsLayouts::IrradianceVolumeLayout gridVolume = MakeVolumeLayout(grid, probeCount);
				Rendering::Renderer::UploadDataToBuffer<GraphicsLayouts::IrradianceVolumeLayout>(m_volumeBuffer, &gridVolume);
				Rendering::Renderer::UploadDataToBuffer<GraphicsLayouts::ProbeListLayout>(m_probeListBuffer, &captured);

				Rendering::Renderer::SetComputeShader(m_visibilityShader->GetPlatformObject());
				Rendering::Renderer::SetConstantBufferCS(m_volumeBuffer, k_volumeBufferSlot);
				Rendering::Renderer::SetConstantBufferCS(m_probeListBuffer, k_probeListSlot);
				Rendering::Renderer::SetTextureResourceCS(state.Atlas, k_captureAtlasSlot);
				Rendering::Renderer::SetUnorderedAccess(state.Visibility, k_visibilityOutput);
				Rendering::Renderer::SetUnorderedAccess(state.Activity, k_activeOutput);
				Rendering::Renderer::Dispatch(captured.Count);
				Rendering::Renderer::UnsetUnorderedAccess(k_visibilityOutput);
				Rendering::Renderer::UnsetUnorderedAccess(k_activeOutput);
				Rendering::Renderer::UnsetTextureResourceCS(k_captureAtlasSlot);

				ActivityReadback activity;
				activity.Readback = std::make_shared<Rendering::TextureReadback>();
				activity.Count = captured.Count;
				std::copy(captured.Probes, captured.Probes + captured.Count, activity.Probes);
				Rendering::Renderer::ReadTexture(state.Activity, activity.Readback);
				state.PendingActivity.push_back(std::move(activity));

				context.WorkCounts.IrradianceProbesCaptured += captured.Count;
				context.WorkCounts.IrradianceCaptureDraws += draws;
				state.CaptureDraws += draws;
				++state.CaptureFrames;
				if (!scheduler.IsFilling() && !state.HasLoggedCaptures)
				{
					state.HasLoggedCaptures = true;
					RZE_LOG_ARGS("Irradiance probes: captured %u (%u x %u x %u, %.2f m apart) over %u frames, %.1f draws a frame",
						probeCount, grid.CountX, grid.CountY, grid.CountZ, grid.Spacing, state.CaptureFrames,
						static_cast<double>(state.CaptureDraws) / static_cast<double>(state.CaptureFrames));
				}
			}

			// Relit while their light is still changing: the probes just captured, then those around changed lights,
			// those near the camera, then the rest in turn, up to the budget
			GraphicsLayouts::ProbeListLayout relit = captured;
			scheduler.ListRelights(settings.RelightBudget, settings.SettleRelights, cameraPosition, context.FrameIndex, relit);
			state.WereRelit = relit.Count > 0;

			if (relit.Count > 0)
			{
				const SceneLightingData& sceneLighting = m_sceneLightingInput.Get(context);

				// Last frame's light, for the bounce, as the relighting writes this frame's over it
				for (int channel = 0; channel < 3; ++channel)
				{
					Rendering::Renderer::CopyTexture(state.SH[channel], state.PreviousSH[channel]);
				}

				const GraphicsLayouts::IrradianceVolumeLayout previousVolume = MakeVolumeLayout(grid, probeCount);
				Rendering::Renderer::UploadDataToBuffer<GraphicsLayouts::IrradianceVolumeLayout>(m_volumeBuffer, &previousVolume);
				Rendering::Renderer::UploadDataToBuffer<GraphicsLayouts::ProbeListLayout>(m_probeListBuffer, &relit);

				Rendering::Renderer::SetComputeShader(m_relightShader->GetPlatformObject());
				Rendering::Renderer::SetConstantBufferCS(sceneLighting.LightBuffer, k_lightBufferSlot);
				Rendering::Renderer::SetConstantBufferCS(sceneLighting.ShadowBuffer, k_shadowBufferSlot);
				Rendering::Renderer::SetConstantBufferCS(m_volumeBuffer, k_volumeBufferSlot);
				Rendering::Renderer::SetConstantBufferCS(m_probeListBuffer, k_probeListSlot);
				Rendering::Renderer::SetTextureResourceCS(sceneLighting.StaticDirectionalShadowMap, k_shadowMapSlot);
				Rendering::Renderer::SetTextureResourceCS(sceneLighting.StaticPointShadowCubes, k_pointShadowCubesSlot);
				for (U32 channel = 0; channel < 3; ++channel)
				{
					Rendering::Renderer::SetTextureResourceCS(state.PreviousSH[channel], k_shRedSlot + channel);
					Rendering::Renderer::SetUnorderedAccess(state.SH[channel], channel);
				}
				Rendering::Renderer::SetTextureResourceCS(state.Visibility, k_visibilitySlot);
				Rendering::Renderer::SetTextureResourceCS(state.Atlas, k_captureAtlasSlot);

				Rendering::Renderer::Dispatch(relit.Count);
				context.WorkCounts.IrradianceProbesRelit += relit.Count;

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
			}

			Rendering::Renderer::End();
		}

		const IrradianceProbeGrid& grid = state.Scheduler.GetGrid();
		if (!grid.IsEmpty())
		{
			// Every probe can be read: those not captured yet are inactive (their visibility maps are 0)
			volume = MakeVolumeLayout(grid, grid.GetProbeCount());
			irradiance.SHRed = state.SH[0];
			irradiance.SHGreen = state.SH[1];
			irradiance.SHBlue = state.SH[2];
			irradiance.Visibility = state.Visibility;
			irradiance.IsChanging = state.WereRelit;
		}
	}

	// For the forward pass of this view, after the probe passes' own uploads
	Rendering::Renderer::UploadDataToBuffer<GraphicsLayouts::IrradianceVolumeLayout>(m_volumeBuffer, &volume);
	m_sceneIrradianceOutput.Publish(context, irradiance);
}
