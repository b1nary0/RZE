#include <StdAfx.h>
#include <EngineCore/Timing/FramePacer.h>

#include <algorithm>
#include <cmath>

void FramePacer::SetSettings(const FrameSettings& settings)
{
	m_settings = settings;
}

void FramePacer::SetRefreshRate(double refreshRate)
{
	m_refreshRate = refreshRate;
}

void FramePacer::SetIsThrottled(bool isThrottled)
{
	m_isThrottled = isThrottled;
}

void FramePacer::BeginFrame(double now)
{
	if (m_hasStarted)
	{
		m_frameTime = now - m_frameStart;
		UpdateDeltaTime(GetFramePeriod());
	}
	m_hasStarted = true;
	m_frameStart = now;

	ScheduleNextFrame(now);
}

double FramePacer::GetIdleTime(double now) const
{
	const double framePeriod = GetFramePeriod();
	if (framePeriod <= 0.0)
	{
		return 0.0;
	}

	// The CPU cap's schedule, or a refresh period after this frame started when vsync paces frames more slowly
	const double deadline = std::max(m_nextFrameStart, m_frameStart + framePeriod);
	return std::max(0.0, deadline - now - k_idleSafetyMargin);
}

double FramePacer::GetCapPeriod() const
{
	double frameRate = m_settings.MaxFrameRate;
	if (m_isThrottled && m_settings.UnfocusedFrameRate > 0.0)
	{
		frameRate = frameRate > 0.0 ? std::min(frameRate, m_settings.UnfocusedFrameRate) : m_settings.UnfocusedFrameRate;
	}

	return frameRate > 0.0 ? 1.0 / frameRate : 0.0;
}

double FramePacer::GetFramePeriod() const
{
	const double vsyncPeriod = m_settings.VSync && m_refreshRate > 0.0 ? 1.0 / m_refreshRate : 0.0;
	return std::max(GetCapPeriod(), vsyncPeriod);
}

void FramePacer::UpdateDeltaTime(double framePeriod)
{
	m_deltaTime = std::min(m_frameTime, k_maxDeltaTime);

	const double tolerance = framePeriod * k_snapTolerance;
	if (framePeriod <= 0.0 || std::abs(m_frameTime - framePeriod) > tolerance)
	{
		return;
	}

	// Starts over from the target when the target has moved, e.g. when the window loses focus
	if (std::abs(m_measuredPeriod - framePeriod) > tolerance)
	{
		m_measuredPeriod = framePeriod;
	}
	m_measuredPeriod += (m_frameTime - m_measuredPeriod) * k_periodSmoothing;
	m_deltaTime = m_measuredPeriod;
}

void FramePacer::ScheduleNextFrame(double now)
{
	const double capPeriod = GetCapPeriod();
	if (capPeriod <= 0.0)
	{
		m_nextFrameStart = now;
		return;
	}

	// From this frame's scheduled start rather than when it actually started, so small oversleeps don't add up.
	// A frame that started over a period late re-bases on now instead, so the frames after it don't rush to catch
	// up; so does a schedule further out than one period, which a lower cap left behind.
	const double scheduled = m_nextFrameStart + capPeriod;
	const bool isLate = now - m_nextFrameStart > capPeriod;
	m_nextFrameStart = isLate || scheduled > now + capPeriod ? now + capPeriod : scheduled;
}
