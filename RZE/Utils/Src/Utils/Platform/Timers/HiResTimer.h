#pragma once

#include <chrono>

using namespace std::chrono;

class HiResTimer
{
public:
	HiResTimer();

	void Start();
	void Stop();

	template <typename TCast>
	TCast GetElapsed()
	{
		high_resolution_clock::time_point endTime = high_resolution_clock::now();

		return duration_cast<duration<TCast>>(endTime - mStartTime).count();
	}

	template <typename TCast>
	TCast GetElapsedMS()
	{
		return GetElapsed<TCast>() * 1000;
	}

private:
	high_resolution_clock::time_point mStartTime;
	duration<double> mElapsedTime;
};

struct ScopedHiResTimer
{
	ScopedHiResTimer(const char* TimerName);
	~ScopedHiResTimer();

	void Print();

	HiResTimer Timer;
	const char* Name;
};