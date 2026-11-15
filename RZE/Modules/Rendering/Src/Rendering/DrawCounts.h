#pragma once

#include <Utils/DebugUtils/DebugConfig.h>

// Render stats only
#ifdef RZE_RENDER_STATS

#include <Rendering/StateTracker.h>

#include <Utils/PrimitiveDefs.h>

#include <array>

namespace Rendering
{
	// The binds of one EStateCategory, and how many of them repeated what the slot already had (StateTracker): the binds
	// that did nothing. Repeats are part of Binds. U32 as Renderer counts them, double where they're averaged per frame
	// (RenderStageStats).
	template <typename T>
	struct GPUStateChange
	{
		T Binds = 0;
		T Repeats = 0;

		GPUStateChange& operator+=(const GPUStateChange& other);
		GPUStateChange operator-(const GPUStateChange& earlier) const;
	};

	// What Renderer counted, as whole binds
	using GPUStateChangeCount = GPUStateChange<U32>;
	// Counts averaged over frames (RenderStageStats), so not whole
	using GPUStateChangeAverage = GPUStateChange<double>;

	// Every category's, indexed by EStateCategory
	using GPUStateChangeCounts = std::array<GPUStateChangeCount, k_stateCategoryCount>;
	using GPUStateChangeAverages = std::array<GPUStateChangeAverage, k_stateCategoryCount>;

	// The counts as averages, the same numbers, e.g. to add to a stage's other figures
	GPUStateChangeAverages ToAverages(const GPUStateChangeCounts& counts);
	// Totals over frameCount frames as what each frame had: 0 for none, as StatsWindow::PerFrame
	GPUStateChangeAverages ToPerFrame(const GPUStateChangeAverages& totals, int frameCount);

	// What Renderer recorded in each category
	struct StateChangeCounts
	{
		GPUStateChangeCounts ByCategory = {};

		StateChangeCounts& operator+=(const StateChangeCounts& other);
		StateChangeCounts operator-(const StateChangeCounts& earlier) const;
	};

	// The draws Renderer recorded (Renderer::TakeDrawCounts), counted on the main thread as they're recorded. What the GPU
	// did with them, such as their triangles, is GPUPipelineStatistics', counted by the GPU.
	struct DrawCounts
	{
		U32 DrawCalls = 0;
		// Each draw's instances: its instance count if it's instanced, otherwise 1
		U32 Instances = 0;
		StateChangeCounts StateChanges;

		DrawCounts& operator+=(const DrawCounts& other);
		// The draws recorded since earlier, an older reading of the same counts
		DrawCounts operator-(const DrawCounts& earlier) const;
	};
}

#endif // RZE_RENDER_STATS
