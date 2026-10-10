#pragma once

#include <Graphics/RenderData/SceneColourData.h>
#include <Graphics/RenderData/ShadowMapData.h>
#include <Graphics/RenderStage.h>

#include <EngineCore/Resources/ResourceHandler.h>

class VertexShader;

class ForwardRenderStage : public IRenderStage
{
public:
	ForwardRenderStage() = default;
	~ForwardRenderStage() override = default;

public:
	const char* GetName() const override { return "ForwardRenderStage"; }

	void Initialize() override;
	void Setup(RenderStageBuilder& builder) override;
	void Render(RenderContext& context) override;

private:
	RenderInput<ShadowMapData> m_shadowMapInput;
	RenderOutput<SceneColourData> m_sceneColourOutput;

	// @TODO temp until ShaderTechniques are properly implemented
// (could be a while)
	ResourceHandle m_vertexShaderResource;
	const VertexShader* m_vertexShader = nullptr; // @TODO This is just to avoid having to GetResource() in a loop but is more a deficiency of the Resource API

	// Zero-strength light bound when the scene has none (e.g. while a scene is streaming in).
	std::unique_ptr<LightObject> m_fallbackLight;
};