#pragma once

#include <Graphics/RenderData/AmbientOcclusionData.h>
#include <Graphics/RenderData/DisplayColourData.h>
#include <Graphics/RenderStage.h>

#include <EngineCore/Resources/TypedResourceHandle.h>

class VertexShader;
class PixelShader;

// Shows one of the passes that make a view's image in place of the lit scene, for RenderView::Debug: drawn over the
// finished, anti-aliased image, so what's shown is exactly the pass's output. Only AmbientOcclusion is drawn here;
// IndirectDiffuse is shaded by the forward pass itself. Overlays added after it (debug lines, the UI) still draw on top.
class DebugViewStage : public IRenderStage
{
public:
	DebugViewStage() = default;
	~DebugViewStage() override = default;

public:
	const char* GetName() const override { return "DebugViewStage"; }

	void Initialize() override;
	void Setup(RenderStageBuilder& builder) override;
	void Render(RenderContext& context) override;

private:
	RenderInput<AmbientOcclusionData> m_ambientOcclusionInput;
	RenderInOut<DisplayColourData> m_displayColour;

	TypedResourceHandle<VertexShader> m_vertexShader;
	TypedResourceHandle<PixelShader> m_ambientOcclusionShader;
};
