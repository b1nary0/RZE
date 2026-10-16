# Engine Startup & Frame Loop

The two central types in RZE are **`RZE_Engine`** (owns everything — window, renderer, scene, resources, input) and **`RZE_Application`** (a virtual base class overridden once per product: `GameApp`, `EditorApp`). There is no `main()` inside `Engine` itself — it's a library consumed by each product's entry point.

## The global engine accessor

`RZE\Engine\Src\RZE.h` / `RZE.cpp` expose a free function:

```cpp
RZE_Engine& RZE();
```

backed by a file-static global instance in `RZE.cpp`. This is used pervasively — `RZE().GetResourceHandler()`, `RZE().GetRenderEngine()`, `RZE().GetActiveScene()`, `RZE().GetInputHandler()`, `RZE().PostExit()` — as a **service locator**, not dependency injection. Almost any file in the codebase can reach any core subsystem through this single global.

## Entry points

```cpp
// RZE\Game\Src\Main.cpp
int main(int argc, char* argv[]) {
    GameApp gGameApp;
    if (argc > 1) gGameApp.ParseArguments(argv, argc);
    gGameApp.Run();
}
```

`RZE\Editor\Src\EditorMain.cpp` follows the identical shape with `Editor::EditorApp`. `RZE_Application::Run()` (in `EngineApp.cpp`) calls `RZE().Run(engineHook)`, passing a callback that knows how to construct the concrete application subclass.

## `RZE_Engine` (`Engine\Src\EngineCore\Engine.h` / `.cpp`)

Owns: `RZE_Application* m_application`, `GameScene* m_activeScene`, `Win32Window* m_window`, `ResourceHandler m_resourceHandler`, `EventHandler m_eventHandler`, `InputHandler m_inputHandler`, `std::unique_ptr<RenderEngine> m_renderEngine`, `AsyncOperationManager m_asyncOperationManager`, plus frame timing: `HiResTimer m_programTimer` (started with the main loop; frame times are measured on it), `FramePacer m_framePacer` ([below](#frame-pacing)), `PreciseSleep m_frameSleep`, `FrameTimeCapture m_frameTimeCapture`, `m_deltaTime` and `m_frameCount`.

Public accessors used as the de-facto service locator: `GetApplication()`, `GetResourceHandler()`, `GetInputHandler()`, `GetRenderEngine()`, `GetAsyncOperationManager()`, `GetActiveScene()`, `GetDeltaTime()`/`GetDeltaTimeMS()`, `GetFramePacer()` / `SetFrameSettings()`, `SetWindowSize()`, `PostExit()`.

## Startup sequence

```mermaid
%%{init: {'flowchart': {'subGraphTitleMargin': {'top': 6, 'bottom': 10}}, 'themeCSS': '.labelBkg, .edgeLabel { background-color: var(--background-primary) !important; } .messageText, .labelText, .loopText { paint-order: stroke; stroke: var(--background-primary); stroke-width: 5px; stroke-linejoin: round; }'}}%%
flowchart TD
    A[RZE_Application::Run] --> B["RZE_Engine::Run(createApplicationCallback)"]
    B --> C[Init]
    C --> C1[Debug::CreateLogFile / DebugServices::Get.Initialize]
    C1 --> C1b["MainThreadDispatcher::Initialize<br/>+ Debug::SetLogSink<br/>(log lines also go to DebugServices)"]
    C1b --> C2[Threading::JobScheduler::Get.Initialize]
    C2 --> C2b[AsyncOperationManager::Initialize]
    C2b --> C3["LoadEngineConfig (Config/Engine.ini)"]
    C3 --> C4[CreateAndInitializeWindow — Win32Window]
    C4 --> C5[InputHandler::Initialize + RegisterWindowEvents]
    C5 --> C6[ResourceHandler::Init]
    C6 --> C7["CreateAndInitializeRenderer (new RenderEngine, Initialize(hwnd))"]
    C7 --> C8[Create + Initialize GameScene]
    C8 --> D[PostInit]
    D --> D1[Register key events + ImGui key map]
    D1 --> D2["InitializeApplication:<br/>construct the app, then app.Initialize()<br/>(registers components),<br/>app.RegisterInputEvents(), app.Start()"]
    D2 --> D3[GameScene::Start]
    D3 --> F["Start the program timer,<br/>FrameTimeCapture::Initialize,<br/>InitializeFramePacing"]
    F --> E[Main Loop]
```

1. **`Init()`**:
   - `Debug::CreateLogFile()`, `DebugServices::Get().Initialize()`
   - `Threading::MainThreadDispatcher::Get().Initialize()` (records the main thread's ID), then `Debug::SetLogSink(&TraceLogLine)` so every `RZE_LOG` line also reaches `DebugServices` ([Debug-Services-And-Logging.md](../06-Platform-And-Infrastructure/Debug-Services-And-Logging.md))
   - `Threading::JobScheduler::Get().Initialize()`, then `m_asyncOperationManager.Initialize()` ([Threading-And-Jobs.md](../06-Platform-And-Infrastructure/Threading-And-Jobs.md))
   - `LoadEngineConfig()` — loads `Config/Engine.ini` as an `EngineConfig` resource
   - `CreateAndInitializeWindow()` — constructs `Win32Window` sized/titled per config
   - `m_inputHandler.Initialize()`, `RegisterWindowEvents()` (subscribes to `EEventType::Window` → window resize, destroy, and close requests, which exit if `RZE_Application::OnCloseRequested()` agrees)
   - `m_resourceHandler.Init()`
   - `CreateAndInitializeRenderer()` — `new RenderEngine`, `Initialize(hwnd)`
   - Creates and initializes the `GameScene`
2. **`PostInit(createApplicationCallback)`**: registers key events (ImGui key map), invokes the callback to construct the concrete `RZE_Application`, sets its window, calls `app->Initialize()` (registers all built-in `GameObjectComponent` types via `RegisterEngineComponents()`), then `app->RegisterInputEvents()`, then `app->Start()`, then `m_activeScene->Start()`.
3. **Before the first frame**: the program timer starts, `FrameTimeCapture::Initialize()` reads the `-perf*` arguments, and `InitializeFramePacing()` reads the [frame settings](#settings), after the capture since a capture turns pacing off, and gives the `FramePacer` the monitor's refresh rate.

## Per-frame loop

Each iteration of `while (!m_shouldExit)` is an Optick frame, and runs these steps in order:

```mermaid
%%{init: {'flowchart': {'subGraphTitleMargin': {'top': 6, 'bottom': 10}}, 'themeCSS': '.labelBkg, .edgeLabel { background-color: var(--background-primary) !important; } .messageText, .labelText, .loopText { paint-order: stroke; stroke: var(--background-primary); stroke-width: 5px; stroke-linejoin: round; }'}}%%
sequenceDiagram
    participant ML as Main Loop
    participant FP as FramePacer
    participant Win as Win32Window
    participant Async as AsyncOps
    participant App as RZE_Application
    participant RE as RenderEngine

    ML->>FP: BeginFrame(now) - frame time and delta time
    ML->>Win: PreUpdate - drain posted tasks,<br/>handle every queued window message
    ML->>App: ProcessInput, then RaiseEvents or Reset
    ML->>Async: Tick() - the frame's budget,<br/>completion callbacks fire here
    ML->>FP: TakeUIFrame(now)
    opt a UI frame
        ML->>App: PreUIUpdate()
        ML->>App: ImGui NewFrame, UpdateUI(), ImGui EndFrame
    end
    ML->>App: Update(), then the scene's Update()
    ML->>App: PreRender()
    ML->>RE: Render("Main Render"), Finish()
    opt a UI frame
        ML->>ML: ResourceHandler::Update()
    end
    ML->>Async: TickIdle(spare time)
    ML->>ML: sleep until the next frame is due
    ML->>RE: WaitForPresentSlot()
```

```mermaid
gantt
    dateFormat x
    axisFormat %L
    title One paced frame on the main thread, in order (not to scale)
    section Main thread
    Tasks, input and events       :a, 0, 2ms
    Async tick                    :b, after a, 2ms
    UI, on UI frames              :c, after b, 3ms
    App, scene and PreRender      :d, after c, 3ms
    Render and hand over          :e, after d, 3ms
    Idle async work               :f, after e, 2ms
    Sleep                         :g, after f, 3ms
    Wait for a present slot       :h, after g, 1ms
```

### Input and events

`PreUpdate()` runs first:

1. `MainThreadDispatcher::Drain()` runs every task worker threads posted since the last frame, such as log lines.
2. `Win32Window::CompileInputMessages` handles **every** queued window message, not one per frame, so input doesn't queue up behind capped frames ([Windowing.md](../06-Platform-And-Infrastructure/Windowing.md)).
3. `InputHandler::UpdateFrameState` lets go of everything still held if the window has just lost focus, and works out whether the user is interacting ([Input-System.md](../06-Platform-And-Infrastructure/Input-System.md#interaction-and-focus)). If they are, the loop tells the `FramePacer` (`NoteInteraction`), so the UI keeps up (below).
4. `CompileWindowMessages`, then `EventHandler::ProcessEvents()`.
5. `RZE_Application::ProcessInput(inputHandler)`. Returning `true` fires the bound actions (`RaiseEvents()`); returning `false` means the application is "stealing" input (e.g. the Editor consuming mouse and keyboard for its panels), so the engine calls `Reset()` instead and no gameplay actions fire that frame.

### Update

The update is split into a scene part, which runs every frame, and a UI part, which can run less often:

1. **`AsyncOperationManager::Tick()`**, on its own and first, so completion callbacks (a scene finishing loading) land before the application gathers this frame's state.
2. **`RZE_Engine::UpdateUI()`**, only on a UI frame (`FramePacer::TakeUIFrame`, [below](#ui-frame-rate)): the application's `PreUIUpdate()`, then ImGui's `NewFrame` (DX11 and Win32 backends, `ImGuizmo::BeginFrame`), the application's `UpdateUI()`, and `ImGui::EndFrame()`.
3. **`RZE_Engine::Update()`**, every frame: the application's `Update()`, the scene's `Update()`, then the application's `PreRender()`.

| Hook on `RZE_Application` | Runs | For | ImGui calls |
|---|---|---|---|
| `PreUIUpdate()` | UI frames, before ImGui's frame | Gathering what the UI shows (the Editor's panel `Update`s) | No: there's no ImGui frame, so a stray call fails at once |
| `UpdateUI()` | UI frames, inside ImGui's frame | The UI and applying what it changes (the Editor's commands) | Yes |
| `Update()` | Every frame, after the UI | The application's own per-frame work | No |
| `PreRender()` | Every frame, after the scene's update | Per-frame scene overlays such as debug lines, which are cleared after every render (the Editor's grid and camera frustums) | No |

### Render and present

`RenderEngine::Render("Main Render")` runs the render pipeline for the main view, and `Finish()` hands the frame's commands to the render thread (`DevicePresent`). After that, on UI frames only, `ResourceHandler::Update()` destroys the resources released in earlier frames that the render thread has finished with ([Asset-Import-Runtime.md](../05-AssetPipeline/Asset-Import-Runtime.md#releasing-resources)). A frame without a UI update renders the last UI again, which may still show a texture released since, so resources are only destroyed once a newer UI has replaced it.

### Waiting for the next frame

`WaitForNextFrame()` ends the frame:

1. It tells the `FramePacer` the frame's work is done. If the frame is paced and has at least 1 ms to spare before the next one is due (less a 1.5 ms safety margin), `AsyncOperationManager::TickIdle` gets that time ([Asset-Import-Runtime.md](../05-AssetPipeline/Asset-Import-Runtime.md#async-operations)). It runs after `Finish()`, so it adds nothing to this frame's latency.
2. `PreciseSleep` sleeps until the CPU cap lets the next frame start: a high-resolution waitable timer, then a spin for the last millisecond, since timer wake-ups can be that late.
3. `RenderEngine::WaitForPresentSlot()` waits until the swap chain can queue another frame, so at most two are in flight ([Low-Level-Renderer-Module.md](../04-Rendering/Low-Level-Renderer-Module.md#the-swap-chain-and-vsync)). It comes last so the next frame reads its input as late as possible, and with vsync it's what paces frames. It gives up after 100 ms, e.g. while the window is minimised and nothing presents.

## Frame pacing

`FramePacer` (`Engine\Src\EngineCore\Timing\FramePacer.h/.cpp`) decides when each frame starts, what delta time it simulates, how much spare time it has and whether it updates the UI. It's only timing logic: the engine passes it the time, so it's unit tested (`FramePacerTests.cpp`), and the engine does the waiting.

### Settings

`FrameSettings` (`Engine\Src\EngineCore\Config\FrameSettings.h`):

| Setting | Engine.ini `[FrameSettings]` | Command line | Default | Meaning |
|---|---|---|---|---|
| `VSync` | `EnableVsync` | `-vsync 0/1` | On | Present on the display's refresh |
| `MaxFrameRate` | `MaxFrameRate` | `-maxfps N` | 0 | The most frames a second; 0 for no cap beyond vsync |
| `UnfocusedFrameRate` | `UnfocusedFrameRate` | | 30 | Frames a second while the window is unfocused or minimised; 0 to not throttle |
| `UIFrameRate` | `UIFrameRate` | `-uifps N` | 0 | How many times a second the UI updates; 0 for every frame |

`RZE_Engine::InitializeFramePacing()` reads them once the main loop's timer has started, with the command line overriding Engine.ini ([Config-System.md](../06-Platform-And-Infrastructure/Config-System.md)). `RZE_Engine::SetFrameSettings()` changes them while running (the Editor's [Frame Rate page](../07-Editor/Status-Bar-And-Settings.md#pages)); it also passes vsync on to the renderer. `GetFramePacer()` reads the settings and the stats.

A [`-perfcapture`](../06-Platform-And-Infrastructure/Debug-Services-And-Logging.md#frame-time-capture) run turns vsync, the caps and the UI rate off so it measures what frames cost; `-vsync`, `-maxfps` and `-uifps` still apply.

### When frames start

- **The frame period** is the longer of the CPU cap's period and the refresh period while vsync is on. The cap is `MaxFrameRate`, or the lower of it and `UnfocusedFrameRate` while the window isn't the foreground window or is minimised. The refresh rate comes from `Display::GetRefreshRate`, at start-up and on every `Window_Resize` event, which is also sent after a move to another monitor.
- **Scheduling.** The next frame is due a cap period after the current one *was due*, not after it started, so small oversleeps don't add up. A frame that starts over a period late re-bases on now instead, so the frames after it don't rush to catch up.
- **Delta time** (`GetDeltaTime()`, which `RZE().GetDeltaTime()` returns) is the measured frame time, clamped to 100 ms so breakpoints, window drags and load hitches don't make the scene jump. A frame within 10% of the target period simulates the running average of on-time frames instead, so timer jitter doesn't show as uneven motion. The average converges on the real cadence, e.g. 59.94 Hz on a display that reports 59.
- **Stats** (`GetStats()`), averaged over half a second and updated once per half second so they stay readable: frame time, frames a second, idle time per frame and the part of it spent on async work, and UI frames a second. The Editor's status bar shows them.

### UI frame rate

With `UIFrameRate` set, only some frames are UI frames. Everything else (the async tick, the application's `Update`, the scene's update, `PreRender` and rendering) still runs every frame. On a frame without a UI update, no ImGui frame has begun, so `ImGuiRenderStage`'s `ImGui::Render()` returns at once and the last UI's draw data goes out again around this frame's scene.

`TakeUIFrame(now)` says whether this frame is a UI frame:

- **Always**, with no UI frame rate.
- **While the user is interacting**, and for a quarter of a second after (`k_interactionHoldTime`), so what's drawn over the scene (the gizmo, object icons, the marquee) keeps up with the camera and drags don't feel steppy. Then the UI frame rate resumes, a UI period after the last UI frame.
- **When one was requested** (`RequestUIFrame`). The first frame always is one, and the engine asks for one when the window resizes, since the application may have replaced what the last UI shows (the Editor's scene view target).
- **Otherwise once the next is due**, counting one due within half a frame, so a rate that divides the frame rate gets an even cadence. UI frames are scheduled from when they were due, like frames.

## Shutdown

`BeginShutDown()`, in order:
1. `AsyncOperationManager::ShutDown()` first, so no async work touches the scene, resources or renderer while they shut down (it cancels operations and waits for their worker jobs; completion callbacks don't fire);
2. scene shutdown, then app shutdown;
3. the main scene's `ClearObjects()`, then `ReleaseRenderStages()` (must happen before resources are released);
4. `RenderEngine::WaitUntilIdle()`, then `ResourceHandler::DestroyPendingNow()`: resources released while shutting down are waiting on the render thread, and they must go **before** the renderer shuts down, since releasing a GPU resource records a render command;
5. `RenderEngine::Shutdown()`;
6. `JobScheduler::ShutDown()` (queued jobs are abandoned, running ones finish), clear the log sink, `MainThreadDispatcher::ShutDown()`;
7. `InternalShutDown()` deletes the scene and renderer, releases the engine config, shuts down the `ResourceHandler`, and deletes the window.

## Key files

- `Engine\Src\RZE.h` / `RZE.cpp` — global `RZE()` accessor
- `Engine\Src\EngineCore\Engine.h` / `Engine.cpp` — `RZE_Engine`, the main loop
- `Engine\Src\EngineCore\Timing\FramePacer.h` / `.cpp`, `Engine\Src\EngineCore\Config\FrameSettings.h` — frame pacing
- `Utils\Src\Utils\Platform\Timers\PreciseSleep.h` / `.cpp` — the sleep between frames
- `UnitTests\Src\Engine\FramePacerTests.cpp`
- `Engine\Src\EngineApp.h` / `EngineApp.cpp` — `RZE_Application`
- `Game\Src\Main.cpp`, `Editor\Src\EditorMain.cpp` — the two entry points
