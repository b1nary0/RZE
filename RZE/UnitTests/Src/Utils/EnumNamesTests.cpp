#include <doctest/doctest.h>

#include <Utils/EnumNames.h>

namespace
{
	constexpr const char* k_levels[] = { "Off", "Low", "Medium", "High" };
	constexpr const char* k_pair[] = { "On", "Off" };
	constexpr const char* k_single[] = { "Only" };
}

TEST_CASE("EnumNames names each value by its index, and nothing outside the table")
{
	const EnumNames names(k_levels);
	CHECK(names.GetCount() == 4);
	CHECK(std::string(names.GetName(0)) == "Off");
	CHECK(std::string(names.GetName(3)) == "High");
	CHECK(std::string(names.GetName(4)).empty());
}

TEST_CASE("EnumNames finds a value by its name, ignoring case")
{
	const EnumNames names(k_levels);
	CHECK(names.FindIgnoringCase("Medium") == 2u);
	CHECK(names.FindIgnoringCase("hIGH") == 3u);
	CHECK(names.FindIgnoringCase("OFF") == 0u);
	CHECK_FALSE(names.FindIgnoringCase("Ultra").has_value());
	CHECK_FALSE(names.FindIgnoringCase("Med").has_value());
	CHECK_FALSE(names.FindIgnoringCase("").has_value());
}

TEST_CASE("EnumNames lists its names for a message")
{
	CHECK(EnumNames(k_levels).ListAlternatives() == "Off, Low, Medium or High");
	CHECK(EnumNames(k_pair).ListAlternatives() == "On or Off");
	CHECK(EnumNames(k_single).ListAlternatives() == "Only");
}
