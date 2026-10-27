#pragma once

#include <Graphics/RenderData/SceneDepthData.h>
#include <Graphics/RenderStage.h>

#include <EngineCore/Resources/TypedResourceHandle.h>

class PixelShader;
class VertexShader;

// Draws the depth of everything that isn't blended before any shading, so the forward pass shades each pixel once,
// by its nearest surface, and so later passes (ambient occlusion) can read the scene's depth before lighting it.
// Opaque draws run no pixel shader; cutouts run one that only clips (Docs/Plans/SSAOAndGlobalIllumination.md).
class DepthPrepassStage : public IRenderStage
{
public:
	DepthPrepassStage() = default;
	~DepthPrepassStage() override = default;

public:
	const char* GetName() const override { return "DepthPrepassStage"; }

	void Initialize() override;
	void Setup(RenderStageBuilder& builder) override;
	void Render(RenderContext& context) override;

private:
	RenderOutput<SceneDepthData> m_sceneDepthOutput;

	// The forward pass's, so both passes compute the same depths
	TypedResourceHandle<VertexShader> m_vertexShader;
	TypedResourceHandle<PixelShader> m_cutoutShader;
};
