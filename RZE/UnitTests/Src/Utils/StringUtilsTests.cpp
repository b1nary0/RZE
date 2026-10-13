#include <doctest/doctest.h>

#include <Utils/StringUtils.h>

TEST_CASE("StringUtils changes case")
{
	CHECK(StringUtils::ToLower("Sponza.SCENE") == "sponza.scene");
	CHECK(StringUtils::ToUpper("scene 2") == "SCENE 2");
}

TEST_CASE("StringUtils compares ignoring case")
{
	CHECK(StringUtils::EqualsIgnoringCase("Assets", "aSSETS"));
	CHECK_FALSE(StringUtils::EqualsIgnoringCase("Assets", "Asset"));

	CHECK(StringUtils::StartsWithIgnoringCase("Assets/Scenes", "assets/"));
	CHECK(StringUtils::StartsWithIgnoringCase("Assets", ""));
	CHECK_FALSE(StringUtils::StartsWithIgnoringCase("Assets", "Assets/Scenes"));
}
