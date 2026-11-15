#include <Rendering/DrawCounts.h>

// Render stats only (its header is empty without them)
#ifdef RZE_RENDER_STATS

namespace Rendering
{
	template <typename T>
	GPUStateChange<T>& GPUStateChange<T>::operator+=(const GPUStateChange& other)
	{
		Binds += other.Binds;
		Repeats += other.Repeats;
		return *this;
	}

	template <typename T>
	GPUStateChange<T> GPUStateChange<T>::operator-(const GPUStateChange& earlier) const
	{
		GPUStateChange since;
		since.Binds = Binds - earlier.Binds;
		since.Repeats = Repeats - earlier.Repeats;
		return since;
	}

	template struct GPUStateChange<U32>;
	template struct GPUStateChange<double>;

	GPUStateChangeAverages ToAverages(const GPUStateChangeCounts& counts)
	{
		GPUStateChangeAverages averages;
		for (U32 category = 0; category < k_stateCategoryCount; ++category)
		{
			averages[category].Binds = static_cast<double>(counts[category].Binds);
			averages[category].Repeats = static_cast<double>(counts[category].Repeats);
		}
		return averages;
	}

	GPUStateChangeAverages ToPerFrame(const GPUStateChangeAverages& totals, int frameCount)
	{
		GPUStateChangeAverages perFrame;
		if (frameCount <= 0)
		{
			return perFrame;
		}

		const double frames = static_cast<double>(frameCount);
		for (U32 category = 0; category < k_stateCategoryCount; ++category)
		{
			perFrame[category].Binds = totals[category].Binds / frames;
			perFrame[category].Repeats = totals[category].Repeats / frames;
		}
		return perFrame;
	}

	StateChangeCounts& StateChangeCounts::operator+=(const StateChangeCounts& other)
	{
		for (U32 category = 0; category < k_stateCategoryCount; ++category)
		{
			ByCategory[category] += other.ByCategory[category];
		}
		return *this;
	}

	StateChangeCounts StateChangeCounts::operator-(const StateChangeCounts& earlier) const
	{
		StateChangeCounts since;
		for (U32 category = 0; category < k_stateCategoryCount; ++category)
		{
			since.ByCategory[category] = ByCategory[category] - earlier.ByCategory[category];
		}
		return since;
	}

	DrawCounts& DrawCounts::operator+=(const DrawCounts& other)
	{
		DrawCalls += other.DrawCalls;
		Instances += other.Instances;
		StateChanges += other.StateChanges;
		return *this;
	}

	DrawCounts DrawCounts::operator-(const DrawCounts& earlier) const
	{
		DrawCounts since;
		since.DrawCalls = DrawCalls - earlier.DrawCalls;
		since.Instances = Instances - earlier.Instances;
		since.StateChanges = StateChanges - earlier.StateChanges;
		return since;
	}
}

#endif // RZE_RENDER_STATS
