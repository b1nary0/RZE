#include <StdAfx.h>
#include <Graphics/RenderStages/IrradianceProbeStage.h>

#include <Graphics/GlobalIllumination.h>
#include <Graphics/GraphicsLayouts.h>
#include <Graphics/IrradianceProbeGrid.h>
#include <Graphics/RenderEngine.h>
#include <Graphics/Shader.h>

#include <Rendering/Renderer.h>
#include <Rendering/Driver/GFXBuffer.h>

#include <Utils/Math/BoundingBox.h>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

namespace
{
	// Texels across a capture face: 6 x 64 = 384 samples a probe, about what DDGI traces per probe
	constexpr U32 k_faceSize = 8;
	constexpr U32 k_probeWidth = k_faceSize * ProbeCapture::k_faceCount;
	// The most probes captured a frame, however few draws they take: what fits side by side in the scratch target
	constexpr U32 k_maxCapturesPerFrame = 64;
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
	// Probes within this many spacings of the camera along each axis are relit every frame
	constexpr int k_nearProbes = 2;
	// Probes within this many spacings of an object that changed are captured again
	constexpr float k_recaptureRadiusSpacings = 5.0f;

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

	constexpr U64 k_neverFrame = ~0ull;

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
		// relighting reads while it writes these. Made for Grid, cleared to 0: black until relit.
		Rendering::TextureBuffer2DHandle SH[3];
		Rendering::TextureBuffer2DHandle PreviousSH[3];
		// Every probe's visibility map, at its index's place (k_visibilityProbesPerRow to a row). Cleared to 0 when the
		// grid is fitted, so a probe reads as inactive until it's captured.
		Rendering::TextureBuffer2DHandle Visibility;

		// Probes waiting to be captured, farthest from the camera first, so the nearest is taken off the back
		std::vector<U32> CaptureQueue;
		// By probe: whether it has a capture, whether it's in CaptureQueue, and the frame it was last relit
		std::vector<bool> Captured;
		std::vector<bool> Queued;
		std::vector<U64> RelitFrame;
		// The next probe to relight in turn
		U32 NextRelight = 0;
		// The scene's GetRevision when changed objects were last looked for
		U64 CheckedRevision = 0;
		// The frame the probes were last updated in (RenderContext::FrameIndex), so it's once a frame
		U64 UpdatedFrame = k_neverFrame;
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

	// Whether either corner moved by more than tolerance along any axis, or one is empty and the other isn't
	bool HaveBoundsMoved(const BoundingBox& from, const BoundingBox& to, float tolerance)
	{
		if (from.IsEmpty() || to.IsEmpty())
		{
			return from.IsEmpty() != to.IsEmpty();
		}

		for (int axis = 0; axis < 3; ++axis)
		{
			if (std::abs(from.GetMin()[axis] - to.GetMin()[axis]) > tolerance || std::abs(from.GetMax()[axis] - to.GetMax()[axis]) > tolerance)
			{
				return true;
			}
		}
		return false;
	}

	void SortCaptureQueue(IrradianceProbeState& state, const Vector3D& cameraPosition)
	{
		std::vector<float> distances(state.Grid.GetProbeCount(), 0.0f);
		for (U32 probe : state.CaptureQueue)
		{
			distances[probe] = (GetProbePosition(state.Grid, probe) - cameraPosition).Length();
		}
		std::sort(state.CaptureQueue.begin(), state.CaptureQueue.end(), [&distances](U32 a, U32 b) { return distances[a] > distances[b]; });
	}

	// A new grid over the scene's bounds, every probe waiting to be captured and nothing lit
	void Refit(IrradianceProbeState& state, const RenderScene& scene, U32 targetProbeCount, const Vector3D& cameraPosition)
	{
		state.FittedBounds = scene.GetBounds();
		state.Grid = IrradianceProbeGrid::Fit(state.FittedBounds, targetProbeCount);

		const U32 probeCount = state.Grid.GetProbeCount();
		state.Captured.assign(probeCount, false);
		state.Queued.assign(probeCount, true);
		state.RelitFrame.assign(probeCount, k_neverFrame);
		state.CaptureQueue.resize(probeCount);
		std::iota(state.CaptureQueue.begin(), state.CaptureQueue.end(), 0u);
		SortCaptureQueue(state, cameraPosition);

		state.NextRelight = 0;
		state.CheckedRevision = scene.GetRevision();
		state.CaptureFrames = 0;
		state.CaptureDraws = 0;
		state.HasLoggedCaptures = false;

		const Vector4D zero(0.0f, 0.0f, 0.0f, 0.0f);
		for (int channel = 0; channel < 3; ++channel)
		{
			if (probeCount == 0)
			{
				state.SH[channel] = Rendering::TextureBuffer2DHandle();
				state.PreviousSH[channel] = Rendering::TextureBuffer2DHandle();
				continue;
			}
			state.SH[channel] = CreateSHTexture(state.Grid.CountX, state.Grid.CountY, state.Grid.CountZ);
			state.PreviousSH[channel] = CreateSHTexture(state.Grid.CountX, state.Grid.CountY, state.Grid.CountZ);
			Rendering::Renderer::ClearRenderTarget(state.SH[channel], zero);
			Rendering::Renderer::ClearRenderTarget(state.PreviousSH[channel], zero);
		}
		Rendering::Renderer::ClearRenderTarget(state.Visibility, zero);
	}

	// Queues the captured probes within a few spacings of objects that changed since last time, nearest first. They
	// keep their old captures until they're captured again.
	void QueueChangedProbes(IrradianceProbeState& state, const RenderScene& scene, const Vector3D& cameraPosition)
	{
		if (scene.GetObjectsRevision() <= state.CheckedRevision)
		{
			return;
		}

		const Vector3D reach(state.Grid.Spacing * k_recaptureRadiusSpacings);
		bool hasQueued = false;
		for (U32 probe = 0; probe < state.Grid.GetProbeCount(); ++probe)
		{
			if (!state.Captured[probe] || state.Queued[probe])
			{
				continue;
			}

			const Vector3D position = GetProbePosition(state.Grid, probe);
			if (scene.HaveObjectsChangedWithin(BoundingBox(position - reach, position + reach), state.CheckedRevision))
			{
				state.CaptureQueue.push_back(probe);
				state.Queued[probe] = true;
				hasQueued = true;
			}
		}

		state.CheckedRevision = scene.GetRevision();
		if (hasQueued)
		{
			SortCaptureQueue(state, cameraPosition);
		}
	}

	void AddToRelight(IrradianceProbeState& state, GraphicsLayouts::ProbeListLayout& list, U32 probe, U64 frameIndex)
	{
		list.Probes[list.Count++] = probe;
		state.RelitFrame[probe] = frameIndex;
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

			const Vector3D& cameraPosition = view.Camera.Position;
			if (HaveBoundsMoved(state.FittedBounds, scene.GetBounds(), state.Grid.Spacing))
			{
				Refit(state, scene, settings.TargetProbeCount, cameraPosition);
			}
			QueueChangedProbes(state, scene, cameraPosition);

			const U32 probeCount = state.Grid.GetProbeCount();

			GraphicsLayouts::ProbeListLayout captured;
			captured.FaceSize = k_faceSize;
			captured.AtlasProbesPerRow = k_atlasProbesPerRow;
			if (!state.CaptureQueue.empty())
			{
				// Far enough to see across the whole grid from any probe in it
				const float gridDiagonal = std::sqrt(static_cast<float>(state.Grid.CountX * state.Grid.CountX + state.Grid.CountY * state.Grid.CountY + state.Grid.CountZ * state.Grid.CountZ));
				const float farDistance = (gridDiagonal + 1.0f) * state.Grid.Spacing;

				Rendering::Renderer::SetColourTarget(state.Scratch, state.ScratchDepth);
				Rendering::Renderer::ClearRenderTarget(state.Scratch, Vector4D(0.0f, 0.0f, 0.0f, 0.0f));
				Rendering::Renderer::ClearDepthStencilBuffer(state.ScratchDepth);

				// Nearest first, until the draws or the scratch target run out
				U32 draws = 0;
				while (!state.CaptureQueue.empty() && captured.Count < k_maxCapturesPerFrame && draws < settings.CaptureDrawBudget)
				{
					const U32 probe = state.CaptureQueue.back();
					state.CaptureQueue.pop_back();
					state.Queued[probe] = false;

					draws += m_capture.Capture(scene, GetProbePosition(state.Grid, probe), farDistance, captured.Count * k_probeWidth, 0);
					captured.Probes[captured.Count++] = probe;
				}

				// Copied out of the scratch target, so it can't still be bound to be drawn into
				Rendering::Renderer::ClearRenderTargets();
				for (U32 slot = 0; slot < captured.Count; ++slot)
				{
					const U32 probe = captured.Probes[slot];
					const U32 atlasX = (probe % k_atlasProbesPerRow) * k_probeWidth;
					const U32 atlasY = (probe / k_atlasProbesPerRow) * k_faceSize;
					Rendering::Renderer::CopyTextureRegion(state.Scratch, slot * k_probeWidth, 0, k_probeWidth, k_faceSize, state.Atlas, atlasX, atlasY);
					state.Captured[probe] = true;
				}

				// Their visibility maps, from what they just captured
				const GraphicsLayouts::IrradianceVolumeLayout gridVolume = MakeVolumeLayout(state.Grid, probeCount);
				Rendering::Renderer::UploadDataToBuffer<GraphicsLayouts::IrradianceVolumeLayout>(m_volumeBuffer, &gridVolume);
				Rendering::Renderer::UploadDataToBuffer<GraphicsLayouts::ProbeListLayout>(m_probeListBuffer, &captured);

				Rendering::Renderer::SetComputeShader(m_visibilityShader->GetPlatformObject());
				Rendering::Renderer::SetConstantBufferCS(m_volumeBuffer, k_volumeBufferSlot);
				Rendering::Renderer::SetConstantBufferCS(m_probeListBuffer, k_probeListSlot);
				Rendering::Renderer::SetTextureResourceCS(state.Atlas, k_captureAtlasSlot);
				Rendering::Renderer::SetUnorderedAccess(state.Visibility, 0);
				Rendering::Renderer::Dispatch(captured.Count);
				Rendering::Renderer::UnsetUnorderedAccess(0);
				Rendering::Renderer::UnsetTextureResourceCS(k_captureAtlasSlot);

				context.WorkCounts.IrradianceProbesCaptured += captured.Count;
				context.WorkCounts.IrradianceCaptureDraws += draws;
				state.CaptureDraws += draws;
				++state.CaptureFrames;
				if (state.CaptureQueue.empty() && !state.HasLoggedCaptures)
				{
					state.HasLoggedCaptures = true;
					RZE_LOG_ARGS("Irradiance probes: captured %u (%u x %u x %u, %.2f m apart) over %u frames, %.1f draws a face",
						probeCount, state.Grid.CountX, state.Grid.CountY, state.Grid.CountZ, state.Grid.Spacing, state.CaptureFrames,
						static_cast<double>(state.CaptureDraws) / (static_cast<double>(probeCount) * ProbeCapture::k_faceCount));
				}
			}

			// Relit: the probes just captured, then those near the camera, then the rest in turn, up to the budget
			GraphicsLayouts::ProbeListLayout relit = captured;
			const U64 frameIndex = context.FrameIndex;
			for (U32 slot = 0; slot < relit.Count; ++slot)
			{
				state.RelitFrame[relit.Probes[slot]] = frameIndex;
			}
			const U32 budget = std::max(std::min(settings.RelightBudget, GraphicsLayouts::ProbeListLayout::k_maxProbes), relit.Count);

			if (probeCount > 0)
			{
				const Vector3D cameraCell = (cameraPosition - state.Grid.Origin) / state.Grid.Spacing;
				const int counts[3] = { static_cast<int>(state.Grid.CountX), static_cast<int>(state.Grid.CountY), static_cast<int>(state.Grid.CountZ) };
				int nearest[3];
				for (int axis = 0; axis < 3; ++axis)
				{
					nearest[axis] = std::clamp(static_cast<int>(std::lround(cameraCell[axis])), 0, counts[axis] - 1);
				}
				for (int z = std::max(nearest[2] - k_nearProbes, 0); z <= std::min(nearest[2] + k_nearProbes, counts[2] - 1); ++z)
				{
					for (int y = std::max(nearest[1] - k_nearProbes, 0); y <= std::min(nearest[1] + k_nearProbes, counts[1] - 1); ++y)
					{
						for (int x = std::max(nearest[0] - k_nearProbes, 0); x <= std::min(nearest[0] + k_nearProbes, counts[0] - 1); ++x)
						{
							const U32 probe = static_cast<U32>(x + counts[0] * (y + counts[1] * z));
							if (relit.Count < budget && state.Captured[probe] && state.RelitFrame[probe] != frameIndex)
							{
								AddToRelight(state, relit, probe, frameIndex);
							}
						}
					}
				}

				for (U32 tried = 0; tried < probeCount && relit.Count < budget; ++tried)
				{
					const U32 probe = state.NextRelight;
					state.NextRelight = (state.NextRelight + 1) % probeCount;
					if (state.Captured[probe] && state.RelitFrame[probe] != frameIndex)
					{
						AddToRelight(state, relit, probe, frameIndex);
					}
				}
			}

			if (relit.Count > 0)
			{
				const SceneLightingData& sceneLighting = m_sceneLightingInput.Get(context);

				// Last frame's light, for the bounce, as the relighting writes this frame's over it
				for (int channel = 0; channel < 3; ++channel)
				{
					Rendering::Renderer::CopyTexture(state.SH[channel], state.PreviousSH[channel]);
				}

				const GraphicsLayouts::IrradianceVolumeLayout previousVolume = MakeVolumeLayout(state.Grid, probeCount);
				Rendering::Renderer::UploadDataToBuffer<GraphicsLayouts::IrradianceVolumeLayout>(m_volumeBuffer, &previousVolume);
				Rendering::Renderer::UploadDataToBuffer<GraphicsLayouts::ProbeListLayout>(m_probeListBuffer, &relit);

				Rendering::Renderer::SetComputeShader(m_relightShader->GetPlatformObject());
				Rendering::Renderer::SetConstantBufferCS(sceneLighting.LightBuffer, k_lightBufferSlot);
				Rendering::Renderer::SetConstantBufferCS(sceneLighting.ShadowBuffer, k_shadowBufferSlot);
				Rendering::Renderer::SetConstantBufferCS(m_volumeBuffer, k_volumeBufferSlot);
				Rendering::Renderer::SetConstantBufferCS(m_probeListBuffer, k_probeListSlot);
				Rendering::Renderer::SetTextureResourceCS(sceneLighting.DirectionalShadowMap, k_shadowMapSlot);
				Rendering::Renderer::SetTextureResourceCS(sceneLighting.PointShadowCubes, k_pointShadowCubesSlot);
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

		if (!state.Grid.IsEmpty())
		{
			// Every probe can be read: those not captured yet are inactive (their visibility maps are 0)
			volume = MakeVolumeLayout(state.Grid, state.Grid.GetProbeCount());
			irradiance.SHRed = state.SH[0];
			irradiance.SHGreen = state.SH[1];
			irradiance.SHBlue = state.SH[2];
			irradiance.Visibility = state.Visibility;
		}
	}

	// For the forward pass of this view, after the probe passes' own uploads
	Rendering::Renderer::UploadDataToBuffer<GraphicsLayouts::IrradianceVolumeLayout>(m_volumeBuffer, &volume);
	m_sceneIrradianceOutput.Publish(context, irradiance);
}
