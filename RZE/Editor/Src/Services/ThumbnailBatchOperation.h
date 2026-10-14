#pragma once

#include <EngineCore/Async/AsyncOperation.h>

#include <Utils/Platform/Filepath.h>

#include <functional>
#include <memory>
#include <string>

namespace Editor
{
	// Renders thumbnails one at a time, each through the same steps: set the item up, wait until it's ready, let it
	// settle for a few frames, capture it, wait for the file, then tidy up. A subclass says where the items come from
	// and how each is set up and captured; it can take more while the batch runs.
	//
	// Reports as it goes: "<item> (3 of 12)" with the progress, a logged step for each thumbnail written, and a warning
	// for each that couldn't be, after which the batch carries on. It finishes once there's nothing left.
	class ThumbnailBatchOperation : public AsyncOperation
	{
	public:
		// flags are EAsyncOperationFlags
		ThumbnailBatchOperation(const std::string& name, U32 flags);

	protected:
		// The next item, or false when there are none left, which finishes the batch
		virtual bool TakeNextItem(Filepath& outItem) = 0;
		// How many more are to come after the current one, for the progress
		virtual size_t GetItemsLeft() const = 0;

		// Sets the scene up for item, e.g. loads it
		virtual void BeginItem(const Filepath& item) = 0;
		// Whether the item is set up; it's captured once it has also settled for GetSettleFrames frames. FailItem from
		// here gives up on it.
		virtual bool IsItemReady() = 0;
		// Captures the item's thumbnail, calling onDone once, with the error if it wasn't written
		virtual void CaptureItem(const Filepath& item, std::function<void(bool succeeded, const std::string& error)> onDone) = 0;
		// After the item, whether or not it was written, e.g. removes what BeginItem added
		virtual void EndItem(const Filepath& item, bool succeeded) {}
		// Once the batch has finished: Succeeded when there was nothing left, or Cancelled
		virtual void EndBatch(EAsyncOperationState finalState) {}
		// Frames a ready item waits before its capture, for its first frames' work (e.g. uploads)
		virtual int GetSettleFrames() const { return 3; }

		// Gives up on the current item, as a warning; the batch carries on with the next
		void FailItem(const std::string& reason);

		size_t GetWrittenCount() const { return m_writtenCount; }

	private:
		void OnStart() override;
		void OnTick(AsyncFrameBudget& budget) override;
		void OnFinished(EAsyncOperationState finalState) override;

		void StartNextItem();
		void FinishItem(bool succeeded, const std::string& error);

	private:
		enum class EState
		{
			WaitingForItem,
			Settling,
			Capturing
		};

		// Written by a capture's callback, which may outlive the operation
		struct CaptureResult
		{
			bool IsDone = false;
			bool Succeeded = false;
			std::string Error;
		};

		Filepath m_item;
		bool m_hasItem = false;
		// Started, including the current one
		size_t m_itemsStarted = 0;
		size_t m_writtenCount = 0;
		size_t m_failedCount = 0;

		EState m_state = EState::WaitingForItem;
		int m_settleFramesLeft = 0;
		std::shared_ptr<CaptureResult> m_captureResult;
		// Set by FailItem
		bool m_hasItemFailed = false;
		std::string m_itemError;
	};
}
