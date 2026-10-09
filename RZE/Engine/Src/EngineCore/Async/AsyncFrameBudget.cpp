#include <StdAfx.h>
#include <EngineCore/Async/AsyncFrameBudget.h>

#include <algorithm>

AsyncFrameBudget::AsyncFrameBudget(double timeBudgetMS, size_t uploadBudgetBytes, size_t arenaBudgetBytes, size_t arenaCapacityBytes)
	: m_timeBudgetMS(timeBudgetMS)
	, m_uploadBytesRemaining(uploadBudgetBytes)
	, m_arenaBytesRemaining(arenaBudgetBytes)
	, m_arenaCapacityBytes(arenaCapacityBytes)
{
	m_timer.Start();
}

bool AsyncFrameBudget::IsExhausted() const
{
	return m_hasConsumedWork && (m_uploadBytesRemaining == 0 || GetElapsedMS() >= m_timeBudgetMS);
}

bool AsyncFrameBudget::TryConsume(const ResourceFinalizeCost& cost)
{
	if (cost.ArenaBytes > m_arenaBytesRemaining)
	{
		return false;
	}

	if (m_hasConsumedWork)
	{
		if (GetElapsedMS() >= m_timeBudgetMS || cost.UploadBytes > m_uploadBytesRemaining)
		{
			return false;
		}
	}

	m_arenaBytesRemaining -= cost.ArenaBytes;
	m_arenaBytesConsumed += cost.ArenaBytes;
	m_uploadBytesRemaining -= std::min(cost.UploadBytes, m_uploadBytesRemaining);
	m_hasConsumedWork = true;

	return true;
}

bool AsyncFrameBudget::CanEverAfford(const ResourceFinalizeCost& cost) const
{
	return cost.ArenaBytes <= m_arenaCapacityBytes;
}

double AsyncFrameBudget::GetElapsedMS() const
{
	return m_timer.GetElapsedMS<double>();
}
