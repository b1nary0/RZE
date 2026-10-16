#include <doctest/doctest.h>

#include <EngineCore/Timing/FramePacer.h>

namespace
{
	FrameSettings MakeSettings(double maxFrameRate, double unfocusedFrameRate = 0.0, bool vsync = false)
	{
		FrameSettings settings;
		settings.VSync = vsync;
		settings.MaxFrameRate = maxFrameRate;
		settings.UnfocusedFrameRate = unfocusedFrameRate;
		return settings;
	}
}

TEST_CASE("A capped frame is scheduled a period after the last one was due, so late wake-ups don't add up")
{
	FramePacer pacer;
	pacer.SetSettings(MakeSettings(100.0));

	pacer.BeginFrame(0.0);
	CHECK(pacer.GetNextFrameStart() == doctest::Approx(0.01));

	// Woke 2 ms late each time; the schedule stays on the 10 ms grid
	pacer.BeginFrame(0.012);
	CHECK(pacer.GetNextFrameStart() == doctest::Approx(0.02));
	pacer.BeginFrame(0.022);
	CHECK(pacer.GetNextFrameStart() == doctest::Approx(0.03));
}

TEST_CASE("A frame that starts over a period late re-bases the schedule instead of rushing the frames after it")
{
	FramePacer pacer;
	pacer.SetSettings(MakeSettings(100.0));

	pacer.BeginFrame(0.0);
	pacer.BeginFrame(0.5);
	CHECK(pacer.GetNextFrameStart() == doctest::Approx(0.51));
}

TEST_CASE("Delta time is clamped, so a long stall doesn't make the scene jump")
{
	FramePacer pacer;

	pacer.BeginFrame(0.0);
	pacer.BeginFrame(2.0);
	CHECK(pacer.GetFrameTime() == doctest::Approx(2.0));
	CHECK(pacer.GetDeltaTime() == doctest::Approx(FramePacer::k_maxDeltaTime));
}

TEST_CASE("Uncapped frames simulate the time measured, and have no spare time")
{
	FramePacer pacer;

	pacer.BeginFrame(0.0);
	pacer.BeginFrame(0.004);
	CHECK(pacer.GetDeltaTime() == doctest::Approx(0.004));
	CHECK(pacer.GetIdleTime(0.005) == 0.0);
	CHECK(pacer.GetNextFrameStart() <= 0.004);
}

TEST_CASE("On-time frames simulate the measured period rather than each frame's jitter")
{
	FramePacer pacer;
	pacer.SetSettings(MakeSettings(100.0));

	double now = 0.0;
	pacer.BeginFrame(now);
	for (int frame = 0; frame < 400; ++frame)
	{
		// Alternately 0.5 ms early and late around a 10.2 ms cadence
		now += (frame % 2 == 0) ? 0.0097 : 0.0107;
		pacer.BeginFrame(now);
	}

	// Close to the true average cadence, and steady from frame to frame
	const double settled = pacer.GetDeltaTime();
	CHECK(settled == doctest::Approx(0.0102).epsilon(0.02));

	now += 0.0097;
	pacer.BeginFrame(now);
	CHECK(pacer.GetDeltaTime() == doctest::Approx(settled).epsilon(0.01));
}

TEST_CASE("A frame well off the target period simulates its measured time")
{
	FramePacer pacer;
	pacer.SetSettings(MakeSettings(100.0));

	pacer.BeginFrame(0.0);
	pacer.BeginFrame(0.01);
	pacer.BeginFrame(0.035);
	CHECK(pacer.GetDeltaTime() == doctest::Approx(0.025));
}

TEST_CASE("An unfocused window runs at the unfocused frame rate, or the cap if that's lower")
{
	FramePacer pacer;
	pacer.SetSettings(MakeSettings(0.0, 20.0));
	pacer.BeginFrame(0.0);
	CHECK(pacer.GetNextFrameStart() == 0.0);

	// Each scheduled a period after the previous frame was due (0, then 0.05)
	pacer.SetIsThrottled(true);
	pacer.BeginFrame(0.01);
	CHECK(pacer.GetNextFrameStart() == doctest::Approx(0.05));

	pacer.SetSettings(MakeSettings(10.0, 20.0));
	pacer.BeginFrame(0.05);
	CHECK(pacer.GetNextFrameStart() == doctest::Approx(0.15));
}

TEST_CASE("Spare time runs to the next frame's start, less the safety margin")
{
	FramePacer pacer;
	pacer.SetSettings(MakeSettings(100.0));

	pacer.BeginFrame(0.0);
	CHECK(pacer.GetIdleTime(0.004) == doctest::Approx(0.006 - FramePacer::k_idleSafetyMargin));
	CHECK(pacer.GetIdleTime(0.0095) == 0.0);
}

TEST_CASE("With vsync and a known refresh rate, spare time runs to a refresh period after the frame started")
{
	FramePacer pacer;
	pacer.SetSettings(MakeSettings(0.0, 0.0, true));
	pacer.SetRefreshRate(50.0);

	pacer.BeginFrame(1.0);
	CHECK(pacer.GetIdleTime(1.005) == doctest::Approx(0.015 - FramePacer::k_idleSafetyMargin));
	// Vsync does the waiting, not the CPU cap
	CHECK(pacer.GetNextFrameStart() <= 1.0);
}

TEST_CASE("Stats average over a window of frames, and only change when a window ends")
{
	FramePacer pacer;
	pacer.SetSettings(MakeSettings(64.0));

	// Durations that add up exactly in binary: 1/64 s frames, each idle for 1/256 s of which 1/1024 s is work
	constexpr double k_frameTime = 1.0 / 64.0;
	constexpr double k_idleTime = 1.0 / 256.0;
	constexpr double k_idleWork = 1.0 / 1024.0;
	const int framesPerWindow = static_cast<int>(FramePacer::k_statsWindow / k_frameTime);

	double now = 0.0;
	pacer.BeginFrame(now);
	for (int frame = 0; frame < framesPerWindow; ++frame)
	{
		CHECK(pacer.GetStats().FramesPerSecond == 0.0);

		pacer.EndFrameWork(now + k_frameTime - k_idleTime);
		pacer.AddIdleWork(k_idleWork);
		now += k_frameTime;
		pacer.BeginFrame(now);
	}

	const FramePacer::Stats& stats = pacer.GetStats();
	CHECK(stats.FrameTimeMS == doctest::Approx(k_frameTime * 1000.0));
	CHECK(stats.FramesPerSecond == doctest::Approx(64.0));
	CHECK(stats.IdleTimeMS == doctest::Approx(k_idleTime * 1000.0));
	CHECK(stats.IdleWorkMS == doctest::Approx(k_idleWork * 1000.0));
}
