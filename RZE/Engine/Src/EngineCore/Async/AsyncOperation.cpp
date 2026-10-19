#include <StdAfx.h>
#include <EngineCore/Async/AsyncOperation.h>

#include <EngineCore/Threading/MainThreadDispatcher.h>
#include <EngineCore/Threading/JobSystem/JobScheduler.h>

#include <algorithm>

AsyncOperation::AsyncOperation(const std::string& name, U32 flags, EAsyncOperationKind kind)
	: m_name(name)
	, m_flags(flags)
	, m_kind(kind)
{
}

AsyncOperation::~AsyncOperation()
{
	AssertMsg(m_outstandingJobs.empty() || std::all_of(m_outstandingJobs.begin(), m_outstandingJobs.end(),
		[](const Threading::JobHandle& handle) { return handle.IsComplete(); }),
		"AsyncOperation destroyed while worker jobs are still running.");
}

bool AsyncOperation::IsFinished() const
{
	const EAsyncOperationState state = GetState();
	return state == EAsyncOperationState::Succeeded
		|| state == EAsyncOperationState::Failed
		|| state == EAsyncOperationState::Cancelled;
}

std::string AsyncOperation::GetStatusText() const
{
	std::lock_guard<std::mutex> lock(m_textMutex);
	return m_statusText;
}

std::string AsyncOperation::GetErrorText() const
{
	std::lock_guard<std::mutex> lock(m_textMutex);
	return m_errorText;
}

std::string AsyncOperation::GetWarningText() const
{
	std::lock_guard<std::mutex> lock(m_textMutex);
	return m_warningText;
}

bool AsyncOperation::HasWarnings() const
{
	std::lock_guard<std::mutex> lock(m_textMutex);
	return !m_warningText.empty();
}

std::vector<AsyncOperation::LoggedStep> AsyncOperation::GetLoggedSteps(size_t first) const
{
	std::lock_guard<std::mutex> lock(m_textMutex);
	if (first >= m_loggedSteps.size())
	{
		return {};
	}
	return std::vector<LoggedStep>(m_loggedSteps.begin() + first, m_loggedSteps.end());
}

void AsyncOperation::SetProgress(float progress)
{
	const float clamped = std::min(1.0f, std::max(0.0f, progress));

	float current = m_progress.load(std::memory_order_relaxed);
	while (clamped > current && !m_progress.compare_exchange_weak(current, clamped, std::memory_order_relaxed))
	{
	}
}

void AsyncOperation::SetStatusText(const std::string& text)
{
	std::lock_guard<std::mutex> lock(m_textMutex);
	m_statusText = text;
}

void AsyncOperation::SetErrorText(const std::string& text)
{
	std::lock_guard<std::mutex> lock(m_textMutex);
	m_errorText = text;
}

void AsyncOperation::AddWarning(const std::string& text)
{
	std::lock_guard<std::mutex> lock(m_textMutex);
	if (!m_warningText.empty())
	{
		m_warningText += '\n';
	}
	m_warningText += text;
	m_loggedSteps.push_back({ std::chrono::system_clock::now(), text, true });
}

void AsyncOperation::LogStep(const std::string& text)
{
	std::lock_guard<std::mutex> lock(m_textMutex);
	m_loggedSteps.push_back({ std::chrono::system_clock::now(), text, false });
}

void AsyncOperation::RequestCancel()
{
	m_cancellationToken.RequestCancel();
}

void AsyncOperation::SetOnCompleted(CompletionCallback callback)
{
	AssertExpr(Threading::MainThreadDispatcher::Get().IsMainThread());
	m_onCompleted = callback;
}

Threading::JobHandle AsyncOperation::PushWorkerJob(Threading::Job::Task task)
{
	AssertExpr(Threading::MainThreadDispatcher::Get().IsMainThread());

	Threading::JobHandle handle = Threading::JobScheduler::Get().PushJob(std::move(task), IsBackground() ? Threading::EJobLane::Background : Threading::EJobLane::Foreground);
	m_outstandingJobs.push_back(handle);

	return handle;
}

bool AsyncOperation::HasOutstandingJobs()
{
	m_outstandingJobs.erase(
		std::remove_if(m_outstandingJobs.begin(), m_outstandingJobs.end(),
			[](const Threading::JobHandle& handle) { return handle.IsComplete(); }),
		m_outstandingJobs.end());

	return !m_outstandingJobs.empty();
}

void AsyncOperation::Complete()
{
	if (!HasRequestedFinish())
	{
		m_requestedFinalState = EAsyncOperationState::Succeeded;
	}
}

void AsyncOperation::Fail(const std::string& reason)
{
	if (!HasRequestedFinish())
	{
		SetErrorText(reason);
		m_requestedFinalState = EAsyncOperationState::Failed;
		m_cancellationToken.RequestCancel();
	}
}

double AsyncOperation::GetSecondsRunning() const
{
	const EAsyncOperationState state = GetState();
	if (state == EAsyncOperationState::NotStarted)
	{
		return 0.0;
	}

	const std::chrono::steady_clock::time_point end = IsFinished() ? m_finishTime : std::chrono::steady_clock::now();
	return std::chrono::duration<double>(end - m_startTime).count();
}

std::optional<double> AsyncOperation::EstimateSecondsLeft() const
{
	return EstimateSecondsLeft(GetSecondsRunning(), GetProgress());
}

std::optional<double> AsyncOperation::EstimateSecondsLeft(double secondsRunning, float progress)
{
	if (progress < k_minProgressToEstimate || secondsRunning < 1.0)
	{
		return std::nullopt;
	}
	if (progress >= 1.0f)
	{
		return 0.0;
	}
	// At the rate it's gone so far
	return secondsRunning * (1.0 - progress) / progress;
}

void AsyncOperation::Finish(EAsyncOperationState finalState)
{
	AssertExpr(!IsFinished());
	m_finishTime = std::chrono::steady_clock::now();

	if (finalState == EAsyncOperationState::Succeeded)
	{
		SetProgress(1.0f);
	}

	OnFinished(finalState);

	m_state.store(finalState, std::memory_order_release);

	if (m_onCompleted != nullptr)
	{
		m_onCompleted(*this);
	}
}
