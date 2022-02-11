#pragma once

#include <Graphics/RenderPipeline.h>

class VertexShader;

class ForwardRenderPipeline : public IRenderPipeline
{
public:
	ForwardRenderPipeline() = default;
	virtual ~ForwardRenderPipeline() = default;

public:
	void Initialize() override;
	void Update(const std::vector<std::shared_ptr<RenderObject>>& renderObjects) override;
	void Render(const std::vector<std::shared_ptr<RenderObject>>& renderObjects) override;

	U32 GetPriority() override { return 0; }

private:
	// @TODO temp until ShaderTechniques are properly implemented
// (could be a while)
	ResourceHandle m_vertexShaderResource;
	const VertexShader* m_vertexShader = nullptr; // @TODO This is just to avoid having to GetResource() in a loop but is more a deficiency of the Resource API
};