#include <doctest/doctest.h>

#include <EngineCore/Timing/FramePacer.h>

#include <vector>

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

namespace
{
	// 60 frames a second with the UI at uiFrameRate, from a first frame at time 0. Returns which of the next frameCount
	// frames updated the UI, each with up to 0.5 ms of jitter.
	std::vector<bool> RunUIFrames(FramePacer& pacer, double uiFrameRate, int frameCount)
	{
		FrameSettings settings = MakeSettings(60.0);
		settings.UIFrameRate = uiFrameRate;
		pacer.SetSettings(settings);

		pacer.BeginFrame(0.0);
		CHECK(pacer.TakeUIFrame(0.0));

		std::vector<bool> uiFrames;
		for (int frame = 1; frame <= frameCount; ++frame)
		{
			const double jitter = (frame % 3 - 1) * 0.0005;
			const double now = frame / 60.0 + jitter;
			pacer.BeginFrame(now);
			uiFrames.push_back(pacer.TakeUIFrame(now));
		}
		return uiFrames;
	}
}

TEST_CASE("With no UI frame rate, every frame updates the UI")
{
	FramePacer pacer;
	for (const bool isUIFrame : RunUIFrames(pacer, 0.0, 10))
	{
		CHECK(isUIFrame);
	}
}

TEST_CASE("A UI rate that divides the frame rate gets an even cadence, despite jitter")
{
	FramePacer pacer;
	const std::vector<bool> uiFrames = RunUIFrames(pacer, 30.0, 60);
	for (size_t frame = 0; frame < uiFrames.size(); ++frame)
	{
		// The first frame was one, so every second frame after it
		CHECK(uiFrames[frame] == (frame % 2 == 1));
	}
}

TEST_CASE("A requested UI frame comes at once, and the UI's cadence carries on from it")
{
	FramePacer pacer;
	RunUIFrames(pacer, 10.0, 2);

	// Between UI frames, which are 6 frames apart at 60 and 10
	pacer.RequestUIFrame();
	pacer.BeginFrame(3.0 / 60.0);
	CHECK(pacer.TakeUIFrame(3.0 / 60.0));

	int framesToNext = 0;
	for (int frame = 4; frame < 20; ++frame)
	{
		pacer.BeginFrame(frame / 60.0);
		if (pacer.TakeUIFrame(frame / 60.0))
		{
			framesToNext = frame - 3;
			break;
		}
	}
	CHECK(framesToNext == 6);
}

TEST_CASE("While the user interacts every frame updates the UI, and after a short hold the UI's cadence resumes")
{
	FramePacer pacer;
	FrameSettings settings = MakeSettings(60.0);
	settings.UIFrameRate = 10.0;
	pacer.SetSettings(settings);

	const auto frameAt = [&pacer](int frame)
	{
		const double now = frame / 60.0;
		pacer.BeginFrame(now);
		return pacer.TakeUIFrame(now);
	};

	CHECK(frameAt(0));
	CHECK_FALSE(frameAt(1));

	// Input on frames 2 and 3
	for (int frame = 2; frame <= 3; ++frame)
	{
		pacer.NoteInteraction(frame / 60.0);
		CHECK(frameAt(frame));
	}

	// Still every frame within the hold time after the last interaction
	int frame = 4;
	for (; (frame - 3) / 60.0 < FramePacer::k_interactionHoldTime - 0.01; ++frame)
	{
		CHECK(frameAt(frame));
	}

	// Past the hold, the next UI frame comes a UI period (6 frames) after the last one
	int lastUIFrame = frame - 1;
	for (; (frame - 3) / 60.0 <= FramePacer::k_interactionHoldTime + 0.01; ++frame)
	{
		if (frameAt(frame))
		{
			lastUIFrame = frame;
		}
	}

	int nextUIFrame = 0;
	for (; frame < lastUIFrame + 20; ++frame)
	{
		if (frameAt(frame))
		{
			nextUIFrame = frame;
			break;
		}
	}
	CHECK(nextUIFrame == lastUIFrame + 6);
}
