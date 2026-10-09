#include <StdAfx.h>
#include <EngineCore/Async/AsyncOperationManager.h>

#include <EngineCore/Async/AsyncFrameBudget.h>
#include <EngineCore/Threading/MainThreadDispatcher.h>
#include <EngineCore/Threading/JobSystem/JobScheduler.h>

#include <Rendering/MemArena.h>

#include <Utils/Memory/MemoryUtils.h>

namespace
{
	const size_t k_defaultUploadBudgetBytes = MemoryUtils::Megabytes(16);

	// Headroom left in the render command arena for whatever else the frame submits.
	const size_t k_arenaSafetyMarginBytes = MemoryUtils::Megabytes(8);
}

AsyncOperationManager::AsyncOperationManager()
	: m_frameBudgetMS(k_defaultFrameBudgetMS)
	, m_uploadBudgetBytes(k_defaultUploadBudgetBytes)
{
}

AsyncOperationManager::~AsyncOperationManager()
{
	AssertExpr(m_entries.empty());
}

void AsyncOperationManager::Initialize()
{
	m_isShutDown = false;
}

void AsyncOperationManager::ShutDown()
{
	AssertExpr(Threading::MainThreadDispatcher::Get().IsMainThread());
	AssertExpr(!m_isTicking);

	m_isShutDown = true;

	for (std::shared_ptr<AsyncOperation>& operation : m_pendingStarts)
	{
		m_entries.push_back({ operation });
	}
	m_pendingStarts.clear();

	for (Entry& entry : m_entries)
	{
		entry.Operation->RequestCancel();
	}

	for (Entry& entry : m_entries)
	{
		for (const Threading::JobHandle& handle : entry.Operation->m_outstandingJobs)
		{
			Threading::JobScheduler::Get().Wait(handle);
		}
		entry.Operation->m_outstandingJobs.clear();

		if (entry.Operation->GetState() != EAsyncOperationState::NotStarted)
		{
			entry.Operation->OnCleanup();
		}
	}

	m_entries.clear();
}

void AsyncOperationManager::Start(const std::shared_ptr<AsyncOperation>& operation)
{
	AssertExpr(Threading::MainThreadDispatcher::Get().IsMainThread());
	AssertNotNull(operation);
	AssertExpr(operation->GetState() == EAsyncOperationState::NotStarted);

	if (m_isShutDown)
	{
		RZE_LOG_ARGS("AsyncOperationManager::Start called after shutdown. Operation [%s] ignored.", operation->GetName().c_str());
		return;
	}

	if (m_isTicking)
	{
		// Started from inside a callback or another operation's tick; picked up at the end of this Tick().
		m_pendingStarts.push_back(operation);
		return;
	}

	StartInternal(operation);
}

void AsyncOperationManager::StartInternal(const std::shared_ptr<AsyncOperation>& operation)
{
	operation->m_state.store(EAsyncOperationState::Running, std::memory_order_release);
	m_entries.push_back({ operation });

	operation->OnStart();
}

void AsyncOperationManager::Tick()
{
	OPTICK_EVENT();
	AssertExpr(Threading::MainThreadDispatcher::Get().IsMainThread());

	m_isTicking = true;
	{
		AsyncFrameBudget budget = CreateFrameBudget();

		// Index-based: StartInternal is deferred while ticking, so m_entries is stable during this loop.
		for (size_t entryIndex = 0; entryIndex < m_entries.size();)
		{
			if (TickEntry(m_entries[entryIndex], budget))
			{
				m_entries.erase(m_entries.begin() + entryIndex);
			}
			else
			{
				++entryIndex;
			}
		}

		m_arenaBytesConsumedLastFrame = budget.GetArenaBytesConsumed();
	}
	m_isTicking = false;

	std::vector<std::shared_ptr<AsyncOperation>> pendingStarts;
	pendingStarts.swap(m_pendingStarts);
	for (const std::shared_ptr<AsyncOperation>& operation : pendingStarts)
	{
		StartInternal(operation);
	}
}

bool AsyncOperationManager::TickEntry(Entry& entry, AsyncFrameBudget& budget)
{
	AsyncOperation& operation = *entry.Operation;

	if (entry.IsFinished)
	{
		if (entry.RetireFramesRemaining > 0)
		{
			--entry.RetireFramesRemaining;
			return false;
		}

		operation.OnCleanup();
		return true;
	}

	if (!operation.HasRequestedFinish() && !operation.IsCancellationRequested())
	{
		operation.OnTick(budget);
	}

	EAsyncOperationState finalState = operation.m_requestedFinalState;
	if (finalState == EAsyncOperationState::NotStarted && operation.IsCancellationRequested())
	{
		finalState = EAsyncOperationState::Cancelled;
	}

	// Wait for worker jobs to drain before finishing; they may still be touching the operation.
	if (finalState != EAsyncOperationState::NotStarted && !operation.HasOutstandingJobs())
	{
		operation.Finish(finalState);

		entry.IsFinished = true;
		entry.RetireFramesRemaining = k_retireFrameCount;
	}

	return false;
}

bool AsyncOperationManager::HasBlockingOperation() const
{
	return GetBlockingOperation() != nullptr;
}

const AsyncOperation* AsyncOperationManager::GetBlockingOperation() const
{
	for (const Entry& entry : m_entries)
	{
		if (!entry.IsFinished && entry.Operation->IsBlocking())
		{
			return entry.Operation.get();
		}
	}

	return nullptr;
}

void AsyncOperationManager::SetFrameBudget(double timeBudgetMS, size_t uploadBudgetBytes)
{
	m_frameBudgetMS = timeBudgetMS;
	m_uploadBudgetBytes = uploadBudgetBytes;
}

AsyncFrameBudget AsyncOperationManager::CreateFrameBudget() const
{
	// Estimate what the rest of the frame will need from the render command arena by taking last frame's
	// usage minus what async operations consumed last frame, and hand out what's left.
	const size_t arenaSize = Rendering::MemArena::GetSize();
	const size_t lastFrameTotal = Rendering::MemArena::GetTotalMemoryAllocatedLastFrame();
	const size_t baselineUsage = lastFrameTotal > m_arenaBytesConsumedLastFrame ? lastFrameTotal - m_arenaBytesConsumedLastFrame : 0;

	const size_t arenaCapacity = arenaSize > k_arenaSafetyMarginBytes ? arenaSize - k_arenaSafetyMarginBytes : 0;
	const size_t arenaBudget = arenaCapacity > baselineUsage ? arenaCapacity - baselineUsage : 0;

	return AsyncFrameBudget(m_frameBudgetMS, m_uploadBudgetBytes, arenaBudget, arenaCapacity);
}
