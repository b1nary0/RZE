#pragma once

#include <Commands/EditorCommand.h>

#include <memory>
#include <utility>
#include <vector>

namespace Editor
{
	// Commands waiting to be applied. Owned by EditorContext and flushed once per frame, after every panel has
	// drawn. Main thread only.
	class CommandQueue
	{
	public:
		CommandQueue() = default;
		~CommandQueue() = default;

		CommandQueue(const CommandQueue&) = delete;
		CommandQueue& operator=(const CommandQueue&) = delete;

	public:
		template <typename TCommand, typename... Args>
		void Push(Args&&... args)
		{
			Push(std::make_unique<TCommand>(std::forward<Args>(args)...));
		}

		void Push(std::unique_ptr<EditorCommand> command);

		// Executes every pending command in the order they were pushed, including any pushed during the flush, then
		// publishes SceneModifiedEvent if any ran
		void Flush(EditorContext& context);

		// Drops pending commands without running them, e.g. when they'd target a scene that's being unloaded
		void Clear();

		bool IsEmpty() const { return m_pending.empty(); }

	private:
		std::vector<std::unique_ptr<EditorCommand>> m_pending;
	};
}
