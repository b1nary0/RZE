#pragma once

#include <Graphics/AntiAliasing.h>
#include <Graphics/RenderData/DisplayColourData.h>
#include <Graphics/RenderData/TonemappedColourData.h>
#include <Graphics/RenderStage.h>

#include <EngineCore/Resources/TypedResourceHandle.h>

#include <Rendering/BufferHandle.h>

#include <cstddef>

class VertexShader;
class PixelShader;

class RenderView;

// Smooths jagged edges in PostProcessRenderStage's tonemapped image, into the view's target, by the view's
// AntiAliasing method and quality:
// - FXAA: one pass, Pixel_FXAA.
// - SMAA: three. Edge detection into the view's Edges texture, blending weights into its BlendWeights (SMAAState,
//   kept per view), then neighbourhood blending into the target.
// With the method Off, the tonemap is already in the target, so it draws nothing and passes the target on.
class AntiAliasingStage : public IRenderStage
{
public:
	// Mirrors FXAAParams in Pixel_FXAA.hlsl
	struct FXAAParamsLayout
	{
		float ViewportScale[2];
		// The view's FXAASettings
		float EdgeThreshold;
		float EdgeThresholdMin;
		float Subpixel;
		float SearchStepCount;
		float _pad0[2];
		// Starts on a register, as HLSL's float4 array does
		float SearchSteps[FXAASettings::k_maxSearchSteps];
	};
	static_assert(sizeof(FXAAParamsLayout) % 16 == 0, "Constant buffers are whole 16-byte registers");
	static_assert(offsetof(FXAAParamsLayout, SearchSteps) % 16 == 0, "HLSL starts the float4 array on a register");

	// Mirrors SMAAParams in Common/SMAA/SMAA.hlsli
	struct SMAAParamsLayout
	{
		// 1 / width, 1 / height, width, height of the target
		float RTMetrics[4];
		// The view's SMAASettings
		float Threshold;
		float MaxSearchSteps;
		float MaxSearchStepsDiag;
		float CornerRounding;
	};
	static_assert(sizeof(SMAAParamsLayout) % 16 == 0, "Constant buffers are whole 16-byte registers");

public:
	AntiAliasingStage() = default;
	~AntiAliasingStage() override = default;

public:
	const char* GetName() const override { return "AntiAliasingStage"; }

	void Initialize() override;
	void Setup(RenderStageBuilder& builder) override;
	void Render(RenderContext& context) override;

private:
	void DrawFXAA(const RenderView& view, const Rendering::TextureBuffer2DHandle& source, const Rendering::RenderTargetHandle& target);
	void DrawSMAA(RenderContext& context, const Rendering::TextureBuffer2DHandle& source, const Rendering::RenderTargetHandle& target);

private:
	RenderInput<TonemappedColourData> m_tonemappedColourInput;
	RenderOutput<DisplayColourData> m_displayColourOutput;

	TypedResourceHandle<VertexShader> m_vertexShader;
	TypedResourceHandle<PixelShader> m_fxaaShader;
	Rendering::ConstantBufferHandle m_fxaaParamsBuffer;

	TypedResourceHandle<PixelShader> m_smaaEdgeShader;
	// With diagonal and corner detection, and without
	TypedResourceHandle<PixelShader> m_smaaWeightsShader;
	TypedResourceHandle<PixelShader> m_smaaWeightsNoDiagonalsShader;
	TypedResourceHandle<PixelShader> m_smaaBlendShader;
	Rendering::ConstantBufferHandle m_smaaParamsBuffer;
	// SMAA's precomputed lookups, the same for every view: the blend each edge shape gets (RG8, 160x560), and where
	// a search lands relative to an edge's end (R8, 64x16)
	Rendering::TextureBuffer2DHandle m_areaTexture;
	Rendering::TextureBuffer2DHandle m_searchTexture;
};
