#pragma once

#include <EngineCore/Async/AsyncOperation.h>

#include <memory>
#include <string>
#include <vector>

// Reports on several operations as one, e.g. a folder's thumbnails loading an image at a time. Its progress is how far
// they've got between them, its status how many are done and the one it's waiting on, and it finishes once they all
// have. Operations can be added while it runs, until it finishes.
//
// It doesn't start them: whoever adds one starts it too, flagged EAsyncOperationFlags::SubStep so progress displays can
// show the group in its place. One that fails is a warning on the group, which still succeeds. One cancelled on its own
// just counts as done; cancelling the group cancels those still running.
class AsyncOperationGroup final : public AsyncOperation
{
public:
	// flags are EAsyncOperationFlags
	AsyncOperationGroup(const std::string& name, U32 flags, EAsyncOperationKind kind);

	// Main thread only. Not once the group has finished; start another group for more.
	void Add(const std::shared_ptr<AsyncOperation>& operation);

protected:
	void OnStart() override;
	void OnTick(AsyncFrameBudget& budget) override;
	void OnFinished(EAsyncOperationState finalState) override;

private:
	// Counts those that have finished since last time, then sets the progress and status
	void Update();

private:
	// Only until each finishes, so the group doesn't keep what they hold
	std::vector<std::shared_ptr<AsyncOperation>> m_running;
	size_t m_doneCount = 0;
};
