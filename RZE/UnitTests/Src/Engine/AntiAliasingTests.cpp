#include <doctest/doctest.h>

#include <Graphics/AntiAliasing.h>

TEST_CASE("Anti-aliasing levels read back from their names, ignoring case")
{
	for (int level = 0; level < static_cast<int>(EAntiAliasing::Count); ++level)
	{
		const EAntiAliasing antiAliasing = static_cast<EAntiAliasing>(level);
		CHECK(ParseAntiAliasing(GetAntiAliasingName(antiAliasing)) == antiAliasing);
	}

	CHECK(ParseAntiAliasing("high") == EAntiAliasing::High);
	CHECK(ParseAntiAliasing("OFF") == EAntiAliasing::Off);
	CHECK_FALSE(ParseAntiAliasing("Ultra").has_value());
	CHECK_FALSE(ParseAntiAliasing("").has_value());
}

TEST_CASE("Higher anti-aliasing levels catch fainter edges, search further in finer steps and soften more, and Off does nothing")
{
	const AntiAliasingSettings low = GetAntiAliasingSettings(EAntiAliasing::Low);
	const AntiAliasingSettings medium = GetAntiAliasingSettings(EAntiAliasing::Medium);
	const AntiAliasingSettings high = GetAntiAliasingSettings(EAntiAliasing::High);

	CHECK(medium.EdgeThreshold < low.EdgeThreshold);
	CHECK(high.EdgeThreshold < medium.EdgeThreshold);
	CHECK(medium.EdgeThresholdMin < low.EdgeThresholdMin);
	CHECK(high.EdgeThresholdMin < medium.EdgeThresholdMin);

	CHECK(medium.SearchStepCount > low.SearchStepCount);
	CHECK(high.SearchStepCount > medium.SearchStepCount);
	CHECK(medium.GetSearchLength() > low.GetSearchLength());
	CHECK(high.GetSearchLength() > medium.GetSearchLength());
	CHECK(high.SearchStepCount <= AntiAliasingSettings::k_maxSearchSteps);

	CHECK(medium.Subpixel > low.Subpixel);
	CHECK(high.Subpixel > medium.Subpixel);

	CHECK(GetAntiAliasingSettings(EAntiAliasing::Off).SearchStepCount == 0);
}

TEST_CASE("FXAA's presets search the distances FXAA 3.11 gives them")
{
	// Preset 10 (Low): three coarse steps
	CHECK(GetAntiAliasingSettings(EAntiAliasing::Low).GetSearchLength() == doctest::Approx(16.5f));
	// Preset 12 (Medium)
	CHECK(GetAntiAliasingSettings(EAntiAliasing::Medium).GetSearchLength() == doctest::Approx(20.5f));
	// Preset 39 (High): twelve, mostly one pixel each
	CHECK(GetAntiAliasingSettings(EAntiAliasing::High).GetSearchLength() == doctest::Approx(26.5f));
}
