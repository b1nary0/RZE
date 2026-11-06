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

// Smooths jagged edges in PostProcessRenderStage's tonemapped image, into the view's target, at the view's
// AntiAliasing level: FXAA (Pixel_FXAA). With AntiAliasing Off, the tonemap is already in the target, so it draws
// nothing and passes the target on.
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

public:
	AntiAliasingStage() = default;
	~AntiAliasingStage() override = default;

public:
	const char* GetName() const override { return "AntiAliasingStage"; }

	void Initialize() override;
	void Setup(RenderStageBuilder& builder) override;
	void Render(RenderContext& context) override;

private:
	RenderInput<TonemappedColourData> m_tonemappedColourInput;
	RenderOutput<DisplayColourData> m_displayColourOutput;

	TypedResourceHandle<VertexShader> m_vertexShader;
	TypedResourceHandle<PixelShader> m_fxaaShader;

	Rendering::ConstantBufferHandle m_fxaaParamsBuffer;
};
