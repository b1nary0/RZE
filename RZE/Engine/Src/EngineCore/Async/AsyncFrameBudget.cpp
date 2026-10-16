#include <StdAfx.h>
#include <EngineCore/Async/AsyncFrameBudget.h>

#include <algorithm>

AsyncFrameBudget::AsyncFrameBudget(double timeBudgetMS, size_t uploadBudgetBytes, size_t arenaBudgetBytes, size_t arenaCapacityBytes, bool isIdle)
	: m_timeBudgetMS(timeBudgetMS)
	, m_uploadBytesRemaining(uploadBudgetBytes)
	, m_arenaBytesRemaining(arenaBudgetBytes)
	, m_arenaCapacityBytes(arenaCapacityBytes)
	, m_isIdle(isIdle)
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
	const size_t uploadBytes = std::min(cost.UploadBytes, m_uploadBytesRemaining);
	m_uploadBytesRemaining -= uploadBytes;
	m_uploadBytesConsumed += uploadBytes;
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
