#pragma once

#include <chrono>
#include <string>

namespace TimeUtils
{
	// How long ago time was, for a person: "Just now", "5 minutes ago", "3 days ago", or the date once it's months old
	std::string FormatTimeAgo(std::chrono::system_clock::time_point time, std::chrono::system_clock::time_point now);
}
