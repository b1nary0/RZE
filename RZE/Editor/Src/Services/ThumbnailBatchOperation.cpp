#include <Services/ThumbnailBatchOperation.h>

namespace Editor
{
	ThumbnailBatchOperation::ThumbnailBatchOperation(const std::string& name, U32 flags)
		: AsyncOperation(name, flags)
	{
	}

	void ThumbnailBatchOperation::FailItem(const std::string& reason)
	{
		m_hasItemFailed = true;
		m_itemError = reason;
	}

	void ThumbnailBatchOperation::OnStart()
	{
		StartNextItem();
	}

	void ThumbnailBatchOperation::OnTick(AsyncFrameBudget& budget)
	{
		switch (m_state)
		{
		case EState::WaitingForItem:
		{
			const bool isReady = IsItemReady();
			if (m_hasItemFailed)
			{
				FinishItem(false, m_itemError);
			}
			else if (isReady)
			{
				m_settleFramesLeft = GetSettleFrames();
				m_state = EState::Settling;
			}
			break;
		}

		case EState::Settling:
		{
			if (m_settleFramesLeft-- > 0)
			{
				break;
			}

			m_captureResult = std::make_shared<CaptureResult>();
			std::shared_ptr<CaptureResult> result = m_captureResult;
			CaptureItem(m_item, [result](bool succeeded, const std::string& error)
				{
					result->IsDone = true;
					result->Succeeded = succeeded;
					result->Error = error;
				});
			m_state = EState::Capturing;
			break;
		}

		case EState::Capturing:
		{
			if (m_captureResult->IsDone)
			{
				FinishItem(m_captureResult->Succeeded, m_captureResult->Error);
			}
			break;
		}
		}
	}

	void ThumbnailBatchOperation::OnFinished(EAsyncOperationState finalState)
	{
		if (m_hasItem)
		{
			EndItem(m_item, false);
			m_hasItem = false;
		}
		EndBatch(finalState);
	}

	void ThumbnailBatchOperation::FinishItem(bool succeeded, const std::string& error)
	{
		if (succeeded)
		{
			++m_writtenCount;
			LogStep("Rendered " + m_item.GetStem());
		}
		else
		{
			++m_failedCount;
			AddWarning("Couldn't render " + m_item.GetStem() + (error.empty() ? "" : ": " + error));
		}

		const Filepath item = m_item;
		m_hasItem = false;
		EndItem(item, succeeded);
		StartNextItem();
	}

	void ThumbnailBatchOperation::StartNextItem()
	{
		Filepath item;
		if (!TakeNextItem(item))
		{
			SetProgress(1.0f);
			SetStatusText(std::to_string(m_writtenCount) + " rendered" + (m_failedCount > 0 ? ", " + std::to_string(m_failedCount) + " couldn't be" : ""));
			Complete();
			return;
		}

		++m_itemsStarted;
		const size_t total = m_itemsStarted + GetItemsLeft();
		SetProgress(static_cast<float>(m_itemsStarted - 1) / static_cast<float>(total));
		SetStatusText(item.GetStem() + " (" + std::to_string(m_itemsStarted) + " of " + std::to_string(total) + ")");

		m_item = item;
		m_hasItem = true;
		m_hasItemFailed = false;
		m_itemError.clear();
		m_state = EState::WaitingForItem;
		BeginItem(item);
	}
}
