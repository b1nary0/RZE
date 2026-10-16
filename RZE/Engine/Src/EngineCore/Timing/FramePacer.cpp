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
		UpdateStats(now);
	}
	m_hasStarted = true;
	m_frameStart = now;

	ScheduleNextFrame(now);
}

bool FramePacer::TakeUIFrame(double now)
{
	const double uiPeriod = m_settings.UIFrameRate > 0.0 ? 1.0 / m_settings.UIFrameRate : 0.0;

	// Half a frame early counts as due, so jitter doesn't push a UI frame to the frame after
	const double framePeriod = GetFramePeriod() > 0.0 ? GetFramePeriod() : m_frameTime;
	const double tolerance = framePeriod * 0.5;

	const bool isInteracting = now - m_lastInteraction <= k_interactionHoldTime;
	const bool isDue = uiPeriod <= 0.0 || isInteracting || now + tolerance >= m_nextUIFrame;
	if (!isDue && !m_isUIFrameRequested)
	{
		return false;
	}
	m_isUIFrameRequested = false;
	++m_windowUIFrames;

	// From when this one was due, so the cadence holds. A UI frame over a period late re-bases on now, and so does one
	// that came early because it was requested or the user is interacting.
	if (uiPeriod > 0.0)
	{
		const double scheduled = m_nextUIFrame + uiPeriod;
		const bool isLate = now - m_nextUIFrame > uiPeriod;
		m_nextUIFrame = isLate || scheduled > now + uiPeriod + tolerance ? now + uiPeriod : scheduled;
	}

	return true;
}

void FramePacer::EndFrameWork(double now)
{
	m_frameWorkEnd = now;
}

void FramePacer::AddIdleWork(double seconds)
{
	m_frameIdleWork += seconds;
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

void FramePacer::UpdateStats(double now)
{
	++m_windowFrames;
	m_windowFrameTime += m_frameTime;
	if (m_frameWorkEnd >= 0.0)
	{
		m_windowIdleTime += now - m_frameWorkEnd;
	}
	m_windowIdleWork += m_frameIdleWork;

	m_frameWorkEnd = -1.0;
	m_frameIdleWork = 0.0;

	if (m_windowFrameTime < k_statsWindow)
	{
		return;
	}

	const double frames = static_cast<double>(m_windowFrames);
	m_stats.FrameTimeMS = m_windowFrameTime / frames * 1000.0;
	m_stats.FramesPerSecond = frames / m_windowFrameTime;
	m_stats.IdleTimeMS = m_windowIdleTime / frames * 1000.0;
	m_stats.IdleWorkMS = m_windowIdleWork / frames * 1000.0;
	m_stats.UIFramesPerSecond = static_cast<double>(m_windowUIFrames) / m_windowFrameTime;

	m_windowFrames = 0;
	m_windowFrameTime = 0.0;
	m_windowIdleTime = 0.0;
	m_windowIdleWork = 0.0;
	m_windowUIFrames = 0;
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
