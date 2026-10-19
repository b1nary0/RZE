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

	CHECK(StringUtils::EndsWithIgnoringCase("Sponza.SCENE", ".scene"));
	CHECK(StringUtils::EndsWithIgnoringCase("Sponza", ""));
	CHECK_FALSE(StringUtils::EndsWithIgnoringCase("scene", "Sponza.scene"));
}

TEST_CASE("StringUtils trims spaces and tabs from both ends only")
{
	CHECK(StringUtils::Trim("  Level 2 \t") == "Level 2");
	CHECK(StringUtils::Trim("Level 2") == "Level 2");
	CHECK(StringUtils::Trim(" \t ").empty());
	CHECK(StringUtils::Trim("").empty());
}

TEST_CASE("StringUtils finds a part anywhere in text, ignoring case")
{
	CHECK(StringUtils::ContainsIgnoringCase("ceiling_plaster_02_BaseColor.png", "basecolor"));
	CHECK(StringUtils::ContainsIgnoringCase("Loading thumbnails", "LOADING"));
	CHECK(StringUtils::ContainsIgnoringCase("lion.png", ""));
	CHECK_FALSE(StringUtils::ContainsIgnoringCase("lion.png", "lion.jpg"));
	CHECK_FALSE(StringUtils::ContainsIgnoringCase("", "lion"));
}
