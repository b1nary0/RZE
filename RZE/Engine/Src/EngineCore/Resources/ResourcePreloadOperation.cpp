#include <StdAfx.h>
#include <EngineCore/Resources/ResourcePreloadOperation.h>

ResourcePreloadOperation::ResourcePreloadOperation(ResourceDependencyList resources, U32 flags)
	: AsyncOperation("Preloading resources", flags)
	, m_resources(std::move(resources))
	, m_batch(*this)
{
}

void ResourcePreloadOperation::OnStart()
{
	m_resources.RequestAll(m_batch);
}

void ResourcePreloadOperation::OnTick(AsyncFrameBudget& budget)
{
	m_batch.Tick(budget);

	const U32 requestedCount = m_batch.GetRequestedCount();
	SetProgress(requestedCount > 0 ? static_cast<float>(m_batch.GetSettledCount()) / static_cast<float>(requestedCount) : 1.0f);
	SetStatusText(m_batch.GetLastProcessedPath());

	if (m_batch.IsComplete())
	{
		if (m_batch.GetFailedCount() > 0)
		{
			RZE_LOG_ARGS("Preloading: %u of %u resources failed to load.", m_batch.GetFailedCount(), requestedCount);
		}
		Complete();
	}
}

void ResourcePreloadOperation::OnCleanup()
{
	// Any GPU uploads that read a never-registered resource's memory have been processed by now. What was loaded stays
	// held until the operation is destroyed.
	m_batch.DiscardPending();
}
