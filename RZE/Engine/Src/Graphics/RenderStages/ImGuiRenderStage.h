#pragma once

#include <Graphics/RenderStage.h>

class ImGuiRenderStage : public IRenderStage
{
public:
	ImGuiRenderStage() = default;
	ImGuiRenderStage(bool withEditor)
		: m_withEditor (withEditor) {}

	~ImGuiRenderStage() override = default;

	void Initialize() override;
	void Update(const RenderStageData& renderData) override {}
	void Render(const RenderStageData& renderData) override;

	U32 GetPriority() override { return 1000; }

private:
	bool m_withEditor = false;
};