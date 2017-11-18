#pragma once

#include <chrono>

using namespace std::chrono;

class HiResTimer
{
public:
	HiResTimer();

	void Start();
	void Stop();
	void Reset();

	template <typename TCast>
	TCast GetElapsed()
	{
		high_resolution_clock::time_point endTime = high_resolution_clock::now();

		return duration_cast<duration<TCast>>(endTime - mStartTime).count();
	}

private:
	high_resolution_clock::time_point mStartTime;
	duration<double> mElapsedTime;
};