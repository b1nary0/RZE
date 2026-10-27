#include <StdAfx.h>
#include <Graphics/RenderStages/IrradianceProbeStage.h>

#include <Graphics/IrradianceProbeGrid.h>
#include <Graphics/RenderEngine.h>

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
	// The atlas has a row of probes per k_faceSize texels, as many to a row as a frame captures
	constexpr U32 k_atlasProbesPerRow = k_capturesPerFrame;
	constexpr U32 k_atlasWidth = k_atlasProbesPerRow * k_probeWidth;
	constexpr U32 k_atlasHeight = (IrradianceProbeGrid::k_maxProbes + k_atlasProbesPerRow - 1) / k_atlasProbesPerRow * k_faceSize;

	// A scene's probes and what they've captured, kept in its ScenePersistent() blackboard
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
		// The first probe not yet captured
		U32 NextCapture = 0;
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

	IrradianceProbeState CreateState()
	{
		IrradianceProbeState state;
		state.Atlas = CreateCaptureTexture(k_atlasWidth, k_atlasHeight, false);
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
}

IrradianceProbeStage::IrradianceProbeStage()
	: m_capture(k_faceSize)
{
}

void IrradianceProbeStage::Initialize()
{
	m_capture.Initialize();
}

void IrradianceProbeStage::Setup(RenderStageBuilder&)
{
}

void IrradianceProbeStage::Render(RenderContext& context)
{
	OPTICK_EVENT();

	const RenderView& view = context.View;
	const RenderScene& scene = context.Scene;

	// #TODO(Probes) Once per scene per frame, whichever view renders it (step 8). For now the main view's scene only.
	if (!view.IsMainView() || view.Lighting != ERenderViewLighting::Scene)
	{
		return;
	}

	IrradianceProbeState& state = context.ScenePersistent().GetOrCreate<IrradianceProbeState>(&CreateState);

	// #TODO(Probes) Refit only when the bounds move by more than a spacing, and recapture near changed objects (step 8)
	const BoundingBox& bounds = scene.GetBounds();
	if (bounds != state.FittedBounds)
	{
		state.FittedBounds = bounds;
		state.Grid = IrradianceProbeGrid::Fit(bounds, k_targetProbeCount);
		state.NextCapture = 0;
		state.CaptureFrames = 0;
		state.CaptureDraws = 0;
	}

	const U32 probeCount = state.Grid.GetProbeCount();
	if (state.NextCapture >= probeCount)
	{
		return;
	}

	Rendering::Renderer::Begin("IrradianceProbeStage");

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

	state.NextCapture += batchCount;
	++state.CaptureFrames;
	if (state.NextCapture == probeCount)
	{
		RZE_LOG_ARGS("Irradiance probes: captured %u (%u x %u x %u, %.2f m apart) over %u frames, %.1f draws a face",
			probeCount, state.Grid.CountX, state.Grid.CountY, state.Grid.CountZ, state.Grid.Spacing, state.CaptureFrames,
			static_cast<double>(state.CaptureDraws) / (static_cast<double>(probeCount) * ProbeCapture::k_faceCount));
	}

	Rendering::Renderer::End();
}
