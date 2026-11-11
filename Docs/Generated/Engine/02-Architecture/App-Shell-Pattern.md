# App-Shell Pattern: `RZE_Application` and its Subclasses

`RZE_Application` (`Engine\Src\EngineApp.h` / `.cpp`) is the virtual base class that every RZE product — `GameApp`, `Editor::EditorApp` — subclasses. Nearly all engine logic (scene graph, rendering, resources, input plumbing) lives in `Engine` itself; the app subclasses are deliberately thin "shells" that wire up product-specific concerns: which render stages are active, what scene loads at startup, and a handful of product-only components.

## The interface

```cpp
class RZE_Application {
public:
    void Initialize(Window&);                    // the engine calls it once its window exists (not virtual)
    void Run();                                  // kicks off RZE().Run(engineHook)
    virtual void Start();
    virtual void PreUIUpdate();                  // UI frames, before ImGui's frame: gather what the UI shows; no ImGui calls
    virtual void UpdateUI();                     // UI frames, inside ImGui's frame: the UI and what it changes
    virtual void Update();                       // every frame, after the UI: the app's own work; no ImGui calls
    virtual void PreRender();                    // every frame, after the scene's update: debug lines etc.; no ImGui calls
    virtual void ShutDown();

    virtual bool OnCloseRequested();               // window close button / Alt+F4; true (default) exits
    virtual Filepath GetSplashScreenImage() const; // shown while starting up; invalid (default) for none
    virtual std::string GetWindowTitle() const;    // the splash screen's and window's first title; empty (default) for Engine.ini's
    virtual bool IsReadyToShow() const;            // false keeps the window off screen while start-up loads
    virtual void CreateRenderTarget(...) = 0;      // pure virtual
    virtual void OnWindowResize(...) = 0;          // pure virtual

    static void RegisterEngineComponents();        // the built-in component types

    virtual bool ProcessInput(const InputHandler&); // false = "steal" input from engine-wide firing
    virtual void ParseArguments(...);               // delegates to CmdLine::Arguments::Initialize
    virtual void RegisterInputEvents();

protected:
    virtual void OnInitialize();                 // the app's own start-up
};
```

`Initialize(window)` is a template method: the engine calls it once its window exists, before `Start()`. It keeps the window, registers the engine's built-in `GameObjectComponent` types (`CameraComponent`, `RenderComponent`, `TransformComponent`, `LightComponent`, `GifChatComponent`) through the static `RegisterEngineComponents()`, then calls the virtual `OnInitialize()`, where each app does its own start-up (render target, render stages, icon). It isn't virtual, so an app can't skip the registration, and there's no base call to remember. Code that runs no application (the unit tests) calls `RegisterEngineComponents()` itself. Product-specific components are registered by the app (the Editor's `EditorCameraComponent`, in `EditorApp`'s constructor).

The four per-frame hooks split each frame into a UI part (`PreUIUpdate`, `UpdateUI`) and a scene part (`Update`, `PreRender`), so the UI can update less often than the scene (`FrameSettings::UIFrameRate`). Both apps' UI is in `UpdateUI`; neither overrides `Update` today, and the Editor draws its grid and camera frustums in `PreRender`. See [Engine-Startup-And-Frame-Loop.md](Engine-Startup-And-Frame-Loop.md#update) for when each runs.

`OnCloseRequested()` is called when the user asks to close the window (its close button, Alt+F4). Returning true, the default, exits. The Editor returns false while the scene has unsaved changes and asks about them first, exiting itself afterwards with `RZE().PostExit()`.

`GetSplashScreenImage()`, `GetWindowTitle()` and `IsReadyToShow()` shape start-up ([Engine-Startup-And-Frame-Loop.md](Engine-Startup-And-Frame-Loop.md#startup-sequence)). The engine takes the application at the start of `RZE_Engine::Run`, before anything is initialized, so it can ask for the splash screen's image. An invalid path, the default, means no splash screen; the Editor returns `logo.png`, and the Game has none. `GetWindowTitle()` is the title the splash screen and the window start with: the Editor's is "RZEStudio", while the Game returns nothing and so takes Engine.ini's `WindowTitle`. The two share Engine.ini, so the application is what tells them apart. `IsReadyToShow()` keeps the window off screen while the engine ticks start-up's async operations with no frames. The default is true; the Editor's is false until its start scene has loaded.

`RZE_Application` owns the `Window*` (null until `Initialize(window)`) and a `std::unique_ptr<RenderTargetTexture> m_renderTarget` that each subclass is responsible for creating via `CreateRenderTarget()`.

The application is `main`'s object, not the engine's, so it's destroyed only after the engine has shut down. What it holds on the GPU must go in `ShutDown()`, while the renderer is still up: the base `ShutDown()` releases `m_renderTarget` (and stops the main view pointing at it), so a subclass that overrides it calls the base last, as `EditorApp::ShutDown` does. See [Engine-Startup-And-Frame-Loop.md](Engine-Startup-And-Frame-Loop.md#shutdown).

## Editor vs Game: what differs

| Concern | `GameApp` (`Game\Src\GameApp.h/.cpp`) | `Editor::EditorApp` (`Editor\Src\EditorApp.h/.cpp`) |
|---|---|---|
| Render target destination | Adds `FinalRenderTargetStage` — blits the offscreen RTT to the actual back buffer/swap chain | Never adds it — stays in an off-screen RTT, displayed inside an ImGui `SceneViewPanel` window instead |
| ImGui | In Debug and Development, not Retail (`#ifdef IMGUI_ENABLED`, defined in `DebugConfig.h`) — shows the ImGui demo window as leftover dev debug | Always on — the entire UI (dockspace, panels, menu bar) is ImGui |
| Scene loading | Deserializes `-scene` CLI arg or falls back to `Assets/Scenes/RenderTest.scene`, synchronously in `OnInitialize()` | Loads `-scene` CLI arg or falls back to `Assets/Scenes/RenderTest.scene` (same fallback, called a "hack" in a code comment), asynchronously through `SceneDocument::Load` in `Start()` |
| Start-up | No splash screen. The window shows once `Start()` is done | A splash screen with the logo and what start-up is doing. The window shows once the start scene has loaded (`IsReadyToShow`) |
| Extra components registered | None beyond the base set | `EditorCameraComponent` (free-fly viewport camera, see [07-Editor](../07-Editor/Editor-Camera-And-Build-Launch.md)) |
| Player/runtime behavior | `Start()` takes the scene's main camera and attaches a `PlayerControllerComponent` to it at runtime (not scene-serialized) | No player controller; navigation is via the editor camera instead |
| Closing the window | Exits | Asks about unsaved changes first (`OnCloseRequested`) |
| Extra UI | None (ships without editor tooling) | Full dockspace of self-registering panels (Object Browser, SceneView, Object Inspector, Log, Resource Monitor), sharing an `EditorContext`; see [Editor-Framework.md](../07-Editor/Editor-Framework.md) |
| Dev conveniences | F1/F2/F3 resize window to 720p/900p/1080p presets; Escape quits | "Build Game..." / "Launch Game..." menu items that run `BuildGame.bat` (Windows) or `BuildGame.sh` (Linux) and the built game via a background job |

**A third shell, on the Editor's: `EditorHarness`.** `EditorHarness::HarnessApp` (`EditorHarness/Src/`) subclasses `Editor::EditorApp`, so it *is* the Editor, plus commands from an automation driver on stdin, which it runs in `PreUIUpdate` after the Editor's own. It reads the Editor's state through `EditorApp::GetContext()`, a protected accessor kept for an app built on the Editor. See [EditorHarness-And-Drivers.md](../07-Editor/EditorHarness-And-Drivers.md).

## Why this split matters

Because both apps sit on the exact same `Engine` and `Modules/Rendering` stack, the Editor is not a separate rendering path bolted onto the game — it's the same `RenderEngine`/`RenderStage` pipeline, just pointed at an offscreen texture and given extra ImGui-driven input instead of a `PlayerControllerComponent`. This is why the Editor can render a fully accurate preview of what the Game will show: there is only one renderer, one scene model, and one component system underneath both.

## Key files

- `Engine/Src/EngineApp.h` / `EngineApp.cpp`
- `Game/Src/GameApp.h` / `GameApp.cpp`, `Game/Src/Main.cpp`
- `Editor/Src/EditorApp.h` / `EditorApp.cpp`, `Editor/Src/EditorMain.cpp`
- `EditorHarness/Src/HarnessApp.h` / `HarnessApp.cpp`, `HarnessMain.cpp`
