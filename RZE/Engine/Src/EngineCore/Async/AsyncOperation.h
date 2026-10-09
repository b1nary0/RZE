#pragma once

#include <string>
#include <vector>

#include <EngineCore/Threading/Threading.h>
#include <EngineCore/Threading/CancellationToken.h>
#include <EngineCore/Threading/JobSystem/Job.h>

#include <Utils/Functor.h>
#include <Utils/PrimitiveDefs.h>

class AsyncFrameBudget;

enum class EAsyncOperationState : U8
{
	NotStarted,
	Running,
	Succeeded,
	Failed,
	Cancelled
};

namespace EAsyncOperationFlags
{
	enum T : U32
	{
		None = 0,
		// The operation should block user interaction (e.g. the editor shows a modal progress dialog).
		Blocking = 1 << 0
	};
}

// Base class for long-running work that is spread across worker threads and multiple frames.
// Operations are owned and driven by AsyncOperationManager.
//
// Threading contract:
//  - Query functions and SetProgress/SetStatusText/RequestCancel are safe from any thread.
//  - Everything else, including all virtual hooks and the completion callback, runs on the main thread.
//  - Worker jobs must be pushed through PushWorkerJob(). The manager will not finish or destroy an
//    operation while any of those jobs are outstanding, so jobs may safely capture `this`.
class AsyncOperation
{
	friend class AsyncOperationManager;

public:
	using CompletionCallback = Functor<void, const AsyncOperation&>;

public:
	AsyncOperation(const std::string& name, U32 flags);
	virtual ~AsyncOperation();

	AsyncOperation(const AsyncOperation&) = delete;
	AsyncOperation& operator=(const AsyncOperation&) = delete;

	// Thread-safe
public:
	const std::string& GetName() const { return m_name; }
	bool IsBlocking() const { return (m_flags & EAsyncOperationFlags::Blocking) != 0; }

	EAsyncOperationState GetState() const { return m_state.load(std::memory_order_acquire); }
	bool IsFinished() const;
	bool Succeeded() const { return GetState() == EAsyncOperationState::Succeeded; }

	float GetProgress() const { return m_progress.load(std::memory_order_relaxed); }
	std::string GetStatusText() const;
	std::string GetErrorText() const;

	const Threading::CancellationToken& GetCancellationToken() const { return m_cancellationToken; }
	bool IsCancellationRequested() const { return m_cancellationToken.IsCancellationRequested(); }

	// Progress is clamped to [0, 1] and never decreases.
	void SetProgress(float progress);
	void SetStatusText(const std::string& text);
	void SetErrorText(const std::string& text);

	void RequestCancel();

	// Main thread only
public:
	// Called exactly once when the operation reaches a terminal state. Not called if the engine shuts down first.
	void SetOnCompleted(CompletionCallback callback);

	Threading::JobHandle PushWorkerJob(Threading::Job::Task task);
	bool HasOutstandingJobs();

protected:
	// Called once when the operation is started.
	virtual void OnStart() = 0;

	// Called once per frame while running. Do bounded main-thread work here, respecting the budget,
	// and call Complete() or Fail() when done.
	virtual void OnTick(AsyncFrameBudget& budget) = 0;

	// Called once all outstanding jobs have finished, right before the completion callback.
	// Use it to roll back partial work on failure/cancellation.
	virtual void OnFinished(EAsyncOperationState finalState) {}

	// Called a few frames after finishing (or at shutdown) to release anything that must outlive
	// in-flight render thread work, such as texture data referenced by pending GPU uploads.
	virtual void OnCleanup() {}

	void Complete();
	// Also requests cancellation so that outstanding worker jobs can exit early.
	void Fail(const std::string& reason);

private:
	bool HasRequestedFinish() const { return m_requestedFinalState != EAsyncOperationState::NotStarted; }
	void Finish(EAsyncOperationState finalState);

private:
	std::string m_name;
	U32 m_flags;

	std::atomic<EAsyncOperationState> m_state{ EAsyncOperationState::NotStarted };
	std::atomic<float> m_progress{ 0.0f };

	mutable std::mutex m_textMutex;
	std::string m_statusText;
	std::string m_errorText;

	Threading::CancellationToken m_cancellationToken;

	// Main thread only
	std::vector<Threading::JobHandle> m_outstandingJobs;
	CompletionCallback m_onCompleted;
	EAsyncOperationState m_requestedFinalState = EAsyncOperationState::NotStarted;
};
