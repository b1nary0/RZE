#include <Services/GameBuilder.h>

#include <RZE.h>

#include <Asset/AssetPaths.h>

#include <EngineCore/Async/AsyncOperationManager.h>
#include <EngineCore/Async/JobAsyncOperation.h>
#include <EngineCore/Threading/CancellationToken.h>

#include <DebugUtils/DebugServices.h>

#include <Utils/Conversions.h>
#include <Utils/Functor.h>
#include <Utils/Platform/Filepath.h>

#include <process.h>
#include <Windows.h>

namespace
{
	using OutputLineSink = Functor<void, const std::string&>;

	void ForwardLine(std::string line, const OutputLineSink& onOutputLine)
	{
		while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
		{
			line.pop_back();
		}

		onOutputLine(line);
	}

	// Runs a process through cmd on the calling thread and forwards each line of its stdout and stderr (without the
	// trailing newline). Stops reading early if cancellation is requested. Returns the process exit code, or -1 if it
	// couldn't be started. Not _popen: the editor has no console, so cmd would open a console window that takes focus.
	int RunProcessStreamed(const Filepath& processPath, const OutputLineSink& onOutputLine, const Threading::CancellationToken* cancellationToken)
	{
		SECURITY_ATTRIBUTES pipeAttributes = {};
		pipeAttributes.nLength = sizeof(pipeAttributes);
		pipeAttributes.bInheritHandle = TRUE;

		HANDLE readPipe = nullptr;
		HANDLE writePipe = nullptr;
		if (!CreatePipe(&readPipe, &writePipe, &pipeAttributes, 0))
		{
			onOutputLine("Failed to start process: " + processPath.GetRelativePath());
			return -1;
		}
		// Only the child's end is inherited
		SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

		STARTUPINFOW startupInfo = {};
		startupInfo.cb = sizeof(startupInfo);
		startupInfo.dwFlags = STARTF_USESTDHANDLES;
		startupInfo.hStdInput = nullptr;
		startupInfo.hStdOutput = writePipe;
		startupInfo.hStdError = writePipe;

		// /s takes the outer quotes off, so a path with spaces stays one argument
		std::wstring commandLine = L"cmd.exe /s /c \"\"" + Conversions::StringToWString(processPath.GetAbsolutePath()) + L"\"\"";

		PROCESS_INFORMATION processInfo = {};
		const BOOL bStarted = CreateProcessW(nullptr, commandLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo);
		// Closed here so the read below ends once the child (and anything it started) exits
		CloseHandle(writePipe);
		if (!bStarted)
		{
			CloseHandle(readPipe);
			onOutputLine("Failed to start process: " + processPath.GetRelativePath());
			return -1;
		}

		std::string pending;
		char buffer[2048];
		DWORD bytesRead = 0;
		bool bCancelled = false;
		while (!bCancelled && ReadFile(readPipe, buffer, sizeof(buffer), &bytesRead, nullptr) && bytesRead > 0)
		{
			pending.append(buffer, bytesRead);

			size_t lineEnd;
			while ((lineEnd = pending.find('\n')) != std::string::npos)
			{
				ForwardLine(pending.substr(0, lineEnd), onOutputLine);
				pending.erase(0, lineEnd + 1);

				if (cancellationToken != nullptr && cancellationToken->IsCancellationRequested())
				{
					bCancelled = true;
					break;
				}
			}
		}

		if (!bCancelled && !pending.empty())
		{
			ForwardLine(pending, onOutputLine);
		}

		// Like _pclose: stop reading, then wait for the process to exit
		CloseHandle(readPipe);
		WaitForSingleObject(processInfo.hProcess, INFINITE);

		DWORD exitCode = 0;
		GetExitCodeProcess(processInfo.hProcess, &exitCode);
		CloseHandle(processInfo.hThread);
		CloseHandle(processInfo.hProcess);

		return static_cast<int>(exitCode);
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
