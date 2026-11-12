#include <Rendering/DrawCounts.h>

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
