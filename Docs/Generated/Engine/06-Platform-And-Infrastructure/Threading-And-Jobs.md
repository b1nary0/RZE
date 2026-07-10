# Threading & Job System

`Engine\Src\EngineCore\Threading\JobSystem\` implements a small, fixed-size thread pool. It is early-stage — initialized/shut down alongside the engine, but not yet deeply wired into other subsystems.

## Types

- `Threading.h` — `MAX_WORKER_THREADS = 4`.
- `Threading::Job` (`Job.h/.cpp`) — wraps a `Functor<void>` task plus an `mOnFinished` completion-callback slot. `Run()` executes it.
- `Threading::WorkerThread` (`WorkerThread.h/.cpp`) — owns a `std::thread`, a local job queue, and idle/running/active flags. `Initialize()`/`ShutDown()`/`IsIdle()`/`IsRunning()`.
- `Threading::JobScheduler` (`JobScheduler.h/.cpp`) — **singleton** (`static JobScheduler& Get()`), owns a fixed array `WorkerThread mWorkerThreads[MAX_WORKER_THREADS]` and a shared `JobQueue` (`std::queue<Job>`) guarded by a static `std::mutex`. API: `PushJob(Task)`, `RequestJob(Job&)` (pull-model — idle workers request work rather than having it pushed to them), `Wait()`.

## Lifecycle

`JobScheduler::Get().Initialize()` is called from `RZE_Engine::Init()`; `JobScheduler::ShutDown()` is called from `RZE_Engine::BeginShutDown()`. See [Engine-Startup-And-Frame-Loop.md](../02-Architecture/Engine-Startup-And-Frame-Loop.md).

## Current usage

The one concrete consumer found in the codebase today is the **Editor's build/launch workflow**: `EditorApp::DisplayMenuBar()` pushes jobs onto `JobScheduler::Get()` for "Build Game..." / "Launch Game..." menu actions, which shell out via `_popen` to `BuildGame.bat` / the built `RZE_Game.exe`, streaming stdout into the Editor's `LogPanel` (see [Editor-Camera-And-Build-Launch.md](../07-Editor/Editor-Camera-And-Build-Launch.md)). There is no evidence in the explored source of asset loading, resource decompression, or rendering work being parallelized through this system yet — `ResourceHandler::LoadResource` remains a synchronous call ([Asset-Import-Runtime.md](../05-AssetPipeline/Asset-Import-Runtime.md)). Treat this as scaffolding for future parallel work rather than a load-bearing system today.

## Key files

- `Engine\Src\EngineCore\Threading\Threading.h`
- `Engine\Src\EngineCore\Threading\JobSystem\Job.h/.cpp`
- `Engine\Src\EngineCore\Threading\JobSystem\WorkerThread.h/.cpp`
- `Engine\Src\EngineCore\Threading\JobSystem\JobScheduler.h/.cpp`
