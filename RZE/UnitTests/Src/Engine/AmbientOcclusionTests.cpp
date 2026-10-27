#include <doctest/doctest.h>

#include <Graphics/AmbientOcclusion.h>

TEST_CASE("Ambient occlusion levels read back from their names, ignoring case")
{
	for (int level = 0; level < static_cast<int>(EAmbientOcclusion::Count); ++level)
	{
		const EAmbientOcclusion ambientOcclusion = static_cast<EAmbientOcclusion>(level);
		CHECK(ParseAmbientOcclusion(GetAmbientOcclusionName(ambientOcclusion)) == ambientOcclusion);
	}

	CHECK(ParseAmbientOcclusion("medium") == EAmbientOcclusion::Medium);
	CHECK(ParseAmbientOcclusion("OFF") == EAmbientOcclusion::Off);
	CHECK_FALSE(ParseAmbientOcclusion("Ultra").has_value());
	CHECK_FALSE(ParseAmbientOcclusion("").has_value());
}

TEST_CASE("Higher ambient occlusion levels search more of the hemisphere and Off does nothing")
{
	const AmbientOcclusionSettings low = GetAmbientOcclusionSettings(EAmbientOcclusion::Low);
	const AmbientOcclusionSettings medium = GetAmbientOcclusionSettings(EAmbientOcclusion::Medium);
	const AmbientOcclusionSettings high = GetAmbientOcclusionSettings(EAmbientOcclusion::High);

	CHECK(low.SliceCount * low.StepsPerSlice < medium.SliceCount * medium.StepsPerSlice);
	CHECK(medium.SliceCount * medium.StepsPerSlice < high.SliceCount * high.StepsPerSlice);
	CHECK(low.DenoisePasses >= 1);
	CHECK(medium.DenoisePasses >= low.DenoisePasses);
	CHECK(high.DenoisePasses >= medium.DenoisePasses);

	const AmbientOcclusionSettings off = GetAmbientOcclusionSettings(EAmbientOcclusion::Off);
	CHECK(off.SliceCount == 0);
	CHECK(off.StepsPerSlice == 0);
	CHECK(off.DenoisePasses == 0);
}
