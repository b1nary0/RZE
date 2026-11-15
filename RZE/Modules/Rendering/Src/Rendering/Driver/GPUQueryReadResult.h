#pragma once

#include <Utils/DebugUtils/DebugConfig.h>

// Render stats only
#ifdef RZE_RENDER_STATS

namespace Rendering
{
	// What reading back one frame of GPU queries found (IGPUTimestamps::Read, IGPUStatistics::Read)
	enum class EGPUQueryReadResult
	{
		// The GPU hasn't passed every query of the frame yet: ask again later
		NotReady,
		Ready,
		// The GPU is done, but the results can't be trusted or read (D3D11's clock changed frequency during the frame, or
		// a query couldn't be read)
		Invalid
	};
}

#endif // RZE_RENDER_STATS
