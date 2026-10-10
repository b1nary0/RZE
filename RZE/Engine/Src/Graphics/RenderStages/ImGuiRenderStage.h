#pragma once

#include <Graphics/RenderData/DisplayColourData.h>
#include <Graphics/RenderStage.h>

class ImGuiRenderStage : public IRenderStage
{
public:
	ImGuiRenderStage() = default;
	// withEditor: draw the editor UI to the back buffer. Otherwise ImGui is overlaid on the view's display colour.
	ImGuiRenderStage(bool withEditor)
		: m_withEditor (withEditor) {}

	~ImGuiRenderStage() override = default;

	const char* GetName() const override { return "ImGuiRenderStage"; }

	void Initialize() override;
	void Setup(RenderStageBuilder& builder) override;
	void Render(RenderContext& context) override;

private:
	bool m_withEditor = false;

	// Only the one matching m_withEditor is declared:
	// with the editor, the UI shows the finished view through ImGui::Image, so it only needs to run after it
	RenderInput<DisplayColourData> m_displayedView;
	// without it, ImGui is drawn over the view
	RenderInOut<DisplayColourData> m_overlayTarget;
};
