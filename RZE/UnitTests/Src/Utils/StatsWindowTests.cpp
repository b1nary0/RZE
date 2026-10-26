#include <doctest/doctest.h>

#include <Utils/StatsWindow.h>

namespace
{
	// Adds up exactly in binary, and k_windowSeconds holds a whole number of them
	constexpr double k_frameTime = 1.0 / 64.0;
	const int k_framesPerWindow = static_cast<int>(StatsWindow::k_windowSeconds / k_frameTime);
}

TEST_CASE("The first frame boundary only starts the window")
{
	StatsWindow window;
	CHECK_FALSE(window.HasStarted());

	CHECK_FALSE(window.AddFrame(1.0));
	CHECK(window.HasStarted());
	CHECK(window.GetFrameCount() == 0);
	CHECK(window.PerFrame(1.0) == 0.0);
	CHECK(window.PerSecond(1.0) == 0.0);
}

TEST_CASE("A window closes once it spans k_windowSeconds, and not before")
{
	StatsWindow window;
	double now = 0.0;
	window.AddFrame(now);
	for (int frame = 1; frame < k_framesPerWindow; ++frame)
	{
		now += k_frameTime;
		CHECK_FALSE(window.AddFrame(now));
	}

	now += k_frameTime;
	CHECK(window.AddFrame(now));
	CHECK(window.GetFrameCount() == k_framesPerWindow);
	CHECK(window.GetSeconds() == StatsWindow::k_windowSeconds);
	CHECK(window.PerFrame(window.GetSeconds()) == k_frameTime);
	CHECK(window.PerSecond(window.GetFrameCount()) == 64.0);
}

TEST_CASE("The next window starts where the last one closed, and the averages hold until it closes")
{
	StatsWindow window;
	window.AddFrame(0.0);
	// One long frame closes the first window by itself
	CHECK(window.AddFrame(0.75));
	CHECK(window.GetFrameCount() == 1);
	CHECK(window.GetSeconds() == 0.75);

	CHECK_FALSE(window.AddFrame(1.0));
	CHECK(window.GetSeconds() == 0.75);

	CHECK(window.AddFrame(1.25));
	CHECK(window.GetFrameCount() == 2);
	CHECK(window.GetSeconds() == 0.5);
}
