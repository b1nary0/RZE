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

		// Executes every pending command in the order they were pushed, including any pushed during the flush, and
		// records what each did in the context's UndoHistory. Publishes SceneModifiedEvent if any changed the scene.
		void Flush(EditorContext& context);

		// Drops pending commands without running them, e.g. when they'd target a scene that's being unloaded
		void Clear();

		bool IsEmpty() const { return m_pending.empty(); }

		// Opened while commands run, and by scene setup that isn't an edit (the editor camera). Adding or removing scene
		// objects while none is open, other than during a load, asserts in Debug (EditorApp), since undo only sees
		// changes made through commands.
		class SceneEditScope
		{
		public:
			explicit SceneEditScope(CommandQueue& commands) : m_commands(commands) { ++m_commands.m_sceneEditDepth; }
			~SceneEditScope() { --m_commands.m_sceneEditDepth; }
			SceneEditScope(const SceneEditScope&) = delete;
			SceneEditScope& operator=(const SceneEditScope&) = delete;

		private:
			CommandQueue& m_commands;
		};

		bool IsSceneEditOpen() const { return m_sceneEditDepth > 0; }

	private:
		std::vector<std::unique_ptr<EditorCommand>> m_pending;
		int m_sceneEditDepth = 0;
	};
}
