#include <doctest/doctest.h>

#include <Utils/TimeUtils.h>

#include <chrono>
#include <ctime>
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

TEST_CASE("TimeUtils::FormatClockTime gives the local time of day")
{
	std::tm local = {};
	local.tm_year = 2026 - 1900;
	local.tm_mon = 9;
	local.tm_mday = 19;
	local.tm_hour = 15;
	local.tm_min = 6;
	local.tm_sec = 2;
	local.tm_isdst = -1;
	const std::chrono::system_clock::time_point time = std::chrono::system_clock::from_time_t(std::mktime(&local));

	CHECK(TimeUtils::FormatClockTime(time) == "15:06:02");
}

TEST_CASE("TimeUtils::FormatDuration is short")
{
	CHECK(TimeUtils::FormatDuration(0.2) == "0s");
	CHECK(TimeUtils::FormatDuration(8.4) == "8s");
	CHECK(TimeUtils::FormatDuration(59.4) == "59s");
	CHECK(TimeUtils::FormatDuration(65.0) == "1m 05s");
	CHECK(TimeUtils::FormatDuration(3599.0) == "59m 59s");
	CHECK(TimeUtils::FormatDuration(3600.0 * 2 + 180.0) == "2h 03m");
}
