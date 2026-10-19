#pragma once

#include <chrono>
#include <string>

namespace TimeUtils
{
	// How long ago time was, for a person: "Just now", "5 minutes ago", "3 days ago", or the date once it's months old
	std::string FormatTimeAgo(std::chrono::system_clock::time_point time, std::chrono::system_clock::time_point now);

	// The local time of day, e.g. "15:56:32"
	std::string FormatClockTime(std::chrono::system_clock::time_point time);

	// How long something takes, short: "8s", "1m 05s", "2h 03m"
	std::string FormatDuration(double seconds);
}
