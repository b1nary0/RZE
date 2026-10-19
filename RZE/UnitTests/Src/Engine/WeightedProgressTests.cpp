#include <doctest/doctest.h>

#include <EngineCore/Async/WeightedProgress.h>

TEST_CASE("Weighted progress adds up its stages by weight, counted or as fractions")
{
	WeightedProgress progress;
	const U32 counted = progress.AddStage(0.6f);
	const U32 fractional = progress.AddStage(0.4f);
	CHECK(progress.Compute() == doctest::Approx(0.0f));

	progress.SetStage(counted, 1, 2);
	CHECK(progress.Compute() == doctest::Approx(0.3f));

	// e.g. 2.5 of 4 resources uploaded
	progress.SetStageFraction(fractional, 2.5f / 4.0f);
	CHECK(progress.Compute() == doctest::Approx(0.3f + 0.25f));

	// Clamped to a whole stage
	progress.SetStageFraction(fractional, 2.0f);
	progress.MarkStageComplete(counted);
	CHECK(progress.Compute() == doctest::Approx(1.0f));
}
