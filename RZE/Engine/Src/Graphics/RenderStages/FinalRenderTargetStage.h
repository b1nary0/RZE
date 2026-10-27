#pragma once

#include <EngineCore/Resources/TypedResourceHandle.h>

#include <Graphics/RenderData/DisplayColourData.h>
#include <Graphics/RenderStage.h>

#include <Graphics/Shader.h>

class FinalRenderTargetStage : public IRenderStage
{
public:
	FinalRenderTargetStage() = default;
	~FinalRenderTargetStage() override = default;

public:
	const char* GetName() const override { return "FinalRenderTargetStage"; }

	void Initialize() override;
	void Setup(RenderStageBuilder& builder) override;
	void Render(RenderContext& context) override;

private:
	RenderInput<DisplayColourData> m_displayColourInput;

	TypedResourceHandle<VertexShader> m_vertexShader;

	TypedResourceHandle<PixelShader> m_pixelShader;
};