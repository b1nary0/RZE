#include <StdAfx.h>
#include <EngineCore/Async/JobAsyncOperation.h>

#include <EngineCore/Threading/MainThreadDispatcher.h>

//
// AsyncWorkerContext
//
AsyncWorkerContext::AsyncWorkerContext(AsyncOperation& operation)
	: m_operation(operation)
{
}

bool AsyncWorkerContext::IsCancellationRequested() const
{
	return m_operation.IsCancellationRequested();
}

const Threading::CancellationToken& AsyncWorkerContext::GetCancellationToken() const
{
	return m_operation.GetCancellationToken();
}

void AsyncWorkerContext::SetProgress(float progress)
{
	m_operation.SetProgress(progress);
}

void AsyncWorkerContext::SetStatusText(const std::string& text)
{
	m_operation.SetStatusText(text);
}

void AsyncWorkerContext::SetErrorText(const std::string& text)
{
	m_operation.SetErrorText(text);
}

void AsyncWorkerContext::PostToMainThread(Threading::Job::Task task)
{
	Threading::MainThreadDispatcher::Get().Post(std::move(task));
}

//
// JobAsyncOperation
//
JobAsyncOperation::JobAsyncOperation(const std::string& name, U32 flags, WorkFunction work)
	: AsyncOperation(name, flags)
	, m_work(std::move(work))
{
}

void JobAsyncOperation::OnStart()
{
	m_jobHandle = PushWorkerJob(Threading::Job::Task([this]()
		{
			AsyncWorkerContext context(*this);
			m_workSucceeded = m_work(context);
		}));
}

void JobAsyncOperation::OnTick(AsyncFrameBudget& budget)
{
	if (!m_jobHandle.IsComplete())
	{
		return;
	}

	if (m_jobHandle.WasAbandoned())
	{
		Fail("Job was abandoned before it could run.");
	}
	else if (m_workSucceeded)
	{
		Complete();
	}
	else
	{
		const std::string errorText = GetErrorText();
		Fail(errorText.empty() ? "Operation failed." : errorText);
	}
}
