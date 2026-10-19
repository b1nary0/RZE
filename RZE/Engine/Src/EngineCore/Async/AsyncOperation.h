#pragma once

#include <chrono>
#include <optional>
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
		Blocking = 1 << 0,
		// Part of another operation that reports on it, e.g. the scene a thumbnail run loads. Progress displays can leave
		// these out (AsyncOperationFilter::ShowSubSteps).
		SubStep = 1 << 1,
		// Work nobody is waiting on, e.g. rendering thumbnails. Ticked after every foreground operation, with a small
		// budget of its own (AsyncOperationManager::k_backgroundFrameBudgetMS), and its worker jobs go to the
		// background lane (Threading::EJobLane). It keeps going while a Blocking operation runs.
		Background = 1 << 2
	};
}

// What an operation does, so a progress display can pick which kinds to show. Each is a bit of its own, so a set of
// kinds is a mask of them (AsyncOperationFilter::ShownKinds).
enum class EAsyncOperationKind : U32
{
	// Anything else, e.g. building the game
	General = 1 << 0,
	// Loading resources (ResourcePreloadOperation)
	ResourceLoad = 1 << 1,
	// Loading a scene (SceneLoadOperation)
	SceneLoad = 1 << 2,
	// Rendering images, e.g. thumbnails
	Rendering = 1 << 3,
	// Writing files
	Saving = 1 << 4
};

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
	AsyncOperation(const std::string& name, U32 flags, EAsyncOperationKind kind = EAsyncOperationKind::General);
	virtual ~AsyncOperation();

	AsyncOperation(const AsyncOperation&) = delete;
	AsyncOperation& operator=(const AsyncOperation&) = delete;

	// Thread-safe
public:
	const std::string& GetName() const { return m_name; }
	bool IsBlocking() const { return (m_flags & EAsyncOperationFlags::Blocking) != 0; }
	bool IsSubStep() const { return (m_flags & EAsyncOperationFlags::SubStep) != 0; }
	bool IsBackground() const { return (m_flags & EAsyncOperationFlags::Background) != 0; }
	EAsyncOperationKind GetKind() const { return m_kind; }

	EAsyncOperationState GetState() const { return m_state.load(std::memory_order_acquire); }
	bool IsFinished() const;
	bool Succeeded() const { return GetState() == EAsyncOperationState::Succeeded; }

	float GetProgress() const { return m_progress.load(std::memory_order_relaxed); }
	std::string GetStatusText() const;
	std::string GetErrorText() const;
	// Every warning added, a line each; empty if there are none
	std::string GetWarningText() const;
	bool HasWarnings() const;

	// One step in the operation's history (LogStep, AddWarning)
	struct LoggedStep
	{
		std::chrono::system_clock::time_point Time;
		std::string Text;
		bool IsWarning = false;
	};
	// The steps logged from first on, oldest first
	std::vector<LoggedStep> GetLoggedSteps(size_t first = 0) const;

	const Threading::CancellationToken& GetCancellationToken() const { return m_cancellationToken; }
	bool IsCancellationRequested() const { return m_cancellationToken.IsCancellationRequested(); }

	// Progress is clamped to [0, 1] and never decreases.
	void SetProgress(float progress);
	void SetStatusText(const std::string& text);
	void SetErrorText(const std::string& text);
	// Something worth knowing that didn't stop the operation, e.g. one item of many that couldn't be done. It can still
	// succeed.
	void AddWarning(const std::string& text);
	// A step worth keeping in the operation's history, e.g. one item of a batch done. Warnings are logged too.
	void LogStep(const std::string& text);

	void RequestCancel();

	// Main thread only
public:
	// Called exactly once when the operation reaches a terminal state. Not called if the engine shuts down first.
	void SetOnCompleted(CompletionCallback callback);

	// How long it's been running: 0 before it starts, and how long it took once it's finished
	double GetSecondsRunning() const;
	// How long it has left at the rate it's gone so far, if it's gone on long enough to tell (see the static overload)
	std::optional<double> EstimateSecondsLeft() const;

	// How long something that's this far through after this long has left, if it's gone on long enough to tell: at least
	// k_minProgressToEstimate done, and a second
	static std::optional<double> EstimateSecondsLeft(double secondsRunning, float progress);
	static constexpr float k_minProgressToEstimate = 0.05f;

	Threading::JobHandle PushWorkerJob(Threading::Job::Task task);
	bool HasOutstandingJobs();

protected:
	// Called once when the operation is started.
	virtual void OnStart() = 0;

	// Called once per frame while running. Do bounded main-thread work here, respecting the budget,
	// and call Complete() or Fail() when done. When the frame has spare time it may be called again in the same frame,
	// with budget.IsIdle() true; count frames only on ticks where it's false.
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
	EAsyncOperationKind m_kind;

	std::atomic<EAsyncOperationState> m_state{ EAsyncOperationState::NotStarted };
	std::atomic<float> m_progress{ 0.0f };

	mutable std::mutex m_textMutex;
	std::string m_statusText;
	std::string m_errorText;
	std::string m_warningText;
	std::vector<LoggedStep> m_loggedSteps;

	Threading::CancellationToken m_cancellationToken;

	// Main thread only
	std::vector<Threading::JobHandle> m_outstandingJobs;
	CompletionCallback m_onCompleted;
	// Set by AsyncOperationManager when it starts the operation, and by Finish
	std::chrono::steady_clock::time_point m_startTime;
	std::chrono::steady_clock::time_point m_finishTime;
	EAsyncOperationState m_requestedFinalState = EAsyncOperationState::NotStarted;
};
