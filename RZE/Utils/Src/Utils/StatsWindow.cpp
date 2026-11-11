#include <Utils/StatsWindow.h>

bool StatsWindow::AddFrame(double now)
{
	if (!HasStarted())
	{
		m_start = now;
		return false;
	}

	++m_frames;
	if (now - m_start < k_windowSeconds)
	{
		return false;
	}

	m_closedFrames = m_frames;
	m_closedSeconds = now - m_start;

	m_start = now;
	m_frames = 0;
	return true;
}

double StatsWindow::PerFrame(double total) const
{
	return m_closedFrames > 0 ? total / static_cast<double>(m_closedFrames) : 0.0;
}

double StatsWindow::PerSecond(double count) const
{
	return m_closedSeconds > 0.0 ? count / m_closedSeconds : 0.0;
}
