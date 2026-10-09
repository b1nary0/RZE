#pragma once

#include <memory>
#include <vector>

#include <EngineCore/Async/AsyncOperation.h>

#include <Utils/PrimitiveDefs.h>

class AsyncFrameBudget;

// Owns and drives every in-flight AsyncOperation. Main thread only.
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

	void Tick();

public:
	void Start(const std::shared_ptr<AsyncOperation>& operation);

	bool HasBlockingOperation() const;
	// The oldest running operation flagged as Blocking, or nullptr.
	const AsyncOperation* GetBlockingOperation() const;

	// Main-thread time and GPU upload allowance shared by all operations each frame.
	void SetFrameBudget(double timeBudgetMS, size_t uploadBudgetBytes);

private:
	struct Entry
	{
		std::shared_ptr<AsyncOperation> Operation;
		bool IsFinished = false;
		U32 RetireFramesRemaining = 0;
	};

	AsyncFrameBudget CreateFrameBudget() const;
	void StartInternal(const std::shared_ptr<AsyncOperation>& operation);
	// Returns true when the entry can be removed.
	bool TickEntry(Entry& entry, AsyncFrameBudget& budget);

private:
	std::vector<Entry> m_entries;
	std::vector<std::shared_ptr<AsyncOperation>> m_pendingStarts;

	double m_frameBudgetMS;
	size_t m_uploadBudgetBytes;
	size_t m_arenaBytesConsumedLastFrame = 0;

	bool m_isTicking = false;
	bool m_isShutDown = false;
};
