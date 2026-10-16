#pragma once

#include <Utils/Platform/Filepath.h>

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
		// Builds on a worker thread, shown by the async operation UI. Ignored (with a log line) while a build is
		// already running.
		void Build();
		// Build, then launches the game into scene (a file in AssetPaths::GetSceneFolder) once it builds
		void BuildAndLaunch(const Filepath& scene);
		bool IsBuilding() const;

	private:
		// launchScene is only used when launchAfterBuild is set
		void StartBuild(bool launchAfterBuild, const Filepath& launchScene);

	private:
		std::shared_ptr<AsyncOperation> m_buildOperation;
	};
}
