#include <StdAfx.h>

#include <Utils/Platform/Timers/PreciseSleep.h>

#include <Utils/DebugUtils/Debug.h>

#include <chrono>
#include <Windows.h>

namespace
{
	// The timer wakes this long before the end, and the rest is spun
	constexpr double k_spinSeconds = 0.001;
}

PreciseSleep::PreciseSleep()
{
	m_timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
	if (m_timer == nullptr)
	{
		RZE_LOG_WARNING("PreciseSleep: no high-resolution waitable timer (Windows 10 1803 or later). Sleeps will spin.");
	}
}

PreciseSleep::~PreciseSleep()
{
	if (m_timer != nullptr)
	{
		CloseHandle(m_timer);
	}
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
	if (m_timer != nullptr && timerSeconds > 0.0)
	{
		// Negative means relative, in 100 ns units
		LARGE_INTEGER dueTime;
		dueTime.QuadPart = -static_cast<LONGLONG>(timerSeconds * 10'000'000.0);
		if (SetWaitableTimerEx(m_timer, &dueTime, 0, nullptr, nullptr, nullptr, 0))
		{
			WaitForSingleObject(m_timer, INFINITE);
		}
	}

	while (std::chrono::steady_clock::now() < end)
	{
		YieldProcessor();
	}
}
