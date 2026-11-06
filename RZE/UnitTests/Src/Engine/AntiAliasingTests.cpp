#include <doctest/doctest.h>

#include <Graphics/AntiAliasing.h>

TEST_CASE("Anti-aliasing methods and qualities read back from their names, ignoring case")
{
	for (int method = 0; method < static_cast<int>(EAntiAliasingMethod::Count); ++method)
	{
		const EAntiAliasingMethod value = static_cast<EAntiAliasingMethod>(method);
		CHECK(ParseAntiAliasingMethod(GetAntiAliasingMethodName(value)) == value);
	}
	for (int quality = 0; quality < static_cast<int>(EAntiAliasingQuality::Count); ++quality)
	{
		const EAntiAliasingQuality value = static_cast<EAntiAliasingQuality>(quality);
		CHECK(ParseAntiAliasingQuality(GetAntiAliasingQualityName(value)) == value);
	}

	CHECK(ParseAntiAliasingMethod("fxaa") == EAntiAliasingMethod::FXAA);
	CHECK(ParseAntiAliasingMethod("OFF") == EAntiAliasingMethod::Off);
	CHECK(ParseAntiAliasingQuality("high") == EAntiAliasingQuality::High);

	// The old single setting's levels are qualities now, not methods; Off is a method, not a quality
	CHECK_FALSE(ParseAntiAliasingMethod("Medium").has_value());
	CHECK_FALSE(ParseAntiAliasingQuality("Off").has_value());
	CHECK_FALSE(ParseAntiAliasingMethod("").has_value());
	CHECK_FALSE(ParseAntiAliasingQuality("").has_value());
}

TEST_CASE("Anti-aliasing defaults to FXAA at Medium")
{
	const AntiAliasingMode mode;
	CHECK(mode.Method == EAntiAliasingMethod::FXAA);
	CHECK(mode.Quality == EAntiAliasingQuality::Medium);
}

TEST_CASE("Higher FXAA qualities catch fainter edges, search further in finer steps and soften more")
{
	const FXAASettings low = GetFXAASettings(EAntiAliasingQuality::Low);
	const FXAASettings medium = GetFXAASettings(EAntiAliasingQuality::Medium);
	const FXAASettings high = GetFXAASettings(EAntiAliasingQuality::High);

	CHECK(medium.EdgeThreshold < low.EdgeThreshold);
	CHECK(high.EdgeThreshold < medium.EdgeThreshold);
	CHECK(medium.EdgeThresholdMin < low.EdgeThresholdMin);
	CHECK(high.EdgeThresholdMin < medium.EdgeThresholdMin);

	CHECK(medium.SearchStepCount > low.SearchStepCount);
	CHECK(high.SearchStepCount > medium.SearchStepCount);
	CHECK(medium.GetSearchLength() > low.GetSearchLength());
	CHECK(high.GetSearchLength() > medium.GetSearchLength());
	CHECK(high.SearchStepCount <= FXAASettings::k_maxSearchSteps);

	CHECK(medium.Subpixel > low.Subpixel);
	CHECK(high.Subpixel > medium.Subpixel);
}

TEST_CASE("FXAA's presets search the distances FXAA 3.11 gives them")
{
	// Preset 10 (Low): three coarse steps
	CHECK(GetFXAASettings(EAntiAliasingQuality::Low).GetSearchLength() == doctest::Approx(16.5f));
	// Preset 12 (Medium)
	CHECK(GetFXAASettings(EAntiAliasingQuality::Medium).GetSearchLength() == doctest::Approx(20.5f));
	// Preset 39 (High): twelve, mostly one pixel each
	CHECK(GetFXAASettings(EAntiAliasingQuality::High).GetSearchLength() == doctest::Approx(26.5f));
}
