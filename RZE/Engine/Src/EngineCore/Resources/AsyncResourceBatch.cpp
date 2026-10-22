#include <StdAfx.h>
#include <EngineCore/Resources/AsyncResourceBatch.h>

#include <EngineCore/Async/AsyncFrameBudget.h>
#include <EngineCore/Async/AsyncOperation.h>
#include <EngineCore/Threading/CancellationToken.h>
#include <EngineCore/Threading/MainThreadDispatcher.h>

#include <algorithm>

AsyncResourceBatch::AsyncResourceBatch(AsyncOperation& owner)
	: m_owner(owner)
{
}

AsyncResourceBatch::~AsyncResourceBatch()
{
	DiscardPending();
	ReleaseHeldResources();
}

ResourceHandler& AsyncResourceBatch::GetResourceHandler()
{
	return RZE().GetResourceHandler();
}

bool AsyncResourceBatch::IsComplete() const
{
	return m_settledCount == m_entries.size();
}

float AsyncResourceBatch::GetSettledAmount() const
{
	float amount = static_cast<float>(m_settledCount);
	for (const std::unique_ptr<Entry>& entry : m_entries)
	{
		if (entry->State == EEntryState::Finalizing && entry->Resource != nullptr)
		{
			amount += std::clamp(entry->Resource->GetFinalizeProgress(), 0.0f, 1.0f);
		}
	}
	return amount;
}

std::string AsyncResourceBatch::GetCurrentPath() const
{
	for (const std::unique_ptr<Entry>& entry : m_entries)
	{
		if (entry->State != EEntryState::Resolved && entry->State != EEntryState::Failed)
		{
			return entry->Path.GetRelativePath();
		}
	}
	return std::string();
}

bool AsyncResourceBatch::IsResolved(const Filepath& resourcePath) const
{
	const auto iter = m_entriesByKey.find(ResourceHandler::CreateResourceKey(resourcePath));
	return iter != m_entriesByKey.end() && iter->second->State == EEntryState::Resolved;
}

void AsyncResourceBatch::ReleaseHeldResources()
{
	m_heldResources.clear();
}

void AsyncResourceBatch::DiscardPending()
{
	for (std::unique_ptr<Entry>& entry : m_entries)
	{
		if (entry->Resource != nullptr)
		{
			AssertMsg(entry->CPUJob.IsComplete(), "Discarding a resource while its CPU load is still running.");

			entry->Resource->Release();
			entry->Resource.reset();
		}

		EndLoad(*entry);
	}
}

AsyncResourceBatch::Entry* AsyncResourceBatch::BeginRequest(const Filepath& resourcePath)
{
	AssertExpr(Threading::MainThreadDispatcher::Get().IsMainThread());

	const std::string resourceKey = ResourceHandler::CreateResourceKey(resourcePath);

	Entry* entry = nullptr;
	bool isNewEntry = false;

	auto iter = m_entriesByKey.find(resourceKey);
	if (iter != m_entriesByKey.end())
	{
		entry = iter->second;
	}
	else
	{
		m_entries.push_back(std::make_unique<Entry>());
		entry = m_entries.back().get();
		entry->Path = resourcePath;
		entry->Key = resourceKey;

		m_entriesByKey.emplace(resourceKey, entry);
		isNewEntry = true;
	}

	if (m_currentParent != nullptr && m_currentParent != entry)
	{
		std::vector<Entry*>& dependencies = m_currentParent->Dependencies;
		if (std::find(dependencies.begin(), dependencies.end(), entry) == dependencies.end())
		{
			dependencies.push_back(entry);
		}
	}

	if (!isNewEntry)
	{
		return nullptr;
	}

	ResourceHandle existingHandle = GetResourceHandler().GetHandle(resourcePath);
	if (existingHandle.IsValid())
	{
		ResolveWithHandle(*entry, std::move(existingHandle));
		return nullptr;
	}

	return entry;
}

void AsyncResourceBatch::StartLoad(Entry& entry)
{
	std::unique_ptr<IResource> resource = entry.Create();
	if (!resource->SupportsAsyncLoad())
	{
		resource.reset();
		ResolveWithHandle(entry, entry.LoadSynchronously());
		return;
	}

	if (!GetResourceHandler().TryBeginAsyncLoad(entry.Key, this))
	{
		entry.State = EEntryState::WaitingForOtherLoad;
		return;
	}

	entry.OwnsLoad = true;
	RZE_LOG_DEBUG_ARGS("Creating resource [%s]", entry.Key.c_str());
	StartCPULoad(entry, std::move(resource));
}

void AsyncResourceBatch::StartCPULoad(Entry& entry, std::unique_ptr<IResource> resource)
{
	entry.State = EEntryState::LoadingCPU;
	entry.Resource = std::move(resource);

	Entry* const entryPtr = &entry;
	IResource* const resourcePtr = entry.Resource.get();
	const Threading::CancellationToken cancellationToken = m_owner.GetCancellationToken();

	entry.CPUJob = m_owner.PushWorkerJob(Threading::Job::Task([entryPtr, resourcePtr, cancellationToken]()
		{
			OPTICK_EVENT("AsyncResourceBatch CPU load");

			if (!cancellationToken.IsCancellationRequested())
			{
				entryPtr->CPULoadSucceeded = resourcePtr->LoadCPU(entryPtr->Path);
			}
		}));
}

void AsyncResourceBatch::ResolveWithHandle(Entry& entry, ResourceHandle handle)
{
	MarkCPULoaded(entry);

	if (!handle.IsValid())
	{
		FailEntry(entry, "load failed");
		return;
	}

	m_heldResources.push_back(std::move(handle));

	entry.State = EEntryState::Resolved;
	++m_settledCount;
}

void AsyncResourceBatch::EndLoad(Entry& entry)
{
	if (entry.OwnsLoad)
	{
		GetResourceHandler().EndAsyncLoad(entry.Key, this);
		entry.OwnsLoad = false;
	}
}

void AsyncResourceBatch::Tick(AsyncFrameBudget& budget)
{
	OPTICK_EVENT();
	AssertExpr(Threading::MainThreadDispatcher::Get().IsMainThread());

	PollWaitingEntries();
	PollCPULoads();
	FinalizeReadyEntries(budget);
}

void AsyncResourceBatch::PollWaitingEntries()
{
	for (std::unique_ptr<Entry>& entryPtr : m_entries)
	{
		Entry& entry = *entryPtr;
		if (entry.State != EEntryState::WaitingForOtherLoad)
		{
			continue;
		}

		ResourceHandle handle = GetResourceHandler().GetHandle(entry.Path);
		if (handle.IsValid())
		{
			ResolveWithHandle(entry, std::move(handle));
		}
		else if (!GetResourceHandler().IsAsyncLoadInProgress(entry.Key))
		{
			// The other load failed or was cancelled (or its resource was already released again), so this batch
			// loads it
			StartLoad(entry);
		}
	}
}

void AsyncResourceBatch::PollCPULoads()
{
	// Index-based: RequestDependencies may append entries while iterating. Entries are heap allocated,
	// so references to them survive reallocation of m_entries.
	for (size_t entryIndex = 0; entryIndex < m_entries.size(); ++entryIndex)
	{
		Entry& entry = *m_entries[entryIndex];
		if (entry.State != EEntryState::LoadingCPU || entry.Resource == nullptr || !entry.CPUJob.IsComplete())
		{
			continue;
		}

		MarkCPULoaded(entry);

		if (entry.CPUJob.WasAbandoned() || !entry.CPULoadSucceeded)
		{
			FailEntry(entry, m_owner.IsCancellationRequested() ? "cancelled" : "CPU load failed");
			continue;
		}

		entry.State = EEntryState::Finalizing;
		m_lastProcessedPath = entry.Path.GetRelativePath();

		m_currentParent = &entry;
		entry.Resource->RequestDependencies(*this);
		m_currentParent = nullptr;
	}
}

void AsyncResourceBatch::FinalizeReadyEntries(AsyncFrameBudget& budget)
{
	for (std::unique_ptr<Entry>& entryPtr : m_entries)
	{
		Entry& entry = *entryPtr;
		if (entry.State != EEntryState::Finalizing || !AreDependenciesSettled(entry))
		{
			continue;
		}

		while (entry.State == EEntryState::Finalizing)
		{
			const ResourceFinalizeCost cost = entry.Resource->GetNextFinalizeStepCost();
			if (!budget.CanEverAfford(cost))
			{
				RZE_LOG_ERROR_ARGS("Resource [%s] needs %zu bytes of render command memory in a single step, which exceeds the arena.",
					entry.Key.c_str(), cost.ArenaBytes);
				FailEntry(entry, "too large to upload");
				break;
			}

			if (!budget.TryConsume(cost))
			{
				// Out of budget for this frame. Stop here so resources keep finalizing in request order.
				return;
			}

			m_lastProcessedPath = entry.Path.GetRelativePath();
			if (entry.Resource->FinalizeStep())
			{
				Register(entry);
			}
		}
	}
}

bool AsyncResourceBatch::AreDependenciesSettled(const Entry& entry) const
{
	return std::all_of(entry.Dependencies.begin(), entry.Dependencies.end(),
		[](const Entry* dependency)
		{
			return dependency->State == EEntryState::Resolved || dependency->State == EEntryState::Failed;
		});
}

void AsyncResourceBatch::Register(Entry& entry)
{
	ResourceHandle handle = GetResourceHandler().RegisterLoadedResource(entry.Path, entry.Resource.release());
	if (handle.IsValid())
	{
		m_heldResources.push_back(std::move(handle));
	}
	// After registering, so batches waiting on it find it
	EndLoad(entry);

	entry.State = EEntryState::Resolved;
	++m_settledCount;
}

void AsyncResourceBatch::FailEntry(Entry& entry, const char* reason)
{
	RZE_LOG_ERROR_ARGS("Async load of resource [%s] failed: %s", entry.Key.c_str(), reason);

	if (entry.Resource != nullptr)
	{
		entry.Resource->Release();
		entry.Resource.reset();
	}
	EndLoad(entry);

	entry.State = EEntryState::Failed;
	++m_settledCount;
	++m_failedCount;
}

void AsyncResourceBatch::MarkCPULoaded(Entry& entry)
{
	if (entry.State == EEntryState::LoadingCPU || entry.State == EEntryState::WaitingForOtherLoad)
	{
		++m_cpuLoadedCount;
	}
}
