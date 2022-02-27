#pragma once

#include <Graphics/RenderStage.h>

#include <EngineCore/Resources/ResourceHandler.h>

class VertexShader;

class ForwardRenderStage : public IRenderStage
{
public:
	ForwardRenderStage() = default;
	~ForwardRenderStage() override = default;

public:
	void Initialize() override;
	void Update(const RenderStageData& renderData) override;
	void Render(const RenderStageData& renderData) override;

	U32 GetPriority() override { return 0; }

private:
	// @TODO temp until ShaderTechniques are properly implemented
// (could be a while)
	ResourceHandle m_vertexShaderResource;
	const VertexShader* m_vertexShader = nullptr; // @TODO This is just to avoid having to GetResource() in a loop but is more a deficiency of the Resource API
};