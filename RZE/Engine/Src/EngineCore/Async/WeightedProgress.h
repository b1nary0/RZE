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
	// For a stage measured in parts of items, e.g. 2.5 of 4 resources uploaded: fraction is in [0, 1]
	void SetStageFraction(U32 stage, float fraction);
	void MarkStageComplete(U32 stage);

	float Compute() const;

private:
	struct Stage
	{
		float Weight = 0.0f;
		// How much of the stage is done, in [0, 1]
		float Fraction = 0.0f;
		bool IsComplete = false;
	};

	std::array<Stage, k_maxStages> m_stages;
	U32 m_stageCount = 0;
};
