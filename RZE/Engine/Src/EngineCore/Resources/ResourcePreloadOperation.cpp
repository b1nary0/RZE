#include <StdAfx.h>
#include <EngineCore/Resources/ResourcePreloadOperation.h>

#include <string>

namespace
{
	// Reading and decoding files on workers takes most of the time; uploading takes the rest
	constexpr float k_readWeight = 0.6f;
	constexpr float k_uploadWeight = 0.4f;
}

ResourcePreloadOperation::ResourcePreloadOperation(ResourceDependencyList resources, U32 flags, const std::string& name)
	: AsyncOperation(name, flags, EAsyncOperationKind::ResourceLoad)
	, m_resources(std::move(resources))
	, m_batch(*this)
{
	m_readStage = m_progress.AddStage(k_readWeight);
	m_uploadStage = m_progress.AddStage(k_uploadWeight);
}

ResourceHandle ResourcePreloadOperation::GetHandle(const Filepath& resourcePath) const
{
	if (!m_batch.IsResolved(resourcePath))
	{
		return ResourceHandle::EmptyHandle();
	}
	return RZE().GetResourceHandler().GetHandle(resourcePath);
}

void ResourcePreloadOperation::OnStart()
{
	m_resources.RequestAll(m_batch);
	UpdateProgressAndStatus();
}

void ResourcePreloadOperation::OnTick(AsyncFrameBudget& budget)
{
	m_batch.Tick(budget);
	UpdateProgressAndStatus();

	const U32 requestedCount = m_batch.GetRequestedCount();
	if (m_batch.IsComplete())
	{
		if (m_batch.GetFailedCount() > 0)
		{
			RZE_LOG_ERROR_ARGS("Preloading: %u of %u resources failed to load.", m_batch.GetFailedCount(), requestedCount);
		}
		Complete();
	}
}

void ResourcePreloadOperation::UpdateProgressAndStatus()
{
	const U32 requestedCount = m_batch.GetRequestedCount();
	if (requestedCount == 0)
	{
		return;
	}

	const U32 readCount = m_batch.GetCPULoadedCount();
	m_progress.SetStage(m_readStage, readCount, requestedCount);
	m_progress.SetStageFraction(m_uploadStage, m_batch.GetSettledAmount() / static_cast<float>(requestedCount));
	SetProgress(m_progress.Compute());

	std::string status = readCount < requestedCount
		? "Loading assets (" + std::to_string(readCount) + "/" + std::to_string(requestedCount) + ")"
		: "Uploading to GPU (" + std::to_string(m_batch.GetSettledCount()) + "/" + std::to_string(requestedCount) + ")";
	const std::string currentPath = m_batch.GetCurrentPath();
	if (!currentPath.empty())
	{
		status += "\n" + currentPath;
	}
	SetStatusText(status);
}

void ResourcePreloadOperation::OnCleanup()
{
	// Destroys what was loaded but never registered, e.g. on cancel. What was registered stays held until the operation is
	// destroyed.
	m_batch.DiscardPending();
}
