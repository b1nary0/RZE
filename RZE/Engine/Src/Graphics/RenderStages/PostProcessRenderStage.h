#pragma once

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
	void Initialize() override;
	void Update(const RenderCamera& camera, const RenderEngine::SceneData& renderData) override {}
	void Render(const RenderCamera& camera, const RenderEngine::SceneData& renderData) override;

	// After ForwardRenderStage, before DebugDrawRenderStage so debug lines aren't tonemapped
	U32 GetPriority() override { return 20; }

private:
	ResourceHandle m_vertexShaderResource;
	const VertexShader* m_vertexShader = nullptr;

	ResourceHandle m_luminanceShaderResource;
	const PixelShader* m_luminanceShader = nullptr;

	ResourceHandle m_adaptShaderResource;
	const PixelShader* m_adaptShader = nullptr;

	ResourceHandle m_tonemapShaderResource;
	const PixelShader* m_tonemapShader = nullptr;

	// Weighted log-luminance (r) and weight (g); its 1x1 mip holds the scene average
	Rendering::TextureBuffer2DHandle m_luminance;
	// 1x1 adapted log-luminance (r) and has-been-measured flag (g). Ping-ponged: read last frame's, write this frame's.
	Rendering::TextureBuffer2DHandle m_adapted[2];
	U32 m_adaptedIndex = 0;
	// Written instead by secondary views (RenderView), which don't adapt over time
	Rendering::TextureBuffer2DHandle m_secondaryViewAdapted;

	Rendering::ConstantBufferHandle m_paramsBuffer;

	// The first frame's history is uninitialized texture memory
	bool m_needsReset = true;
};
