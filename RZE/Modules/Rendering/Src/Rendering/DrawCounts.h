#pragma once

#include <Utils/DebugUtils/DebugConfig.h>

// Render stats only
#ifdef RZE_RENDER_STATS

#include <Utils/PrimitiveDefs.h>

namespace Rendering
{
	// The draws Renderer recorded (Renderer::TakeDrawCounts), counted on the main thread as they're recorded. What the GPU
	// did with them, such as their triangles, is GPUPipelineStatistics', counted by the GPU.
	struct DrawCounts
	{
		U32 DrawCalls = 0;
		// Each draw's instances: its instance count if it's instanced, otherwise 1
		U32 Instances = 0;

		DrawCounts& operator+=(const DrawCounts& other);
		// The draws recorded since earlier, an older reading of the same counts
		DrawCounts operator-(const DrawCounts& earlier) const;
	};
}

#endif // RZE_RENDER_STATS
