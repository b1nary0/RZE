#pragma once

#include <EngineCore/Config/FrameSettings.h>

// Decides when frames start and how much time each one gets: the frame-rate caps, the delta time a frame simulates,
// and the spare time left before the next frame. Only the timing logic, given the time in seconds by its caller so it
// can be tested; RZE_Engine does the waiting.
//
//   BeginFrame(now)        at the top of each frame, once the wait for it is over
//   TakeUIFrame(now)       whether this frame updates the UI (FrameSettings::UIFrameRate)
//   EndFrameWork(now)      after the frame's work; until the next BeginFrame is idle time
//   GetIdleTime(now)       spare time before the next frame is due
//   GetNextFrameStart()    when the CPU cap lets the next frame start
class FramePacer
{
public:
	// Averages over the last k_statsWindow seconds of frames, for display
	struct Stats
	{
		double FrameTimeMS = 0.0;
		double FramesPerSecond = 0.0;
		// Per frame, from the end of its work to the next frame's start
		double IdleTimeMS = 0.0;
		// The part of IdleTimeMS spent on work (AddIdleWork); the rest is waiting
		double IdleWorkMS = 0.0;
		// How many frames a second updated the UI (TakeUIFrame)
		double UIFramesPerSecond = 0.0;
	};

public:
	// Delta time never exceeds this, so breakpoints, window drags and load hitches don't make the scene jump
	static constexpr double k_maxDeltaTime = 0.1;
	// A frame within this fraction of the target period counts as on time, and simulates the measured period
	static constexpr double k_snapTolerance = 0.1;
	// How quickly the measured period follows on-time frames. Small, so it evens out wake-up jitter.
	static constexpr double k_periodSmoothing = 0.05;
	// Spare time ends this early, so whatever runs in it can't make the next frame late
	static constexpr double k_idleSafetyMargin = 0.0015;
	// Seconds of frames that Stats averages over. They're updated once per window, so they stay readable.
	static constexpr double k_statsWindow = 0.5;
	// After the last interaction (NoteInteraction), the UI keeps updating every frame for this long, so short pauses in
	// a drag don't drop it back to the UI frame rate
	static constexpr double k_interactionHoldTime = 0.25;

public:
	void SetSettings(const FrameSettings& settings);
	const FrameSettings& GetSettings() const { return m_settings; }

	// The display's refresh rate in Hz, which paces frames while vsync is on; 0 when unknown
	void SetRefreshRate(double refreshRate);
	// While throttled (the window is unfocused or minimised), frames run at FrameSettings::UnfocusedFrameRate
	void SetIsThrottled(bool isThrottled);

	void BeginFrame(double now);
	// Whether this frame updates the UI. Always with no UI frame rate, or while the user is interacting; otherwise once the
	// next UI frame is due (within half a frame, so a UI rate that divides the frame rate gets an even cadence), or when
	// one was requested. Once per frame, after anything that might request one.
	bool TakeUIFrame(double now);
	// Makes the next frame a UI frame, e.g. when the window resizes. The first frame always is one.
	void RequestUIFrame() { m_isUIFrameRequested = true; }
	// The user is interacting (input this frame, or a mouse button held): every frame updates the UI until
	// k_interactionHoldTime after the last time this is called, so what's drawn over the scene keeps up
	void NoteInteraction(double now) { m_lastInteraction = now; }

	// The frame's work is done; until the next BeginFrame counts as idle time
	void EndFrameWork(double now);
	// Seconds of this frame's idle time spent on work, e.g. async operations
	void AddIdleWork(double seconds);

	// What the frame simulates: the measured frame time clamped to k_maxDeltaTime, and for frames that arrive on
	// time, the measured average period instead, so timer jitter doesn't show as uneven motion
	double GetDeltaTime() const { return m_deltaTime; }
	// Seconds since the previous frame started, as measured
	double GetFrameTime() const { return m_frameTime; }

	// When the CPU cap lets the next frame start. No later than the current frame's start when there's no cap.
	double GetNextFrameStart() const { return m_nextFrameStart; }
	// Seconds before the next frame is due, less k_idleSafetyMargin. 0 when nothing paces the frames.
	double GetIdleTime(double now) const;

	// Zero until the first window of frames has passed
	const Stats& GetStats() const { return m_stats; }

private:
	// Seconds between frame starts the CPU cap aims for; 0 for no cap
	double GetCapPeriod() const;
	// Seconds between frames from the cap or from vsync, whichever is longer; 0 when nothing paces the frames
	double GetFramePeriod() const;

	void UpdateDeltaTime(double framePeriod);
	void ScheduleNextFrame(double now);
	// Adds the frame that just ended to the stats window, which closes once it holds k_statsWindow seconds
	void UpdateStats(double now);

private:
	FrameSettings m_settings;
	double m_refreshRate = 0.0;
	bool m_isThrottled = false;

	bool m_hasStarted = false;
	double m_frameStart = 0.0;
	double m_nextFrameStart = 0.0;
	double m_frameTime = 0.0;
	double m_deltaTime = 0.0;
	// Running average of on-time frame times. It converges on the real cadence, e.g. 59.94 Hz on a display that
	// reports 59 Hz, so on-time frames simulate exactly as much time as passes.
	double m_measuredPeriod = 0.0;

	// When the current frame's work ended; negative until EndFrameWork
	double m_frameWorkEnd = -1.0;
	double m_frameIdleWork = 0.0;

	// When the next UI frame is due, on the UI rate's schedule
	double m_nextUIFrame = 0.0;
	bool m_isUIFrameRequested = true;
	// When NoteInteraction was last called; long enough ago to not count until it is
	double m_lastInteraction = -1000.0;

	Stats m_stats;
	// The stats window so far
	int m_windowFrames = 0;
	double m_windowFrameTime = 0.0;
	double m_windowIdleTime = 0.0;
	double m_windowIdleWork = 0.0;
	int m_windowUIFrames = 0;
};
