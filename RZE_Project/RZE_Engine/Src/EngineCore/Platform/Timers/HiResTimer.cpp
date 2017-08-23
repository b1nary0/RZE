#include <StdAfx.h>

#include <EngineCore/Platform/Timers/HiResTimer.h>

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

