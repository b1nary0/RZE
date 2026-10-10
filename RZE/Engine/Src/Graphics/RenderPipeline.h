#pragma once

#include <Graphics/RenderEngine.h>
#include <Graphics/RenderPipelineCompiler.h>

#include <memory>
#include <vector>

class IRenderStage;
class RenderView;

// Owns the render stages and renders views with them, in the order CompileRenderPipeline() works out from
// what each stage declares in Setup().
class RenderPipeline
{
public:
	RenderPipeline();
	~RenderPipeline();

public:
	// Initializes the stage. The stages are recompiled before the pipeline next renders.
	void AddStage(std::unique_ptr<IRenderStage> stage);
	void Clear();

	void Render(RenderView& view, const RenderEngine::SceneData& sceneData, float deltaTime);

private:
	void CompileIfDirty();

private:
	// In the order they were added; the compiled pipeline's indices refer to these
	std::vector<std::unique_ptr<IRenderStage>> m_stages;
	CompiledRenderPipeline m_compiled;
	bool m_isDirty = true;
};
