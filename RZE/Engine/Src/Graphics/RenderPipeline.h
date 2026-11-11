#pragma once

#include <Graphics/RenderPipelineCompiler.h>
#include <Graphics/RenderWorkCounts.h>

#include <memory>
#include <string>
#include <vector>

class IRenderStage;
class RenderScene;
class RenderView;

// A render stage's Render(), across every view it renders: how long it took to record and the draw calls it recorded,
// and how long the GPU took to run it. Doubles, so RenderStats can hold their averages in the same struct.
struct RenderStageStats
{
	std::string Name;
	double TimeMS = 0.0;
	double DrawCalls = 0.0;
	// Already per frame: the render thread's average over its own stats window (RenderThreadStats::GPUScopeTimesMS)
	double GPUTimeMS = 0.0;
};

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

	// frameIndex: RenderEngine's, the same for every view rendered in one frame
	void Render(RenderView& view, RenderScene& scene, float deltaTime, U64 frameIndex);

	// Each stage's time and draw calls since the last call, in the order they run, then starts them over. Their GPU
	// times are the render thread's latest averages.
	std::vector<RenderStageStats> TakeStageStats();
	// What the stages have counted since the last call (RenderContext::WorkCounts), then starts them over
	RenderWorkCounts TakeWorkCounts();

private:
	void CompileIfDirty();

private:
	// In the order they were added; the compiled pipeline's indices refer to these
	std::vector<std::unique_ptr<IRenderStage>> m_stages;
	// Seconds each stage has spent in Render(), and the draw calls it recorded, since TakeStageStats(), indexed like
	// m_stages
	std::vector<double> m_stageTimes;
	std::vector<U32> m_stageDrawCalls;
	// Since TakeWorkCounts()
	RenderWorkCounts m_workCounts;
	CompiledRenderPipeline m_compiled;
	bool m_isDirty = true;
};
