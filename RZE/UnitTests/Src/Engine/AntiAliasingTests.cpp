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
	CHECK(ParseAntiAliasingMethod("smaa") == EAntiAliasingMethod::SMAA);
	CHECK(ParseAntiAliasingQuality("ULTRA") == EAntiAliasingQuality::Ultra);

	// The old single setting's levels are qualities now, not methods; Off is a method, not a quality
	CHECK_FALSE(ParseAntiAliasingMethod("Medium").has_value());
	CHECK_FALSE(ParseAntiAliasingQuality("Off").has_value());
	CHECK_FALSE(ParseAntiAliasingMethod("").has_value());
	CHECK_FALSE(ParseAntiAliasingQuality("").has_value());
}

TEST_CASE("Anti-aliasing defaults to SMAA at High")
{
	const AntiAliasingMode mode;
	CHECK(mode.Method == EAntiAliasingMethod::SMAA);
	CHECK(mode.Quality == EAntiAliasingQuality::High);
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

TEST_CASE("FXAA has no Ultra: it uses High's settings, and its highest quality is High")
{
	const FXAASettings high = GetFXAASettings(EAntiAliasingQuality::High);
	const FXAASettings ultra = GetFXAASettings(EAntiAliasingQuality::Ultra);
	CHECK(ultra.EdgeThreshold == high.EdgeThreshold);
	CHECK(ultra.EdgeThresholdMin == high.EdgeThresholdMin);
	CHECK(ultra.Subpixel == high.Subpixel);
	CHECK(ultra.SearchStepCount == high.SearchStepCount);
	CHECK(ultra.SearchSteps == high.SearchSteps);

	CHECK(GetHighestQuality(EAntiAliasingMethod::FXAA) == EAntiAliasingQuality::High);
	CHECK(GetHighestQuality(EAntiAliasingMethod::SMAA) == EAntiAliasingQuality::Ultra);
}

TEST_CASE("SMAA's qualities are SMAA.hlsl's presets")
{
	const SMAASettings low = GetSMAASettings(EAntiAliasingQuality::Low);
	CHECK(low.Threshold == doctest::Approx(0.15f));
	CHECK(low.SearchSteps == 4);
	CHECK_FALSE(low.DetectsDiagonalsAndCorners);

	const SMAASettings medium = GetSMAASettings(EAntiAliasingQuality::Medium);
	CHECK(medium.Threshold == doctest::Approx(0.1f));
	CHECK(medium.SearchSteps == 8);
	CHECK_FALSE(medium.DetectsDiagonalsAndCorners);

	const SMAASettings high = GetSMAASettings(EAntiAliasingQuality::High);
	CHECK(high.Threshold == doctest::Approx(0.1f));
	CHECK(high.SearchSteps == 16);
	CHECK(high.DetectsDiagonalsAndCorners);
	CHECK(high.DiagonalSearchSteps == 8);
	CHECK(high.CornerRounding == doctest::Approx(25.0f));

	const SMAASettings ultra = GetSMAASettings(EAntiAliasingQuality::Ultra);
	CHECK(ultra.Threshold == doctest::Approx(0.05f));
	CHECK(ultra.SearchSteps == 32);
	CHECK(ultra.DetectsDiagonalsAndCorners);
	CHECK(ultra.DiagonalSearchSteps == 16);
	CHECK(ultra.CornerRounding == doctest::Approx(25.0f));
}
