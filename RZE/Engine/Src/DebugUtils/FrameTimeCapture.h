#pragma once

#include <Rendering/Driver/GPUStatistics.h>

#include <Utils/PrimitiveDefs.h>

#include <string>
#include <vector>

struct RenderStats;

// One frame's timings and counts for FrameTimeCapture, each from the frame before the one starting
struct FrameTimeSample
{
	float FrameMS = 0.0f;
	// The async operations' tick, the application's PreUIUpdate, UpdateUI, Update and PreRender, and the scene's update,
	// but not ImGui's NewFrame and EndFrame or rendering
	float UpdateMS = 0.0f;
	// Recording every view's render stages (RenderFrameCounts)
	float RecordMS = 0.0f;
	// The render thread's last presented frame: running its commands, Present included, and the part in Present
	// (Rendering::RenderThreadStats). It runs a frame behind the main thread.
	float RenderThreadBusyMS = 0.0f;
	float RenderThreadPresentMS = 0.0f;
	// The GPU's time for the newest frame read back, a few frames older than the rest (Rendering::RenderThreadStats)
	float GPUMS = 0.0f;
	// Not counting the UI's
	U32 DrawCalls = 0;
	U32 Instances = 0;
	// What the GPU counted in the frame GPUMS is for (Rendering::RenderThreadStats): unlike DrawCalls, with the UI's
	// (ImGuiRenderStage). Zero without them.
	Rendering::GPUPipelineStatistics GPUStatistics;
};

// Records every frame's timings for A/B performance comparisons, enabled from the command line:
//   -perfcapture <file>      where to write the samples: a header line, then one line per frame with
//                            FrameTimeSample's values
//                            (frameMs,updateMs,recordMs,renderBusyMs,renderPresentMs,gpuMs,drawCalls,instances,
//                            triangles,verticesShaded,trianglesInView,pixelsShaded). Beside it, <file>.stages.csv has
//                            each render stage's record time, draws, GPU time and GPU statistics a frame, averaged over
//                            the capture. The GPU statistics are zero unless -gpustats is given, or with a driver that can't count them.
//   -perfwarmup <seconds>    how long to wait before recording, e.g. for the scene to load (default 10)
//   -perfduration <seconds>  how long to record (default 15)
// When the duration is up the samples are written and the engine exits. A capture turns frame pacing off (vsync and
// frame rate caps) so it measures what frames cost; -vsync and -maxfps still apply (RZE_Engine::InitializeFramePacing).
class FrameTimeCapture
{
public:
	FrameTimeCapture() = default;
	~FrameTimeCapture() = default;

public:
	// Reads the command line; the capture stays disabled without -perfcapture
	void Initialize();

	bool IsEnabled() const { return m_isEnabled; }

	// Once per frame. elapsedSeconds is the time since the engine loop started; sample is the previous frame's, and
	// renderStats the render engine's current averages, which give the stages' figures. Returns true once the capture
	// has been written, meaning the engine should exit.
	bool AddFrame(double elapsedSeconds, const FrameTimeSample& sample, const RenderStats& renderStats);

private:
	void Write() const;
	void WriteStages() const;

private:
	// A render stage's figures, summed over the frames recorded
	struct StageTotals
	{
		std::string Name;
		double TimeMS = 0.0;
		double DrawCalls = 0.0;
		double Instances = 0.0;
		double GPUTimeMS = 0.0;
		Rendering::GPUPipelineStatistics GPUStatistics;
	};

	bool m_isEnabled = false;
	std::string m_outputPath;
	double m_warmupSeconds = 10.0;
	double m_durationSeconds = 15.0;

	std::vector<FrameTimeSample> m_samples;
	// From RenderStats' averages, each frame, so the capture's average is theirs averaged over every frame recorded
	std::vector<StageTotals> m_stageTotals;
	// The frames m_stageTotals has added up
	size_t m_stageFrameCount = 0;
};
