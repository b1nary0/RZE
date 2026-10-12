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

		// Index-based, since a command can push more
		for (size_t commandIndex = 0; commandIndex < m_pending.size(); ++commandIndex)
		{
			// Moved out first, so a push during Execute can't invalidate it
			const std::unique_ptr<EditorCommand> command = std::move(m_pending[commandIndex]);
			command->Execute(context);
		}

		const bool hasRun = !m_pending.empty();
		m_pending.clear();

		// Every command changes the scene; editor state like the selection isn't changed through commands
		if (hasRun)
		{
			context.GetEvents().Publish(SceneModifiedEvent{});
		}
	}

	void CommandQueue::Clear()
	{
		m_pending.clear();
	}
}
