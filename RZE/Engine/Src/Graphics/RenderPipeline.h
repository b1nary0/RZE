#pragma once

#include <Graphics/RenderPipelineCompiler.h>
#include <Graphics/RenderWorkCounts.h>
#include <Graphics/VisibleDrawsCache.h>

#include <Rendering/DrawCounts.h>
#include <Rendering/Driver/GPUStatistics.h>

#include <memory>
#include <string>
#include <vector>

class IRenderStage;
class RenderScene;
class RenderView;

// A render stage's Render(), across every view it renders: how long it took to record and the draws it recorded, and
// how long the GPU took to run it and what it counted doing so. Doubles, so RenderStats can hold their averages in the
// same struct.
struct RenderStageStats
{
	std::string Name;
	double TimeMS = 0.0;
	double DrawCalls = 0.0;
	double Instances = 0.0;
	// Already per frame: the render thread's average over its own stats window (RenderThreadStats::GPUScopeTimesMS)
	double GPUTimeMS = 0.0;
	// Already per frame, the same way (RenderThreadStats::GPUScopeStatistics); zero without them
	// (RenderThreadStats::HasGPUStatistics)
	Rendering::GPUPipelineStatistics GPUStatistics;
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

	// Each stage's time and draws since the last call, in the order they run, after a "Culling" row, then starts them
	// over. Culling is the time spent culling the scene for every stage (VisibleDrawsCache), taken out of the time of
	// the stage that asked first. The stages' GPU times and statistics are the render thread's latest averages.
	std::vector<RenderStageStats> TakeStageStats();
	// What the stages have counted since the last call (RenderContext::WorkCounts), then starts them over
	RenderWorkCounts TakeWorkCounts();

private:
	void CompileIfDirty();

private:
	// In the order they were added; the compiled pipeline's indices refer to these
	std::vector<std::unique_ptr<IRenderStage>> m_stages;
	// Seconds each stage has spent in Render(), and the draws it recorded, since TakeStageStats(), indexed like m_stages
	std::vector<double> m_stageTimes;
	std::vector<Rendering::DrawCounts> m_stageDrawCounts;
	// What the stages see of the scene, reset for each view
	VisibleDrawsCache m_visibleDraws;
	// Seconds spent culling, since TakeStageStats()
	double m_cullTime = 0.0;
	// Since TakeWorkCounts()
	RenderWorkCounts m_workCounts;
	CompiledRenderPipeline m_compiled;
	bool m_isDirty = true;
};
