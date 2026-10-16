#pragma once

#include <memory>
#include <vector>

#include <EngineCore/Async/AsyncOperation.h>

#include <Utils/PrimitiveDefs.h>

class AsyncFrameBudget;

// Owns and drives every in-flight AsyncOperation. Main thread only.
//
// Each frame, foreground operations are ticked first, oldest first, sharing one budget. Background operations
// (EAsyncOperationFlags::Background) are ticked after them with a smaller budget of their own and whatever render
// command memory the foreground left, so they always make some progress but never take much of a frame.
//
// When a frame has spare time before the next one is due, TickIdle() gives the running operations some of it, foreground
// first. It runs after the frame is handed to the renderer, so what it records goes out with the next frame, and that
// frame's Tick() gets what's left of its allowance.
//
// Lifecycle of an operation:
//   Start() -> OnStart()
//   Tick()  -> OnTick(budget) each frame until the operation calls Complete()/Fail() or is cancelled
//           -> once no worker jobs are outstanding: OnFinished(state), completion callback
//           -> k_retireFrameCount frames later: OnCleanup(), operation released
class AsyncOperationManager
{
public:
	static constexpr double k_defaultFrameBudgetMS = 8.0;
	// Background operations' main-thread time each frame, on top of the foreground's
	static constexpr double k_backgroundFrameBudgetMS = 2.0;
	// Background operations get this fraction of the upload allowance, on top of the foreground's
	static constexpr size_t k_backgroundUploadBudgetDivisor = 4;
	// Render thread processes the previous frame's commands while the main thread builds the next one,
	// so anything referenced by a submitted command must survive at least this many frames.
	static constexpr U32 k_retireFrameCount = 2;

public:
	AsyncOperationManager();
	~AsyncOperationManager();

public:
	void Initialize();

	// Cancels all operations and blocks until their worker jobs have finished. Completion callbacks are not fired.
	void ShutDown();

	// Once per frame, before the frame is built
	void Tick();
	// With the frame's spare time, after it's handed to the renderer: ticks running operations again with an idle budget
	// of timeMS (AsyncFrameBudget::IsIdle). Operations only get more time here; finishing them, their callbacks and
	// starting new ones stay in Tick().
	void TickIdle(double timeMS);

public:
	void Start(const std::shared_ptr<AsyncOperation>& operation);

	bool HasBlockingOperation() const;
	// The oldest running operation flagged as Blocking, or nullptr.
	const AsyncOperation* GetBlockingOperation() const;
	// The running operations that aren't Hidden, oldest first, e.g. for a progress display. Shared so the caller can
	// keep watching one after it finishes, or cancel it.
	std::vector<std::shared_ptr<AsyncOperation>> GetVisibleOperations() const;

	// Main-thread time and GPU upload allowance shared by the foreground operations each frame. Background operations
	// get k_backgroundFrameBudgetMS and a share of the uploads on top.
	void SetFrameBudget(double timeBudgetMS, size_t uploadBudgetBytes);

private:
	struct Entry
	{
		std::shared_ptr<AsyncOperation> Operation;
		bool IsFinished = false;
		U32 RetireFramesRemaining = 0;
	};

	// The render command arena and uploads left for this frame, less what idle ticks before it used
	AsyncFrameBudget CreateFrameBudget(double timeBudgetMS, bool isIdle) const;
	// Ticks the foreground or the background operations, removing those that are done
	void TickEntries(bool background, AsyncFrameBudget& budget);
	// Ticks the foreground or the background operations that are still running, and nothing else
	void TickRunningEntries(bool background, AsyncFrameBudget& budget);
	void StartInternal(const std::shared_ptr<AsyncOperation>& operation);
	// Returns true when the entry can be removed.
	bool TickEntry(Entry& entry, AsyncFrameBudget& budget);

private:
	std::vector<Entry> m_entries;
	std::vector<std::shared_ptr<AsyncOperation>> m_pendingStarts;

	double m_frameBudgetMS;
	size_t m_uploadBudgetBytes;
	size_t m_arenaBytesConsumedLastFrame = 0;
	// What idle ticks used since the last Tick(), all of it part of the frame that Tick() starts
	size_t m_idleArenaBytesConsumed = 0;
	size_t m_idleUploadBytesConsumed = 0;

	bool m_isTicking = false;
	bool m_isShutDown = false;
};
