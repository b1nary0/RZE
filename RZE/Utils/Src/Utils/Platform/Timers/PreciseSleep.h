#pragma once

// Puts the calling thread to sleep for a precise length of time, e.g. until the next frame is due. The OS's sleep
// covers most of it and a short spin the rest, since wake-ups can be up to a millisecond late. On Windows that's a
// high-resolution waitable timer; without one (before Windows 10 1803) it spins the whole time.
class PreciseSleep
{
public:
	PreciseSleep();
	~PreciseSleep();

	PreciseSleep(const PreciseSleep&) = delete;
	PreciseSleep& operator=(const PreciseSleep&) = delete;

public:
	// Returns at once for zero or less
	void SleepFor(double seconds);

private:
	// The OS's timer where it needs one (a waitable timer's HANDLE on Windows), or null
	void* m_timer = nullptr;
};
