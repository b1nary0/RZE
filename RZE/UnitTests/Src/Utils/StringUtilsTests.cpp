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

TEST_CASE("StringUtils formats counts to 3 significant figures, with K, M, G and T")
{
	CHECK(StringUtils::FormatCount(0.0) == "0");
	CHECK(StringUtils::FormatCount(6.0) == "6");
	CHECK(StringUtils::FormatCount(48.6) == "49");
	CHECK(StringUtils::FormatCount(999.0) == "999");
	CHECK(StringUtils::FormatCount(999.4) == "999");
	// What would round to 1000 moves up a unit
	CHECK(StringUtils::FormatCount(999.5) == "1.00K");
	CHECK(StringUtils::FormatCount(1234.0) == "1.23K");
	CHECK(StringUtils::FormatCount(9994.0) == "9.99K");
	CHECK(StringUtils::FormatCount(9996.0) == "10.0K");
	CHECK(StringUtils::FormatCount(12345.0) == "12.3K");
	CHECK(StringUtils::FormatCount(99960.0) == "100K");
	CHECK(StringUtils::FormatCount(361805.0) == "362K");
	CHECK(StringUtils::FormatCount(999400.0) == "999K");
	CHECK(StringUtils::FormatCount(999950.0) == "1.00M");
	CHECK(StringUtils::FormatCount(13826522.0) == "13.8M");
	CHECK(StringUtils::FormatCount(2.5e9) == "2.50G");
	CHECK(StringUtils::FormatCount(4.2e12) == "4.20T");
	// Past T it stays in T
	CHECK(StringUtils::FormatCount(5.0e15) == "5000T");
	CHECK(StringUtils::FormatCount(-1234.0) == "-1.23K");
}

TEST_CASE("StringUtils takes the file name from a path with either separator")
{
	CHECK(StringUtils::ExtractFileNameFromPath("Assets\\Scenes\\Sponza.scene") == "Sponza.scene");
	CHECK(StringUtils::ExtractFileNameFromPath("Assets/Scenes/Sponza.scene") == "Sponza.scene");
	CHECK(StringUtils::ExtractFileNameFromPath("Sponza.scene") == "Sponza.scene");
	CHECK(StringUtils::ExtractFileNameFromPath("Assets/").empty());
}

TEST_CASE("StringUtils copies into a buffer, cut short and terminated")
{
	char buffer[5];
	StringUtils::CopyTruncated(buffer, "Sponza");
	CHECK(std::string(buffer) == "Spon");
	StringUtils::CopyTruncated(buffer, "M4");
	CHECK(std::string(buffer) == "M4");
}
