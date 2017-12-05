#include <Utils/StdAfx.h>

#include <Utils/Platform/Timers/HiResTimer.h>

#include <Utils/DebugUtils/Debug.h>

HiResTimer::HiResTimer()
{

}

void HiResTimer::Start()
{
	mStartTime = high_resolution_clock::now();
}

void HiResTimer::Stop()
{
	high_resolution_clock::time_point endTime = high_resolution_clock::now();
	mElapsedTime = duration_cast<duration<double>>(endTime - mStartTime);
}

ScopedHiResTimer::ScopedHiResTimer(const char* TimerName)
	: Name(TimerName)
{
	Timer.Start();
}

ScopedHiResTimer::~ScopedHiResTimer()
{
	Timer.Stop();
	Print();
}

void ScopedHiResTimer::Print()
{
	LOG_CONSOLE_ARGS("%s took %f ms to complete.", Name, Timer.GetElapsedMS<float>());
}
