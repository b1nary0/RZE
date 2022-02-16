#pragma once

#include <Graphics/RenderStage.h>

class ImGuiRenderStage : public IRenderStage
{
public:
	ImGuiRenderStage() = default;
	~ImGuiRenderStage() override = default;

	void Initialize() override;
	void Update(const std::vector<std::shared_ptr<RenderObject>>& renderObjects) override {}
	void Render(const std::vector<std::shared_ptr<RenderObject>>& renderObjects) override;

	U32 GetPriority() override { return 1000; }
};