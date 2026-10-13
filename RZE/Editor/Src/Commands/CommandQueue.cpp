#include <Commands/CommandQueue.h>

#include <UI/Framework/EditorContext.h>

#include <EngineCore/Threading/MainThreadDispatcher.h>

#include <Utils/DebugUtils/Debug.h>

namespace Editor
{
	void CommandQueue::Push(std::unique_ptr<EditorCommand> command)
	{
		AssertExpr(Threading::MainThreadDispatcher::Get().IsMainThread());
		AssertNotNull(command);

		m_pending.push_back(std::move(command));
	}

	void CommandQueue::Flush(EditorContext& context)
	{
		AssertExpr(Threading::MainThreadDispatcher::Get().IsMainThread());

		const SceneEditScope sceneEdit(*this);

		bool hasChanged = false;

		// Index-based, since a command can push more
		for (size_t commandIndex = 0; commandIndex < m_pending.size(); ++commandIndex)
		{
			// Moved out first, so a push during Execute can't invalidate it
			const std::unique_ptr<EditorCommand> command = std::move(m_pending[commandIndex]);
			std::vector<ObjectGUID> selectionBefore = context.GetSelection().GetGUIDs();
			if (!command->Execute(context))
			{
				continue;
			}

			hasChanged = true;
			UndoHistory& history = context.GetUndoHistory();
			if (std::unique_ptr<UndoRecord> record = command->TakeUndoRecord())
			{
				history.Record(command->GetInteraction(), std::move(record), std::move(selectionBefore), context.GetSelection().GetGUIDs());
			}
			else if (!command->IsHistoryNavigation())
			{
				history.MarkUnrecordedChange();
			}
		}

		m_pending.clear();

		if (hasChanged)
		{
			context.GetEvents().Publish(SceneModifiedEvent{});
		}
	}

	void CommandQueue::Clear()
	{
		m_pending.clear();
	}
}
