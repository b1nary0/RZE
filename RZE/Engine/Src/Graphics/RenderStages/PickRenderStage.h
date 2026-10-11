#pragma once

#include <Graphics/RenderStage.h>

#include <EngineCore/Resources/ResourceHandler.h>

#include <Rendering/BufferHandle.h>

class VertexShader;
class PixelShader;

// Finds the object drawn at one pixel of the main view (RenderView::Pick), for the editor's click selection.
// Only runs on frames with a request. The requested pixel is stretched over a 1x1 ID target, so culling skips
// almost everything and a single texel is read back. Cutout pixels are clipped as in the shadow pass, so
// clicks go through them.
class PickRenderStage : public IRenderStage
{
public:
	// Mirrors PickBuffer in Pixel_PickID.hlsl
	struct PickBufferLayout
	{
		U32 PickID;
		U32 _pad[3];
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
	ResourceHandle m_vertexShaderResource;
	const VertexShader* m_vertexShader = nullptr;

	ResourceHandle m_pixelShaderResource;
	const PixelShader* m_pixelShader = nullptr;

	Rendering::RenderTargetHandle m_idTarget;
	Rendering::TextureBuffer2DHandle m_depthTarget;
	// Separate from the vertex shader's own camera buffer so the forward pass's camera isn't overwritten
	Rendering::ConstantBufferHandle m_cameraBuffer;
	Rendering::ConstantBufferHandle m_pickBuffer;
};
