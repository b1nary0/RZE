#include <Services/GameBuilder.h>

#include <RZE.h>

#include <Asset/AssetPaths.h>

#include <EngineCore/Async/AsyncOperationManager.h>
#include <EngineCore/Async/JobAsyncOperation.h>
#include <EngineCore/Threading/CancellationToken.h>

#include <DebugUtils/DebugServices.h>

#include <Utils/Functor.h>
#include <Utils/Platform/Filepath.h>

#include <process.h>
#include <stdio.h>

namespace
{
	using OutputLineSink = Functor<void, const std::string&>;

	// Runs a process on the calling thread and forwards each line of its stdout (without the trailing newline).
	// Stops reading early if cancellation is requested. Returns the process exit code, or -1 if it couldn't be started.
	int RunProcessStreamed(const Filepath& processPath, const OutputLineSink& onOutputLine, const Threading::CancellationToken* cancellationToken)
	{
		FILE* pipe = _popen(processPath.GetAbsolutePath().c_str(), "rt");
		if (pipe == nullptr)
		{
			onOutputLine("Failed to start process: " + processPath.GetRelativePath());
			return -1;
		}

		char buffer[2048];
		while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
		{
			std::string line(buffer);
			while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
			{
				line.pop_back();
			}

			onOutputLine(line);

			if (cancellationToken != nullptr && cancellationToken->IsCancellationRequested())
			{
				break;
			}
		}

		return _pclose(pipe);
	}

	// Traces each line to the editor's build log channel. Safe from worker threads; lines are marshalled to the main thread.
	OutputLineSink MakeWorkerBuildTraceSink(AsyncWorkerContext& context)
	{
		return OutputLineSink([&context](const std::string& line)
			{
				context.PostToMainThread(Threading::Job::Task([line]()
					{
						DebugServices::Get().Trace(LogChannel::Build, line);
					}));
			});
	}
}

namespace Editor
{
	void GameBuilder::Build()
	{
		StartBuild(false, Filepath());
	}

	void GameBuilder::BuildAndLaunch(const Filepath& scene)
	{
		StartBuild(true, scene);
	}

	void GameBuilder::StartBuild(bool launchAfterBuild, const Filepath& launchScene)
	{
		if (IsBuilding())
		{
			DebugServices::Get().Trace(LogChannel::Build, "A build is already in progress.");
			return;
		}

		// Worked out here, so a bad scene is reported before the build rather than after it
		std::string sceneArgument;
		if (launchAfterBuild)
		{
			const std::string sceneName = AssetPaths::GetSceneName(launchScene);
			if (sceneName.empty())
			{
				DebugServices::Get().Trace(LogChannel::Build, "Can't launch into " + launchScene.GetRelativePath() + ": the game only loads scenes in " + AssetPaths::GetSceneFolder().GetRelativePath() + ".");
				return;
			}

			// Quoted, since _spawnl joins its arguments with spaces and the name may have some
			sceneArgument = "\"" + sceneName + "\"";
			DebugServices::Get().Trace(LogChannel::Build, "Building and launching game into scene " + launchScene.GetRelativePath() + "...");
		}
		else
		{
			DebugServices::Get().Trace(LogChannel::Build, "Building Game...");
		}

		JobAsyncOperation::WorkFunction work([launchAfterBuild, sceneArgument](AsyncWorkerContext& context) -> bool
			{
				static const Filepath buildGameBat("BuildGame.bat");
				static const Filepath gamePath("_build\\debug\\Game.exe");

				const OutputLineSink traceLine = MakeWorkerBuildTraceSink(context);

				context.SetStatusText("Building game...");
				const int buildResult = RunProcessStreamed(buildGameBat, traceLine, &context.GetCancellationToken());
				if (buildResult != 0)
				{
					context.SetErrorText("Build failed with exit code " + std::to_string(buildResult) + ".");
					return false;
				}

				if (!launchAfterBuild || context.IsCancellationRequested())
				{
					return true;
				}

				context.SetStatusText("Launching game...");
				const std::string gameExePath = gamePath.GetAbsolutePath();
				const std::string quotedGameExePath = "\"" + gameExePath + "\"";
				if (_spawnl(_P_NOWAIT, gameExePath.c_str(), quotedGameExePath.c_str(), "-scene", sceneArgument.c_str(), nullptr) == -1)
				{
					context.SetErrorText("Failed to launch " + gameExePath + ".");
					return false;
				}

				return true;
			});

		m_buildOperation = std::make_shared<JobAsyncOperation>(launchAfterBuild ? "Build and Launch Game" : "Build Game", EAsyncOperationFlags::None, work);
		m_buildOperation->SetOnCompleted(AsyncOperation::CompletionCallback([](const AsyncOperation& operation)
			{
				if (operation.Succeeded())
				{
					DebugServices::Get().Trace(LogChannel::Build, operation.GetName() + " succeeded.");
				}
				else
				{
					DebugServices::Get().Trace(LogChannel::Build, operation.GetName() + " failed: " + operation.GetErrorText());
				}
			}));

		RZE().GetAsyncOperationManager().Start(m_buildOperation);
	}

	bool GameBuilder::IsBuilding() const
	{
		return m_buildOperation != nullptr && !m_buildOperation->IsFinished();
	}
}
