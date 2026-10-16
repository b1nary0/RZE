#pragma once

#include <memory>

class AsyncOperation;

namespace Editor
{
	// Builds and launches the game through BuildGame.bat, tracing its output to the Build log channel.
	// Owned by EditorContext.
	class GameBuilder
	{
	public:
		GameBuilder() = default;
		~GameBuilder() = default;

	public:
		// Builds on a worker thread, shown by the async operation UI, and launches the game once it builds if asked to.
		// Ignored (with a log line) while a build is already running.
		void Build(bool launchAfterBuild);
		bool IsBuilding() const;

	private:
		std::shared_ptr<AsyncOperation> m_buildOperation;
	};
}
