#pragma once

#include <Graphics/RenderStage.h>

#include <Graphics/Shader.h>

#include <limits>

class FinalRenderTargetStage : public IRenderStage
{
public:
	FinalRenderTargetStage() = default;
	~FinalRenderTargetStage() override = default;

public:
	void Initialize() override;
	void Update(const RenderStageData& renderData) override;
	void Render(const RenderStageData& renderData) override;

	U32 GetPriority() override { return std::numeric_limits<U32>::max(); }

private:
	ResourceHandle m_vertexShaderResource;
	const VertexShader* m_vertexShader = nullptr;

	ResourceHandle m_pixelShaderResource;
	const PixelShader* m_pixelShader;
};