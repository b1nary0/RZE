#include <StdAfx.h>
#include <Graphics/RenderStages/ReflectionProbeStage.h>

#include <Graphics/GlobalIllumination.h>
#include <Graphics/GraphicsLayouts.h>
#include <Graphics/RenderEngine.h>
#include <Graphics/SettlingRegions.h>
#include <Graphics/Shader.h>

#include <Rendering/Renderer.h>
#include <Rendering/Driver/GFXBuffer.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
	// Texels across a capture face, and the cubes' mip 0
	constexpr U32 k_faceSize = 128;
	constexpr U32 k_probeWidth = k_faceSize * ProbeCapture::k_faceCount;
	constexpr U32 k_maxProbes = GraphicsLayouts::k_maxReflectionProbes;
	constexpr U32 k_cubeSlices = k_maxProbes * ProbeCapture::k_faceCount;
	// Mirrors REFLECTION_MIP_COUNT (Common/Probes.hlsli)
	constexpr U32 k_prefilteredMipCount = 6;
	// Probes relit a frame, the one just captured among them
	constexpr U32 k_relightsPerFrame = 2;
	// While the irradiance probes are still settling, a probe is relit in turn every this many frames for the light
	// bouncing into it, which changes slowly
	constexpr U64 k_settlingRelightFrames = 4;
	// A Blinn-Phong exponent treated as a mirror; mirrors MaxShininessLog2 (Common/Lighting.hlsli), which picks the mip
	constexpr float k_maxShininessLog2 = 11.0f;

	// Must match the registers in the reflection shaders, Common/LightResources.hlsli and Common/IrradianceVolume.hlsli
	constexpr U32 k_paramsSlot = 0;
	constexpr U32 k_lightBufferSlot = 2;
	constexpr U32 k_shadowBufferSlot = 3;
	constexpr U32 k_volumeBufferSlot = 5;
	constexpr U32 k_probeListSlot = 6;
	constexpr U32 k_radianceSlot = 0;
	constexpr U32 k_shadowMapSlot = 5;
	constexpr U32 k_pointShadowCubesSlot = 6;
	constexpr U32 k_shRedSlot = 8;
	constexpr U32 k_visibilitySlot = 11;
	constexpr U32 k_captureAtlasSlot = 13;

	constexpr U64 k_neverFrame = ~0ull;
	constexpr U32 k_groupSize = 8;

	// A scene's reflection probe slots and their cubes, kept in its ScenePersistent() blackboard. Made the first time
	// the scene has a probe.
	struct ReflectionProbeState : RenderStageData<ReflectionProbeState>
	{
		static constexpr const char* k_name = "ReflectionProbeState";

		struct Slot
		{
			// The probe in the slot (ReflectionProbeObject::GetLiveID), 0 for none
			U64 LiveID = 0;
			// The probe's GetRevision when it was last seen moved or resized
			U64 ProbeRevision = 0;
			bool IsCaptured = false;
			// The faces a change has settled in front of since it was captured (a ProbeCapture face mask), captured
			// again
			U32 StaleFaces = 0;
			// Whether its prefiltered cube has been made, so the forward pass can reflect it, and whether its light has
			// changed since: it was captured, or a light changed in its box, or the irradiance probes finished settling
			bool IsLit = false;
			bool NeedsRelight = false;
		};

		// A probe a row of k_faceSize texels, its faces side by side
		Rendering::TextureBuffer2DHandle Atlas;
		// One capture, and its depth
		Rendering::TextureBuffer2DHandle Scratch;
		Rendering::TextureBuffer2DHandle ScratchDepth;
		// The lit captures, a cube per slot with every mip (GenerateMips), and what the forward pass reads, a mip per gloss
		Rendering::TextureBuffer2DHandle Radiance;
		Rendering::TextureBuffer2DHandle Prefiltered;

		std::array<Slot, k_maxProbes> Slots;
		// The next slot to relight in turn while the irradiance probes are settling, and whether they were last frame
		U32 NextRelight = 0;
		bool WasIrradianceChanging = false;
		// Where Static objects and the probes changed, until they've stayed still for long enough to capture again
		// (GlobalIlluminationSettings::RecaptureSettleFrames)
		SettlingRegions Unsettled;
		// The scene's GetRevision when changed objects were last looked for; from when the state was made, as new
		// probes are captured anyway
		U64 CheckedRevision = 0;
		bool HasCheckedChanges = false;
		// The frame the probes were last updated in (RenderContext::FrameIndex), so it's once a frame
		U64 UpdatedFrame = k_neverFrame;
	};

	Rendering::TextureBuffer2DHandle CreateTexture(Rendering::ETextureFormat format, U32 width, U32 height, bool isRenderTarget)
	{
		Rendering::GFXTextureBufferParams params = { 0 };
		params.bIsShaderResource = true;
		params.bIsRenderTarget = isRenderTarget;
		params.Width = width;
		params.Height = height;
		params.MipLevels = 1;
		params.SampleCount = 1;
		params.Format = format;
		return Rendering::Renderer::CreateTextureBuffer2D(params);
	}

	// A cube per slot, written by compute shaders. mipLevels 0 for the whole chain; a render target only so its mips can
	// be generated.
	Rendering::TextureBuffer2DHandle CreateCubeArray(U32 size, U32 cubeCount, U32 mipLevels, bool isRenderTarget)
	{
		Rendering::GFXTextureBufferParams params = { 0 };
		params.bIsShaderResource = true;
		params.bIsUnorderedAccess = true;
		params.bIsRenderTarget = isRenderTarget;
		params.bIsCubeMap = true;
		params.Width = size;
		params.Height = size;
		params.ArraySize = cubeCount * ProbeCapture::k_faceCount;
		params.MipLevels = mipLevels;
		params.SampleCount = 1;
		params.Format = Rendering::ETextureFormat::RGBA16_FLOAT;
		return Rendering::Renderer::CreateTextureBuffer2D(params);
	}

	ReflectionProbeState CreateState()
	{
		ReflectionProbeState state;
		state.Atlas = CreateTexture(Rendering::ETextureFormat::RG32_UINT, k_probeWidth, k_faceSize * k_maxProbes, false);
		state.Scratch = CreateTexture(Rendering::ETextureFormat::RG32_UINT, k_probeWidth, k_faceSize, true);

		Rendering::GFXTextureBufferParams depthParams = { 0 };
		depthParams.bIsDepthTexture = true;
		depthParams.Width = k_probeWidth;
		depthParams.Height = k_faceSize;
		depthParams.MipLevels = 1;
		depthParams.SampleCount = 1;
		state.ScratchDepth = Rendering::Renderer::CreateTextureBuffer2D(depthParams);

		state.Radiance = CreateCubeArray(k_faceSize, k_maxProbes, 0, true);
		state.Prefiltered = CreateCubeArray(k_faceSize, k_maxProbes, k_prefilteredMipCount, false);
		return state;
	}

	U32 DivideRoundingUp(U32 value, U32 divisor)
	{
		return (value + divisor - 1) / divisor;
	}

	// Has the captured probes whose boxes a light changed in since checkedRevision relit, or all of them if the change
	// was everywhere (a directional light) or the scene doesn't know where
	void MarkRelitByLights(const RenderScene& scene, U64 checkedRevision, ReflectionProbeState& state)
	{
		const ReflectionProbeContainer& probes = scene.GetReflectionProbes();
		const U32 probeCount = std::min(static_cast<U32>(probes.size()), k_maxProbes);

		const bool isKnown = scene.GetLightChanges().ForEachChangeSince(checkedRevision, [&state, &probes, probeCount](const BoundingBox& changed)
		{
			for (U32 slot = 0; slot < probeCount; ++slot)
			{
				if (probes[slot]->GetBox().Intersects(changed))
				{
					state.Slots[slot].NeedsRelight = true;
				}
			}
		});
		if (!isKnown)
		{
			for (ReflectionProbeState::Slot& slotState : state.Slots)
			{
				slotState.NeedsRelight = true;
			}
		}
	}

	// Marks stale the faces of the captured probes that a change has settled in front of since last time: Static objects
	// that changed in their boxes, or the probes themselves moved or resized, once nothing there has changed for
	// settleFrames frames. capture says which faces see where. Has those whose boxes lights changed in relit
	// (MarkRelitByLights).
	void MarkStaleProbes(const RenderScene& scene, U64 frameIndex, U32 settleFrames, const ProbeCapture& capture, ReflectionProbeState& state)
	{
		const ReflectionProbeContainer& probes = scene.GetReflectionProbes();
		const U32 probeCount = std::min(static_cast<U32>(probes.size()), k_maxProbes);

		if (state.HasCheckedChanges)
		{
			MarkRelitByLights(scene, state.CheckedRevision, state);
		}

		// Static objects changing: Movable ones aren't captured
		if (state.HasCheckedChanges && scene.GetStaticObjectsRevision() > state.CheckedRevision)
		{
			const auto touch = [&state, frameIndex](const BoundingBox& changedBounds) { state.Unsettled.Touch(changedBounds, frameIndex); };
			// Where isn't known: every probe, at once
			if (!scene.GetStaticChanges().ForEachChangeSince(state.CheckedRevision, touch))
			{
				state.Unsettled.Clear();
				for (ReflectionProbeState::Slot& slotState : state.Slots)
				{
					slotState.StaleFaces = ProbeCapture::k_allFaces;
				}
			}
		}
		state.CheckedRevision = scene.GetRevision();
		state.HasCheckedChanges = true;

		// Probes moved or resized: where they are now
		for (U32 slot = 0; slot < probeCount; ++slot)
		{
			ReflectionProbeState::Slot& slotState = state.Slots[slot];
			const ReflectionProbeObject& probe = *probes[slot];
			if (slotState.IsCaptured && probe.GetRevision() != slotState.ProbeRevision)
			{
				slotState.ProbeRevision = probe.GetRevision();
				state.Unsettled.Touch(probe.GetBox(), frameIndex);
			}
		}

		// Once still for long enough, the faces that see them of the probes whose boxes they're in
		state.Unsettled.TakeSettled(frameIndex, settleFrames, [&state, &probes, probeCount, &capture](const BoundingBox& settled)
		{
			for (U32 slot = 0; slot < probeCount; ++slot)
			{
				const ReflectionProbeObject& probe = *probes[slot];
				if (probe.GetBox().Intersects(settled))
				{
					state.Slots[slot].StaleFaces |= capture.GetFacesSeeing(probe.GetPosition(), settled);
				}
			}
		});
	}
}

ReflectionProbeStage::ReflectionProbeStage(const GlobalIlluminationSettings* settings)
	: m_settings(settings)
	, m_capture(k_faceSize, ProbeCapture::EProbeFaces::CubeMap)
{
	AssertNotNull(m_settings);
}

void ReflectionProbeStage::Initialize()
{
	m_capture.Initialize();
	m_relightShader = LoadComputeShader("Compute_ReflectionRelight");
	m_prefilterShader = LoadComputeShader("Compute_ReflectionPrefilter");
	m_probeListBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(GraphicsLayouts::ProbeListLayout), 16, 1);
	m_relightParamsBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(RelightParamsLayout), 16, 1);
	m_prefilterParamsBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(PrefilterParamsLayout), 16, 1);
	m_probeBuffer = Rendering::Renderer::CreateConstantBuffer(nullptr, sizeof(GraphicsLayouts::ReflectionProbeBufferLayout), 16, 1);
	// Two cubes, as a cube array view needs more than one
	m_placeholderCubes = CreateCubeArray(1, 2, 1, false);
}

void ReflectionProbeStage::Setup(RenderStageBuilder& builder)
{
	m_sceneLightingInput = builder.Reads<SceneLightingData>();
	m_sceneIrradianceInput = builder.Reads<SceneIrradianceData>();
	m_sceneReflectionOutput = builder.Writes<SceneReflectionData>();
}

void ReflectionProbeStage::Render(RenderContext& context)
{
	OPTICK_EVENT();

	const RenderView& view = context.View;
	const RenderScene& scene = context.Scene;
	const ReflectionProbeContainer& probes = scene.GetReflectionProbes();

	SceneReflectionData reflection;
	reflection.ProbeBuffer = m_probeBuffer;
	reflection.Cubes = m_placeholderCubes;
	GraphicsLayouts::ReflectionProbeBufferLayout probeBuffer;

	const bool hasProbes = !probes.empty() || context.ScenePersistent().TryGet<ReflectionProbeState>() != nullptr;
	if (view.GlobalIllumination && view.Lighting == ERenderViewLighting::Scene && hasProbes)
	{
		ReflectionProbeState& state = context.ScenePersistent().GetOrCreate<ReflectionProbeState>(&CreateState);
		const U32 probeCount = std::min(static_cast<U32>(probes.size()), k_maxProbes);

		if (state.UpdatedFrame != context.FrameIndex)
		{
			state.UpdatedFrame = context.FrameIndex;

			// Slot i is probe i: a different probe there starts over
			for (U32 slot = 0; slot < k_maxProbes; ++slot)
			{
				const U64 liveID = slot < probeCount ? probes[slot]->GetLiveID() : 0;
				if (state.Slots[slot].LiveID != liveID)
				{
					state.Slots[slot] = ReflectionProbeState::Slot();
					state.Slots[slot].LiveID = liveID;
				}
			}

			MarkStaleProbes(scene, context.FrameIndex, m_settings->RecaptureSettleFrames, m_capture, state);

			GraphicsLayouts::ProbeListLayout relit;
			relit.FaceSize = k_faceSize;
			relit.AtlasProbesPerRow = 1;

			// One capture a frame: the first probe that's new (every face), or has stale faces (only those)
			for (U32 slot = 0; slot < probeCount; ++slot)
			{
				ReflectionProbeState::Slot& slotState = state.Slots[slot];
				const ReflectionProbeObject& probe = *probes[slot];
				if (slotState.IsCaptured && slotState.StaleFaces == 0)
				{
					continue;
				}
				const U32 faceMask = slotState.IsCaptured ? slotState.StaleFaces : ProbeCapture::k_allFaces;

				Rendering::Renderer::Begin("ReflectionProbeCapture");
				Rendering::Renderer::SetColourTarget(state.Scratch, state.ScratchDepth);
				Rendering::Renderer::ClearRenderTarget(state.Scratch, Vector4D(0.0f, 0.0f, 0.0f, 0.0f));
				Rendering::Renderer::ClearDepthStencilBuffer(state.ScratchDepth);
				// Far enough to see across the whole scene from anywhere in it
				const float farDistance = std::max(scene.GetBounds().GetRadius() * 2.0f, 1.0f);
				m_capture.BeginBatch(k_probeWidth, k_faceSize);
				m_capture.AddProbe(probe.GetPosition(), farDistance, 0, 0, faceMask);
				context.WorkCounts.ReflectionCaptureDraws += m_capture.Flush(scene);
				++context.WorkCounts.ReflectionProbesCaptured;
				Rendering::Renderer::ClearRenderTargets();
				// Only the faces captured: the others keep what they had
				for (U32 face = 0; face < ProbeCapture::k_faceCount; ++face)
				{
					if ((faceMask & (1u << face)) != 0)
					{
						Rendering::Renderer::CopyTextureRegion(state.Scratch, face * k_faceSize, 0, k_faceSize, k_faceSize, state.Atlas, face * k_faceSize, slot * k_faceSize);
					}
				}
				Rendering::Renderer::End();

				slotState.IsCaptured = true;
				slotState.StaleFaces = 0;
				slotState.ProbeRevision = probe.GetRevision();
				slotState.NeedsRelight = true;
				break;
			}

			// The irradiance probes' light bounces into these probes: while it's settling, one is relit in turn now and
			// then, and every one once it has settled
			const SceneIrradianceData& irradiance = m_sceneIrradianceInput.Get(context);
			if (state.WasIrradianceChanging && !irradiance.IsChanging)
			{
				for (ReflectionProbeState::Slot& slotState : state.Slots)
				{
					slotState.NeedsRelight = true;
				}
			}
			else if (irradiance.IsChanging && probeCount > 0 && context.FrameIndex % k_settlingRelightFrames == 0)
			{
				state.Slots[state.NextRelight % probeCount].NeedsRelight = true;
				state.NextRelight = (state.NextRelight + 1) % probeCount;
			}
			state.WasIrradianceChanging = irradiance.IsChanging;

			// Those whose light changed, the rest next frame
			for (U32 slot = 0; slot < probeCount && relit.Count < k_relightsPerFrame; ++slot)
			{
				ReflectionProbeState::Slot& slotState = state.Slots[slot];
				if (slotState.IsCaptured && slotState.NeedsRelight)
				{
					slotState.NeedsRelight = false;
					relit.Probes[relit.Count++] = slot;
				}
			}

			if (relit.Count > 0)
			{
				Rendering::Renderer::Begin("ReflectionProbeRelight");

				const SceneLightingData& sceneLighting = m_sceneLightingInput.Get(context);

				RelightParamsLayout relightParams;
				for (U32 slot = 0; slot < probeCount; ++slot)
				{
					const Vector3D& position = probes[slot]->GetPosition();
					relightParams.SlotPositions[slot][0] = position.X();
					relightParams.SlotPositions[slot][1] = position.Y();
					relightParams.SlotPositions[slot][2] = position.Z();
				}
				Rendering::Renderer::UploadDataToBuffer<RelightParamsLayout>(m_relightParamsBuffer, &relightParams);
				Rendering::Renderer::UploadDataToBuffer<GraphicsLayouts::ProbeListLayout>(m_probeListBuffer, &relit);

				// Lit as the irradiance probes light their captures, and by them
				Rendering::Renderer::SetComputeShader(m_relightShader->GetPlatformObject());
				Rendering::Renderer::SetConstantBufferCS(m_relightParamsBuffer, k_paramsSlot);
				Rendering::Renderer::SetConstantBufferCS(sceneLighting.LightBuffer, k_lightBufferSlot);
				Rendering::Renderer::SetConstantBufferCS(sceneLighting.ShadowBuffer, k_shadowBufferSlot);
				Rendering::Renderer::SetConstantBufferCS(irradiance.VolumeBuffer, k_volumeBufferSlot);
				Rendering::Renderer::SetConstantBufferCS(m_probeListBuffer, k_probeListSlot);
				Rendering::Renderer::SetTextureResourceCS(sceneLighting.DirectionalShadowMap, k_shadowMapSlot);
				Rendering::Renderer::SetTextureResourceCS(sceneLighting.StaticPointShadowCubes, k_pointShadowCubesSlot);
				Rendering::Renderer::SetTextureResourceCS(irradiance.SHRed, k_shRedSlot);
				Rendering::Renderer::SetTextureResourceCS(irradiance.SHGreen, k_shRedSlot + 1);
				Rendering::Renderer::SetTextureResourceCS(irradiance.SHBlue, k_shRedSlot + 2);
				Rendering::Renderer::SetTextureResourceCS(irradiance.Visibility, k_visibilitySlot);
				Rendering::Renderer::SetTextureResourceCS(state.Atlas, k_captureAtlasSlot);
				Rendering::Renderer::SetUnorderedAccess(state.Radiance, 0, 0);
				Rendering::Renderer::Dispatch(DivideRoundingUp(k_faceSize, k_groupSize), DivideRoundingUp(k_faceSize, k_groupSize), relit.Count * ProbeCapture::k_faceCount);
				context.WorkCounts.ReflectionProbesRelit += relit.Count;
				Rendering::Renderer::UnsetUnorderedAccess(0);
				for (U32 slot : { k_shadowMapSlot, k_pointShadowCubesSlot, k_shRedSlot, k_shRedSlot + 1, k_shRedSlot + 2, k_visibilitySlot, k_captureAtlasSlot })
				{
					Rendering::Renderer::UnsetTextureResourceCS(slot);
				}

				// Their mips, for the prefilter's wider lobes to read from: only the relit cubes', as the whole array
				// takes about 0.6 ms
				for (U32 listed = 0; listed < relit.Count; ++listed)
				{
					Rendering::Renderer::GenerateMips(state.Radiance, relit.Probes[listed] * ProbeCapture::k_faceCount, ProbeCapture::k_faceCount);
				}

				// A mip of the prefiltered cubes per gloss, as the forward pass picks them: (1 - gloss) x the last mip
				Rendering::Renderer::SetComputeShader(m_prefilterShader->GetPlatformObject());
				Rendering::Renderer::SetConstantBufferCS(m_prefilterParamsBuffer, k_paramsSlot);
				Rendering::Renderer::SetConstantBufferCS(m_probeListBuffer, k_probeListSlot);
				Rendering::Renderer::SetTextureResourceCS(state.Radiance, k_radianceSlot);
				for (U32 mip = 0; mip < k_prefilteredMipCount; ++mip)
				{
					const float gloss = 1.0f - static_cast<float>(mip) / static_cast<float>(k_prefilteredMipCount - 1);
					PrefilterParamsLayout prefilterParams;
					prefilterParams.Mip = mip;
					prefilterParams.MipSize = std::max(k_faceSize >> mip, 1u);
					// A Blinn-Phong exponent's lobe is about a Phong lobe a quarter as sharp
					prefilterParams.PhongExponent = std::exp2(gloss * k_maxShininessLog2) * 0.25f;
					Rendering::Renderer::UploadDataToBuffer<PrefilterParamsLayout>(m_prefilterParamsBuffer, &prefilterParams);

					Rendering::Renderer::SetUnorderedAccess(state.Prefiltered, 0, mip);
					Rendering::Renderer::Dispatch(DivideRoundingUp(prefilterParams.MipSize, k_groupSize), DivideRoundingUp(prefilterParams.MipSize, k_groupSize), relit.Count * ProbeCapture::k_faceCount);
				}
				Rendering::Renderer::UnsetUnorderedAccess(0);
				Rendering::Renderer::UnsetTextureResourceCS(k_radianceSlot);

				for (U32 listed = 0; listed < relit.Count; ++listed)
				{
					state.Slots[relit.Probes[listed]].IsLit = true;
				}

				Rendering::Renderer::End();
			}
		}

		// The lit probes, for the forward pass
		for (U32 slot = 0; slot < probeCount; ++slot)
		{
			if (!state.Slots[slot].IsLit)
			{
				continue;
			}

			const ReflectionProbeObject& probe = *probes[slot];
			const BoundingBox box = probe.GetBox();
			GraphicsLayouts::ReflectionProbeLayout& layout = probeBuffer.Probes[probeBuffer.Count++];
			for (int axis = 0; axis < 3; ++axis)
			{
				layout.BoxMin[axis] = box.GetMin()[axis];
				layout.BoxMax[axis] = box.GetMax()[axis];
				layout.Position[axis] = probe.GetPosition()[axis];
			}
			layout.BlendDistance = probe.GetBlendDistance();
			layout.Slot = static_cast<float>(slot);
		}
		if (probeBuffer.Count > 0)
		{
			reflection.Cubes = state.Prefiltered;
		}
	}

	// For the forward pass of this view
	Rendering::Renderer::UploadDataToBuffer<GraphicsLayouts::ReflectionProbeBufferLayout>(m_probeBuffer, &probeBuffer);
	m_sceneReflectionOutput.Publish(context, reflection);
}
