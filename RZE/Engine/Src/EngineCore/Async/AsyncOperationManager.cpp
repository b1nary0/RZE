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
	AssertExpr(m_operations.empty());
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
		m_operations.push_back(operation);
	}
	m_pendingStarts.clear();

	for (const std::shared_ptr<AsyncOperation>& operation : m_operations)
	{
		operation->RequestCancel();
	}

	for (const std::shared_ptr<AsyncOperation>& operation : m_operations)
	{
		for (const Threading::JobHandle& handle : operation->m_outstandingJobs)
		{
			Threading::JobScheduler::Get().Wait(handle);
		}
		operation->m_outstandingJobs.clear();

		if (operation->GetState() != EAsyncOperationState::NotStarted)
		{
			operation->OnCleanup();
		}
	}

	m_operations.clear();
}

void AsyncOperationManager::Start(const std::shared_ptr<AsyncOperation>& operation)
{
	AssertExpr(Threading::MainThreadDispatcher::Get().IsMainThread());
	AssertNotNull(operation);
	AssertExpr(operation->GetState() == EAsyncOperationState::NotStarted);

	if (m_isShutDown)
	{
		RZE_LOG_WARNING_ARGS("AsyncOperationManager::Start called after shutdown. Operation [%s] ignored.", operation->GetName().c_str());
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
	operation->m_startTime = std::chrono::steady_clock::now();
	operation->m_state.store(EAsyncOperationState::Running, std::memory_order_release);
	m_operations.push_back(operation);

	operation->OnStart();
}

void AsyncOperationManager::Tick()
{
	OPTICK_EVENT();
	AssertExpr(Threading::MainThreadDispatcher::Get().IsMainThread());

	m_isTicking = true;
	{
		AsyncFrameBudget foregroundBudget = CreateFrameBudget(m_frameBudgetMS, false);
		TickOperations(false, foregroundBudget);

		// Only what the foreground left of the render command arena, which is a hard limit
		AsyncFrameBudget backgroundBudget(k_backgroundFrameBudgetMS, m_uploadBudgetBytes / k_backgroundUploadBudgetDivisor,
			foregroundBudget.GetArenaBytesRemaining(), foregroundBudget.GetArenaCapacityBytes());
		TickOperations(true, backgroundBudget);

		m_arenaBytesConsumedLastFrame = m_idleArenaBytesConsumed + foregroundBudget.GetArenaBytesConsumed() + backgroundBudget.GetArenaBytesConsumed();
		m_idleArenaBytesConsumed = 0;
		m_idleUploadBytesConsumed = 0;
	}
	m_isTicking = false;

	std::vector<std::shared_ptr<AsyncOperation>> pendingStarts;
	pendingStarts.swap(m_pendingStarts);
	for (const std::shared_ptr<AsyncOperation>& operation : pendingStarts)
	{
		StartInternal(operation);
	}
}

void AsyncOperationManager::TickIdle(double timeMS)
{
	OPTICK_EVENT();
	AssertExpr(Threading::MainThreadDispatcher::Get().IsMainThread());

	if (m_operations.empty())
	{
		return;
	}

	m_isTicking = true;
	{
		AsyncFrameBudget budget = CreateFrameBudget(timeMS, true);
		TickRunningOperations(false, budget);
		TickRunningOperations(true, budget);

		m_idleArenaBytesConsumed += budget.GetArenaBytesConsumed();
		m_idleUploadBytesConsumed += budget.GetUploadBytesConsumed();
	}
	m_isTicking = false;
}

void AsyncOperationManager::TickRunningOperations(bool background, AsyncFrameBudget& budget)
{
	for (const std::shared_ptr<AsyncOperation>& operation : m_operations)
	{
		if (operation->IsBackground() == background && !operation->HasRequestedFinish() && !operation->IsCancellationRequested())
		{
			operation->OnTick(budget);
		}
	}
}

void AsyncOperationManager::TickOperations(bool background, AsyncFrameBudget& budget)
{
	// Index-based: StartInternal is deferred while ticking, so m_operations only changes through the erase here.
	for (size_t operationIndex = 0; operationIndex < m_operations.size();)
	{
		AsyncOperation& operation = *m_operations[operationIndex];
		if (operation.IsBackground() == background && TickOperation(operation, budget))
		{
			m_operations.erase(m_operations.begin() + operationIndex);
		}
		else
		{
			++operationIndex;
		}
	}
}

bool AsyncOperationManager::TickOperation(AsyncOperation& operation, AsyncFrameBudget& budget)
{
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
	if (finalState == EAsyncOperationState::NotStarted || operation.HasOutstandingJobs())
	{
		return false;
	}

	operation.Finish(finalState);
	// Straight away: what the render thread may still use is held by its commands, not by the operation
	operation.OnCleanup();
	return true;
}

bool AsyncOperationManager::HasBlockingOperation() const
{
	return GetBlockingOperation() != nullptr;
}

const AsyncOperation* AsyncOperationManager::GetBlockingOperation() const
{
	for (const std::shared_ptr<AsyncOperation>& operation : m_operations)
	{
		if (operation->IsBlocking())
		{
			return operation.get();
		}
	}

	return nullptr;
}

std::vector<std::shared_ptr<AsyncOperation>> AsyncOperationManager::GetVisibleOperations() const
{
	std::vector<std::shared_ptr<AsyncOperation>> operations;
	for (const std::shared_ptr<AsyncOperation>& operation : m_operations)
	{
		if (m_visibilityFilter.Matches(*operation))
		{
			operations.push_back(operation);
		}
	}
	return operations;
}

std::vector<std::shared_ptr<AsyncOperation>> AsyncOperationManager::GetRunningOperations() const
{
	return m_operations;
}

void AsyncOperationManager::SetFrameBudget(double timeBudgetMS, size_t uploadBudgetBytes)
{
	m_frameBudgetMS = timeBudgetMS;
	m_uploadBudgetBytes = uploadBudgetBytes;
}

AsyncFrameBudget AsyncOperationManager::CreateFrameBudget(double timeBudgetMS, bool isIdle) const
{
	// Estimate what the rest of the frame will need from the render command arena by taking last frame's
	// usage minus what async operations consumed last frame, and hand out what's left.
	const size_t arenaSize = Rendering::MemArena::GetSize();
	const size_t lastFrameTotal = Rendering::MemArena::GetTotalMemoryAllocatedLastFrame();
	const size_t baselineUsage = lastFrameTotal > m_arenaBytesConsumedLastFrame ? lastFrameTotal - m_arenaBytesConsumedLastFrame : 0;

	const size_t arenaCapacity = arenaSize > k_arenaSafetyMarginBytes ? arenaSize - k_arenaSafetyMarginBytes : 0;
	const size_t arenaUsage = baselineUsage + m_idleArenaBytesConsumed;
	const size_t arenaBudget = arenaCapacity > arenaUsage ? arenaCapacity - arenaUsage : 0;
	const size_t uploadBudget = m_uploadBudgetBytes > m_idleUploadBytesConsumed ? m_uploadBudgetBytes - m_idleUploadBytesConsumed : 0;

	return AsyncFrameBudget(timeBudgetMS, uploadBudget, arenaBudget, arenaCapacity, isIdle);
}
