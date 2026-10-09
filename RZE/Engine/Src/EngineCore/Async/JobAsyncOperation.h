#pragma once

#include <EngineCore/Async/AsyncOperation.h>

// Thread-safe view of an operation handed to worker-side code.
class AsyncWorkerContext
{
public:
	explicit AsyncWorkerContext(AsyncOperation& operation);

public:
	bool IsCancellationRequested() const;
	const Threading::CancellationToken& GetCancellationToken() const;

	void SetProgress(float progress);
	void SetStatusText(const std::string& text);
	void SetErrorText(const std::string& text);

	// Runs the task on the main thread at the start of a following frame.
	void PostToMainThread(Threading::Job::Task task);

private:
	AsyncOperation& m_operation;
};

// An operation that runs a single function on a worker thread. The function returns true on success;
// on failure it should describe the problem via AsyncWorkerContext::SetErrorText.
class JobAsyncOperation final : public AsyncOperation
{
public:
	using WorkFunction = Functor<bool, AsyncWorkerContext&>;

public:
	JobAsyncOperation(const std::string& name, U32 flags, WorkFunction work);

protected:
	void OnStart() override;
	void OnTick(AsyncFrameBudget& budget) override;

private:
	WorkFunction m_work;
	Threading::JobHandle m_jobHandle;

	// Written by the worker, read on the main thread after m_jobHandle reports completion.
	bool m_workSucceeded = false;
};
