#pragma once

#include <Graphics/ProbeCapture.h>
#include <Graphics/RenderData/SceneIrradianceData.h>
#include <Graphics/RenderData/SceneLightingData.h>
#include <Graphics/RenderData/SceneReflectionData.h>
#include <Graphics/RenderStage.h>

#include <EngineCore/Resources/TypedResourceHandle.h>

#include <Rendering/BufferHandle.h>

class ComputeShader;
struct GlobalIlluminationSettings;

// The scene's reflection probes (ReflectionProbeObject, Docs/Plans/SSAOAndGlobalIllumination.md, Part 4): the first
// GraphicsLayouts::k_maxReflectionProbes the scene made, a slot each. Each frame:
//   - At most one probe is captured (ProbeCapture, 128 texels a face, in CubeMap's layout): a new one, or one that
//     moved or was resized, or whose box a Static object changed in, once that has stayed still for
//     GlobalIlluminationSettings::RecaptureSettleFrames.
//   - Up to two are relit, it and then the others in turn (Compute_ReflectionRelight), with the scene's lights and
//     irradiance probes, into a radiance cube array, which then gets its mips.
//   - Their prefiltered cubes are made from that (Compute_ReflectionPrefilter): a mip per gloss.
// The forward pass reflects them (Common/ReflectionProbes.hlsli). As IrradianceProbeStage, the work is once a frame for
// each scene, for views with global illumination; other views get no probes.
class ReflectionProbeStage : public IRenderStage
{
public:
	// Mirrors ReflectionRelightParams in Compute_ReflectionRelight.hlsl
	struct RelightParamsLayout
	{
		float SlotPositions[16][4] = {};
	};

	// Mirrors PrefilterParams in Compute_ReflectionPrefilter.hlsl
	struct PrefilterParamsLayout
	{
		U32 Mip = 0;
		U32 MipSize = 0;
		float PhongExponent = 0.0f;
		U32 _pad = 0;
	};

public:
	// settings: RenderEngine's, read each frame
	explicit ReflectionProbeStage(const GlobalIlluminationSettings* settings);
	~ReflectionProbeStage() override = default;

public:
	const char* GetName() const override { return "ReflectionProbeStage"; }

	void Initialize() override;
	void Setup(RenderStageBuilder& builder) override;
	void Render(RenderContext& context) override;

private:
	const GlobalIlluminationSettings* m_settings;

	RenderInput<SceneLightingData> m_sceneLightingInput;
	RenderInput<SceneIrradianceData> m_sceneIrradianceInput;
	RenderOutput<SceneReflectionData> m_sceneReflectionOutput;

	ProbeCapture m_capture;
	TypedResourceHandle<ComputeShader> m_relightShader;
	TypedResourceHandle<ComputeShader> m_prefilterShader;
	Rendering::ConstantBufferHandle m_probeListBuffer;
	Rendering::ConstantBufferHandle m_relightParamsBuffer;
	Rendering::ConstantBufferHandle m_prefilterParamsBuffer;
	// GraphicsLayouts::ReflectionProbeBufferLayout, uploaded for each view rendered
	Rendering::ConstantBufferHandle m_probeBuffer;
	// Bound where there are no probes, so the forward pass always has a cube array
	Rendering::TextureBuffer2DHandle m_placeholderCubes;
};
