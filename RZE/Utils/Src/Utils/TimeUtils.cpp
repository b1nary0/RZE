#include <StdAfx.h>

#include <Utils/TimeUtils.h>

#include <ctime>

namespace
{
	std::string Plural(long long count, const char* unit)
	{
		return std::to_string(count) + " " + unit + (count == 1 ? "" : "s") + " ago";
	}
}

namespace TimeUtils
{
	std::string FormatTimeAgo(std::chrono::system_clock::time_point time, std::chrono::system_clock::time_point now)
	{
		using namespace std::chrono;

		const long long seconds = duration_cast<std::chrono::seconds>(now - time).count();
		if (seconds < 60)
		{
			return "Just now";
		}

		const long long minutes = seconds / 60;
		if (minutes < 60)
		{
			return Plural(minutes, "minute");
		}

		const long long hours = minutes / 60;
		if (hours < 24)
		{
			return Plural(hours, "hour");
		}

		const long long days = hours / 24;
		if (days < 30)
		{
			return days == 1 ? std::string("Yesterday") : Plural(days, "day");
		}

		const std::time_t asTimeT = system_clock::to_time_t(time);
		std::tm local;
		localtime_s(&local, &asTimeT);
		char buffer[32];
		std::strftime(buffer, sizeof(buffer), "%b %d, %Y", &local);
		return buffer;
	}

	std::string FormatClockTime(std::chrono::system_clock::time_point time)
	{
		const std::time_t asTimeT = std::chrono::system_clock::to_time_t(time);
		std::tm local;
		localtime_s(&local, &asTimeT);
		char buffer[16];
		std::strftime(buffer, sizeof(buffer), "%H:%M:%S", &local);
		return buffer;
	}
}
