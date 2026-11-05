#include <Utils/Platform/Timers/PreciseSleep.h>

#include <cerrno>
#include <chrono>

#include <time.h>

namespace
{
	// The sleep wakes this long before the end, and the rest is spun
	constexpr double k_spinSeconds = 0.001;
}

// clock_nanosleep needs no timer object, so m_timer stays null
PreciseSleep::PreciseSleep()
{
}

PreciseSleep::~PreciseSleep()
{
}

void PreciseSleep::SleepFor(double seconds)
{
	if (seconds <= 0.0)
	{
		return;
	}

	const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now()
		+ std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(seconds));

	const double timerSeconds = seconds - k_spinSeconds;
	if (timerSeconds > 0.0)
	{
		timespec wakeTime;
		clock_gettime(CLOCK_MONOTONIC, &wakeTime);
		const long long wakeNanoseconds = static_cast<long long>(wakeTime.tv_nsec) + static_cast<long long>(timerSeconds * 1'000'000'000.0);
		wakeTime.tv_sec += static_cast<time_t>(wakeNanoseconds / 1'000'000'000);
		wakeTime.tv_nsec = static_cast<long>(wakeNanoseconds % 1'000'000'000);

		// A time to wake at rather than a length, so a signal that cuts it short just sleeps again to the same time
		while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &wakeTime, nullptr) == EINTR)
		{
		}
	}

	while (std::chrono::steady_clock::now() < end)
	{
		__builtin_ia32_pause();
	}
}
