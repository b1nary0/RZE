#pragma once

#include <Graphics/RenderStage.h>

#include <EngineCore/Resources/TypedResourceHandle.h>

#include <Rendering/BufferHandle.h>

class VertexShader;
class PixelShader;

// Finds the object drawn at one pixel of the main view (RenderFrameRequests::Pick), for the editor's click selection.
// Only runs on frames with a request. The requested pixel is stretched over a 1x1 ID target, so culling skips
// almost everything and a single texel is read back, plus the depth texel if the request asks for it. Cutout pixels
// are clipped as in the shadow pass, so clicks go through them.
class PickRenderStage : public IRenderStage
{
public:
	// Mirrors PickBuffer in Pixel_PickID.hlsl, where PickID is a uint2 (low half, high half)
	struct PickBufferLayout
	{
		U64 PickID;
		U32 _pad[2];
	};

public:
	PickRenderStage() = default;
	~PickRenderStage() override = default;

public:
	const char* GetName() const override { return "PickRenderStage"; }

	void Initialize() override;
	void Setup(RenderStageBuilder& builder) override;
	void Render(RenderContext& context) override;

private:
	TypedResourceHandle<VertexShader> m_vertexShader;

	TypedResourceHandle<PixelShader> m_pixelShader;

	Rendering::RenderTargetHandle m_idTarget;
	Rendering::TextureBuffer2DHandle m_depthTarget;
	// Separate from the vertex shader's own camera buffer so the forward pass's camera isn't overwritten
	Rendering::ConstantBufferHandle m_cameraBuffer;
	Rendering::ConstantBufferHandle m_pickBuffer;
};
