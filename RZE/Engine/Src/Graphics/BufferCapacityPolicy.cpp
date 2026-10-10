#include <StdAfx.h>
#include <Graphics/BufferCapacityPolicy.h>

#include <Utils/DebugUtils/Debug.h>

#include <Utils/Math/Math.h>

BufferCapacityPolicy::BufferCapacityPolicy(const BufferCapacitySettings& settings)
	: m_settings(settings)
	, m_capacity(settings.MinCapacity)
{
	AssertExpr(m_settings.MinCapacity > 0);
	AssertExpr(m_settings.ShrinkWindowCalls > 0);
	AssertExpr(m_settings.ShrinkUsageDivisor > 1);
}

bool BufferCapacityPolicy::Update(U32 needed)
{
	bool capacityChanged = false;

	if (needed > m_capacity)
	{
		m_capacity = std::max(m_settings.MinCapacity, MathUtils::CeilPowerOfTwo(needed));
		capacityChanged = true;

		// A buffer that just grew shouldn't be considered for shrinking until full windows pass again
		m_peakThisWindow = needed;
		m_peakLastWindow = needed;
		m_callsInWindow = 0;
	}

	m_peakThisWindow = std::max(m_peakThisWindow, needed);
	++m_callsInWindow;

	if (m_callsInWindow < m_settings.ShrinkWindowCalls)
	{
		return capacityChanged;
	}

	const U32 recentPeak = std::max(m_peakThisWindow, m_peakLastWindow);
	if (m_capacity > m_settings.MinCapacity && recentPeak * m_settings.ShrinkUsageDivisor <= m_capacity)
	{
		m_capacity = std::max(m_settings.MinCapacity, MathUtils::CeilPowerOfTwo(recentPeak * 2));
		capacityChanged = true;
	}

	m_peakLastWindow = m_peakThisWindow;
	m_peakThisWindow = 0;
	m_callsInWindow = 0;

	return capacityChanged;
}
