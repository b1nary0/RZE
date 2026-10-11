# Threading & Job System

RZE runs on a few kinds of threads:

```mermaid
%%{init: {'flowchart': {'subGraphTitleMargin': {'top': 6, 'bottom': 10}}, 'themeCSS': '.labelBkg, .edgeLabel { background-color: var(--background-primary) !important; } .messageText, .labelText, .loopText { paint-order: stroke; stroke: var(--background-primary); stroke-width: 5px; stroke-linejoin: round; }'}}%%
flowchart TD
    subgraph Main["Main thread, each frame"]
        D["Step 1<br/>MainThreadDispatcher::Drain()<br/>runs tasks posted by workers"]
        U["Step 2<br/>AsyncOperationManager::Tick()<br/>then app and scene Update"]
        R["Step 3<br/>RenderEngine queues<br/>this frame's draw commands"]
        D --> U --> R
    end
    J["Worker threads (4)<br/>JobScheduler jobs:<br/>file I/O, parsing, builds"]
    X["Render thread<br/>runs last frame's<br/>commands on D3D11"]
    U -->|"PushJob"| J
    J -.->|"Post(task)"| D
    R -->|"DevicePresent"| X
```

- The **main thread** owns the engine: scenes, components, `ResourceHandler`, ImGui and render submission. Most engine state is main-thread only.
- **Worker threads** do work that doesn't touch engine state: reading and parsing files, decoding textures, running build scripts.
- The **render thread** owns the D3D11 device and executes queued render commands (see [Low-Level-Renderer-Module.md](../04-Rendering/Low-Level-Renderer-Module.md)).

## The job system (`Engine\Src\EngineCore\Threading\JobSystem\`)

A fixed pool of `MAX_WORKER_THREADS = 4` workers pulling from one shared queue.

| Type | What it is |
|---|---|
| `JobScheduler` | Singleton (`JobScheduler::Get()`). `PushJob(task)` from any thread returns a `JobHandle` (ignore it for fire-and-forget). `Wait(handle)` blocks until a job is done; `WaitForIdle()` until the queue is empty and nothing is running. |
| `Job` | A task (`std::function`, so captures must be copyable) plus its shared completion state. |
| `JobHandle` | Stays valid however long the job or scheduler lives. `IsComplete()`; `WasAbandoned()` if the scheduler shut down before the job ran. |
| `WorkerThread` | A thread that asks the scheduler for the next job and runs it. |

On shutdown, queued jobs are abandoned (never run), running ones finish, then the workers are joined. Started in `RZE_Engine::Init()`, stopped in `BeginShutDown()` ([Engine-Startup-And-Frame-Loop.md](../02-Architecture/Engine-Startup-And-Frame-Loop.md)).

Don't `Wait()` from a worker on a job that might be queued behind you; with only four workers that can deadlock.

## Getting back to the main thread: `MainThreadDispatcher`

```cpp
Threading::MainThreadDispatcher::Get().Post(task);   // any thread
```

Posted tasks run on the main thread, in the order they were posted, at the start of the next frame (`RZE_Engine::PreUpdate`). Tasks posted while draining run the following frame. This is how worker code hands results back without touching main-thread-only state.

## Cancelling: `CancellationToken`

A shared, cooperative cancel flag. Copies share state, so it can be captured by value into jobs and checked from any thread. Long jobs should check it and stop early (the Editor's build job passes it to the process it runs).

## Who uses all this

- **Async operations** (`Engine\Src\EngineCore\Async\`): **scene loading** reads and parses on workers, then finalizes on the main thread a slice per frame; the Editor's **Build/Launch** runs as a worker job. See [Asset-Import-Runtime.md](../05-AssetPipeline/Asset-Import-Runtime.md#async-operations).
- **`AsyncResourceBatch`** runs each resource's `LoadCPU` as a worker job.

## Key files

- `Engine\Src\EngineCore\Threading\Threading.h`, `MainThreadDispatcher.h/.cpp`, `CancellationToken.h`
- `Engine\Src\EngineCore\Threading\JobSystem\Job.h/.cpp`, `WorkerThread.h/.cpp`, `JobScheduler.h/.cpp`
- `Engine\Src\EngineCore\Async\` — operations built on top of the job system
