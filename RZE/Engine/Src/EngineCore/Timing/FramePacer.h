#pragma once

#include <EngineCore/Config/FrameSettings.h>

// Decides when frames start and how much time each one gets: the frame-rate caps, the delta time a frame simulates,
// and the spare time left before the next frame. Only the timing logic, given the time in seconds by its caller so it
// can be tested; RZE_Engine does the waiting.
//
//   BeginFrame(now)        at the top of each frame, once the wait for it is over
//   GetIdleTime(now)       after the frame's work: spare time before the next frame is due
//   GetNextFrameStart()    when the CPU cap lets the next frame start
class FramePacer
{
public:
	// Delta time never exceeds this, so breakpoints, window drags and load hitches don't make the scene jump
	static constexpr double k_maxDeltaTime = 0.1;
	// A frame within this fraction of the target period counts as on time, and simulates the measured period
	static constexpr double k_snapTolerance = 0.1;
	// How quickly the measured period follows on-time frames. Small, so it evens out wake-up jitter.
	static constexpr double k_periodSmoothing = 0.05;
	// Spare time ends this early, so whatever runs in it can't make the next frame late
	static constexpr double k_idleSafetyMargin = 0.0015;

public:
	void SetSettings(const FrameSettings& settings);
	const FrameSettings& GetSettings() const { return m_settings; }

	// The display's refresh rate in Hz, which paces frames while vsync is on; 0 when unknown
	void SetRefreshRate(double refreshRate);
	// While throttled (the window is unfocused or minimised), frames run at FrameSettings::UnfocusedFrameRate
	void SetIsThrottled(bool isThrottled);

	void BeginFrame(double now);

	// What the frame simulates: the measured frame time clamped to k_maxDeltaTime, and for frames that arrive on
	// time, the measured average period instead, so timer jitter doesn't show as uneven motion
	double GetDeltaTime() const { return m_deltaTime; }
	// Seconds since the previous frame started, as measured
	double GetFrameTime() const { return m_frameTime; }

	// When the CPU cap lets the next frame start. No later than the current frame's start when there's no cap.
	double GetNextFrameStart() const { return m_nextFrameStart; }
	// Seconds before the next frame is due, less k_idleSafetyMargin. 0 when nothing paces the frames.
	double GetIdleTime(double now) const;

private:
	// Seconds between frame starts the CPU cap aims for; 0 for no cap
	double GetCapPeriod() const;
	// Seconds between frames from the cap or from vsync, whichever is longer; 0 when nothing paces the frames
	double GetFramePeriod() const;

	void UpdateDeltaTime(double framePeriod);
	void ScheduleNextFrame(double now);

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
};
