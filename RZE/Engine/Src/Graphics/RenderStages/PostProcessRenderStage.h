#pragma once

#include <Graphics/RenderData/SceneColourData.h>
#include <Graphics/RenderData/TonemappedColourData.h>
#include <Graphics/RenderStage.h>

#include <EngineCore/Resources/TypedResourceHandle.h>

#include <Rendering/BufferHandle.h>

class VertexShader;
class PixelShader;

// Turns ForwardRenderStage's linear scene colour into an 8-bit, display-ready image. Exposure adapts
// automatically: the scene's log-average luminance is measured each frame, smoothed over time,
// and mapped to a mid-grey key value before the ACES tonemap and sRGB encode. With the view's AntiAliasing Off, the
// tonemap goes straight to the view's target; otherwise to an intermediate texture that AntiAliasingStage smooths
// into the target.
class PostProcessRenderStage : public IRenderStage
{
public:
	// Mirrors PostProcessParams in Common/PostProcess.hlsli
	struct ParamsLayout
	{
		float ViewportScale[2];
		float DeltaTime;
		float AdaptationRate;
		float MinExposure;
		float MaxExposure;
		float ExposureCompensation;
		float KeyValue;
		float LuminanceMip;
		float Reset;
		float AutoExposure;
		float _pad0;
	};
	static_assert(sizeof(ParamsLayout) % 16 == 0, "Constant buffers are whole 16-byte registers");

public:
	PostProcessRenderStage() = default;
	~PostProcessRenderStage() override = default;

public:
	const char* GetName() const override { return "PostProcessRenderStage"; }

	void Initialize() override;
	void Setup(RenderStageBuilder& builder) override;
	void Render(RenderContext& context) override;

private:
	RenderInput<SceneColourData> m_sceneColourInput;
	RenderOutput<TonemappedColourData> m_tonemappedColourOutput;

	TypedResourceHandle<VertexShader> m_vertexShader;

	TypedResourceHandle<PixelShader> m_luminanceShader;

	TypedResourceHandle<PixelShader> m_adaptShader;

	TypedResourceHandle<PixelShader> m_tonemapShader;

	// Weighted log-luminance (r) and weight (g); its 1x1 mip holds the scene average.
	// Only used within a Render() call, so shared by every view. Eye-adaptation history is per view.
	Rendering::TextureBuffer2DHandle m_luminance;

	Rendering::ConstantBufferHandle m_paramsBuffer;
};
