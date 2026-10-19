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
// show the group in its place. Each one is logged as it ends, so the group's history lists them all: a step with its
// label when it succeeds, "<label>: cancelled" when it's cancelled on its own, and a warning when it fails, after which
// the group still succeeds. Cancelling the group cancels those still running.
class AsyncOperationGroup final : public AsyncOperation
{
public:
	// flags are EAsyncOperationFlags
	AsyncOperationGroup(const std::string& name, U32 flags, EAsyncOperationKind kind);

	// Main thread only. Not once the group has finished; start another group for more. label is what its history calls
	// it, e.g. "lion.png"; the operation's name if it's empty.
	void Add(const std::shared_ptr<AsyncOperation>& operation, const std::string& label = std::string());

protected:
	void OnStart() override;
	void OnTick(AsyncFrameBudget& budget) override;
	void OnFinished(EAsyncOperationState finalState) override;

private:
	// Logs and counts those that have finished since last time, then sets the progress and status
	void Update();

private:
	struct Item
	{
		std::shared_ptr<AsyncOperation> Operation;
		std::string Label;
	};

	// Only until each finishes, so the group doesn't keep what they hold
	std::vector<Item> m_running;
	size_t m_doneCount = 0;
};
