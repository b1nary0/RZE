#pragma once

#include <array>

#include <Utils/PrimitiveDefs.h>

// Aggregates progress across several stages of differing cost into a single [0, 1] value.
// Stage totals may grow as work is discovered; pair with AsyncOperation::SetProgress (which is
// monotonic) so the reported value never moves backwards.
class WeightedProgress
{
public:
	static constexpr U32 k_maxStages = 8;

public:
	// Returns the stage index to use with the other functions.
	U32 AddStage(float weight);

	void SetStage(U32 stage, U32 completed, U32 total);
	void MarkStageComplete(U32 stage);

	float Compute() const;

private:
	struct Stage
	{
		float Weight = 0.0f;
		U32 Completed = 0;
		U32 Total = 0;
		bool IsComplete = false;
	};

	std::array<Stage, k_maxStages> m_stages;
	U32 m_stageCount = 0;
};
