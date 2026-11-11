#include <doctest/doctest.h>

#include <Utils/DebugUtils/Debug.h>

#include <string>
#include <string_view>
#include <vector>

namespace
{
	std::vector<std::string> g_timedLines;

	void CollectTimedLines(Debug::ELogLevel level, std::string_view line)
	{
		if (level == Debug::ELogLevel::Debug && line.find(" took ") != std::string_view::npos)
		{
			g_timedLines.emplace_back(line);
		}
	}
}

TEST_CASE("START_TIMED_BLOCK logs each block's time when its scope ends, wherever logging is on")
{
	g_timedLines.clear();
	Debug::SetLogSink(&CollectTimedLines);
	{
		const char* const secondName = "Second block";
		START_TIMED_BLOCK("First block");
		START_TIMED_BLOCK(secondName);
	}
	Debug::SetLogSink(nullptr);

#ifdef RZE_LOGGING_ENABLED
	// Destroyed in reverse order, so the second block reports first
	REQUIRE(g_timedLines.size() == 2);
	CHECK(g_timedLines[0].rfind("Second block took ", 0) == 0);
	CHECK(g_timedLines[1].rfind("First block took ", 0) == 0);
#else
	CHECK(g_timedLines.empty());
#endif
}
