#pragma once

#include <Graphics/RenderData/DisplayColourData.h>
#include <Graphics/RenderData/SceneColourData.h>
#include <Graphics/RenderStage.h>

#include <EngineCore/Resources/ResourceHandler.h>

#include <Rendering/BufferHandle.h>

class VertexShader;
class PixelShader;

// Turns ForwardRenderStage's linear scene colour into the 8-bit display target. Exposure adapts
// automatically: the scene's log-average luminance is measured each frame, smoothed over time,
// and mapped to a mid-grey key value before the ACES tonemap and sRGB encode.
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
		float _pad0[2];
	};

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
	RenderOutput<DisplayColourData> m_displayColourOutput;

	ResourceHandle m_vertexShaderResource;
	const VertexShader* m_vertexShader = nullptr;

	ResourceHandle m_luminanceShaderResource;
	const PixelShader* m_luminanceShader = nullptr;

	ResourceHandle m_adaptShaderResource;
	const PixelShader* m_adaptShader = nullptr;

	ResourceHandle m_tonemapShaderResource;
	const PixelShader* m_tonemapShader = nullptr;

	// Weighted log-luminance (r) and weight (g); its 1x1 mip holds the scene average.
	// Only used within a Render() call, so shared by every view. Eye-adaptation history is per view.
	Rendering::TextureBuffer2DHandle m_luminance;

	Rendering::ConstantBufferHandle m_paramsBuffer;
};
