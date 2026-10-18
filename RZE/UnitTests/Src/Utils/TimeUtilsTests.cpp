#include <doctest/doctest.h>

#include <Utils/TimeUtils.h>

#include <chrono>
#include <string>

TEST_CASE("TimeUtils::FormatTimeAgo describes recent times in words and older ones by date")
{
	using namespace std::chrono;
	const system_clock::time_point now = system_clock::now();

	CHECK(TimeUtils::FormatTimeAgo(now - seconds(10), now) == "Just now");
	CHECK(TimeUtils::FormatTimeAgo(now - minutes(1), now) == "1 minute ago");
	CHECK(TimeUtils::FormatTimeAgo(now - minutes(5), now) == "5 minutes ago");
	CHECK(TimeUtils::FormatTimeAgo(now - hours(3), now) == "3 hours ago");
	CHECK(TimeUtils::FormatTimeAgo(now - hours(30), now) == "Yesterday");
	CHECK(TimeUtils::FormatTimeAgo(now - hours(24 * 4), now) == "4 days ago");
	// A date, e.g. "Mar 02, 2026"
	CHECK(TimeUtils::FormatTimeAgo(now - hours(24 * 90), now).find(", ") != std::string::npos);
}
