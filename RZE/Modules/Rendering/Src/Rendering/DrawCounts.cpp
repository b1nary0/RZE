#include <Rendering/DrawCounts.h>

// Render stats only (its header is empty without them)
#ifdef RZE_RENDER_STATS

namespace Rendering
{
	DrawCounts& DrawCounts::operator+=(const DrawCounts& other)
	{
		DrawCalls += other.DrawCalls;
		Instances += other.Instances;
		return *this;
	}

	DrawCounts DrawCounts::operator-(const DrawCounts& earlier) const
	{
		DrawCounts since;
		since.DrawCalls = DrawCalls - earlier.DrawCalls;
		since.Instances = Instances - earlier.Instances;
		return since;
	}
}

#endif // RZE_RENDER_STATS
