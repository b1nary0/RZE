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
		return duration_cast<duration<TCast>>(mElapsedTime).count();
	}

private:
	high_resolution_clock::time_point mStartTime;
	duration<double> mElapsedTime;
};