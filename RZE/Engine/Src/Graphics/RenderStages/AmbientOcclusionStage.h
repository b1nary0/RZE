#pragma once

#include <Graphics/RenderData/AmbientOcclusionData.h>
#include <Graphics/RenderData/SceneDepthData.h>
#include <Graphics/RenderStage.h>

#include <EngineCore/Resources/TypedResourceHandle.h>

#include <Rendering/BufferHandle.h>

class ComputeShader;

// Ground Truth Ambient Occlusion (GTAO), ported from Intel's XeGTAO, from the depth prepass: how much creases and corners
// hide each pixel from the light around it. Three compute passes, at the view's EAmbientOcclusion level:
//   1. Compute_GTAOPrefilterDepths: device depth to view-space distance, with 4 smaller mips for far samples
//   2. Compute_GTAOMain: the visibility, from the horizons a few directions around each pixel, and its depth edges
//   3. Compute_GTAODenoise: an edge-aware blur, once or twice
// Views with it Off, Uniform lighting or no perspective Projection get a white texture instead
// (Docs/Plans/SSAOAndGlobalIllumination.md).
class AmbientOcclusionStage : public IRenderStage
{
public:
	// Mirrors GTAOParams in Common/GTAO.hlsli
	struct ParamsLayout
	{
		float ViewportSize[2];
		float ViewportPixelSize[2];
		float DepthUnpackConsts[2];
		float CameraTanHalfFOV[2];
		float NDCToViewMul[2];
		float NDCToViewAdd[2];
		float NDCToViewMul_x_PixelSize[2];
		float EffectRadius;
		float EffectFalloffRange;
		float RadiusMultiplier;
		float FinalValuePower;
		float DenoiseBlurBeta;
		float SampleDistributionPower;
		float ThinOccluderCompensation;
		float DepthMIPSamplingOffset;
		float SliceCount;
		float StepsPerSlice;
		float FinalApply;
		float _pad[3];
	};

public:
	AmbientOcclusionStage() = default;
	~AmbientOcclusionStage() override = default;

public:
	const char* GetName() const override { return "AmbientOcclusionStage"; }

	void Initialize() override;
	void Setup(RenderStageBuilder& builder) override;
	void Render(RenderContext& context) override;

private:
	RenderInput<SceneDepthData> m_sceneDepthInput;
	RenderOutput<AmbientOcclusionData> m_ambientOcclusionOutput;

	TypedResourceHandle<ComputeShader> m_prefilterDepthsShader;
	TypedResourceHandle<ComputeShader> m_mainShader;
	TypedResourceHandle<ComputeShader> m_denoiseShader;

	Rendering::ConstantBufferHandle m_paramsBuffer;
	// What views without ambient occlusion publish
	Rendering::TextureBuffer2DHandle m_white;
};
