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
	void Update(const RenderCamera& camera, const RenderEngine::SceneData& renderData) override;
	void Render(const RenderCamera& camera, const RenderEngine::SceneData& renderData) override;

	// After ShadowRenderStage (0), which produces the shadow map sampled here
	U32 GetPriority() override { return 10; }

private:
	// @TODO temp until ShaderTechniques are properly implemented
// (could be a while)
	ResourceHandle m_vertexShaderResource;
	const VertexShader* m_vertexShader = nullptr; // @TODO This is just to avoid having to GetResource() in a loop but is more a deficiency of the Resource API

	// Zero-strength light bound when the scene has none (e.g. while a scene is streaming in).
	std::unique_ptr<LightObject> m_fallbackLight;
};