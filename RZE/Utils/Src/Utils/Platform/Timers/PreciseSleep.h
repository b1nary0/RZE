#pragma once

// Puts the calling thread to sleep for a precise length of time, e.g. until the next frame is due. A high-resolution
// waitable timer covers most of it and a short spin the rest, since timer wake-ups can be up to a millisecond late.
// Without a high-resolution timer (before Windows 10 1803) it spins the whole time.
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
	// The waitable timer's HANDLE, or null if it couldn't be created
	void* m_timer = nullptr;
};
