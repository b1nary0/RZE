#pragma once

#include <Graphics/RenderPipeline.h>

class ImGuiRenderPipeline : public IRenderPipeline
{
public:
	ImGuiRenderPipeline() = default;
	~ImGuiRenderPipeline() override = default;

	void Initialize() override;
	void Update(const std::vector<std::shared_ptr<RenderObject>>& renderObjects) override {}
	void Render(const std::vector<std::shared_ptr<RenderObject>>& renderObjects) override;

	U32 GetPriority() override { return 1000; }
};