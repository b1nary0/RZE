# App-Shell Pattern: `RZE_Application` and its Subclasses

`RZE_Application` (`Engine\Src\EngineApp.h` / `.cpp`) is the virtual base class that every RZE product — `GameApp`, `Editor::EditorApp` — subclasses. Nearly all engine logic (scene graph, rendering, resources, input plumbing) lives in `Engine` itself; the app subclasses are deliberately thin "shells" that wire up product-specific concerns: which render stages are active, what scene loads at startup, and a handful of product-only components.

## The interface

```cpp
class RZE_Application {
public:
    void Run();                                  // kicks off RZE().Run(engineHook)
    virtual void Start();
    virtual void Update();
    virtual void ShutDown();

    virtual void CreateRenderTarget(...) = 0;      // pure virtual
    virtual void OnWindowResize(...) = 0;          // pure virtual

    virtual bool ProcessInput(const InputHandler&); // false = "steal" input from engine-wide firing
    virtual void ParseArguments(...);               // delegates to CmdLine::Arguments::Initialize
    virtual void RegisterInputEvents();
};
```

The base `Initialize()` registers the engine's built-in `GameObjectComponent` types (`CameraComponent`, `RenderComponent`, `TransformComponent`, `DirectionalLightComponent`, `GifChatComponent`) — subclasses are expected to call the base implementation and then register any product-specific components of their own (e.g. `EditorCameraComponent`).

`RZE_Application` owns the `Win32Window*` (assigned by the engine after construction) and a `RenderTargetTexture* m_renderTarget` that each subclass is responsible for creating via `CreateRenderTarget()`.

## Editor vs Game: what differs

| Concern | `GameApp` (`Game\Src\GameApp.h/.cpp`) | `Editor::EditorApp` (`Editor\Src\EditorApp.h/.cpp`) |
|---|---|---|
| Render target destination | Adds `FinalRenderTargetStage` — blits the offscreen RTT to the actual back buffer/swap chain | Never adds it — stays in an off-screen RTT, displayed inside an ImGui `SceneViewPanel` window instead |
| ImGui | Only in Debug builds (`#ifdef IMGUI_ENABLED`, defined in `Debug.h`) — shows the ImGui demo window as leftover dev debug | Always on — the entire UI (dockspace, panels, menu bar) is ImGui |
| Scene loading | Deserializes `-scene` CLI arg or falls back to `Assets/Scenes/RenderTest.scene` | Deserializes `-scene` CLI arg or falls back to `Assets/Scenes/RenderTest.scene` (same fallback, called a "hack" in a code comment) |
| Extra components registered | None beyond the base set | `EditorCameraComponent` (free-fly viewport camera, see [07-Editor](../07-Editor/Editor-Camera-And-Build-Launch.md)) |
| Player/runtime behavior | `Start()` finds the `"Camera"` GameObject and attaches a `PlayerControllerComponent` to it at runtime (not scene-serialized) | No player controller; navigation is via the editor camera instead |
| Extra UI | None (ships without editor tooling) | Full dockspace: `ScenePanel`, `SceneViewPanel`, `LogPanel`, `ResourceMonitorPanel` |
| Dev conveniences | F1/F2/F3 resize window to 720p/900p/1080p presets; Escape quits | "Build Game..." / "Launch Game..." menu items that shell out to `BuildGame.bat` / the built game exe via a background job |

## Why this split matters

Because both apps sit on the exact same `Engine` and `Modules/Rendering` stack, the Editor is not a separate rendering path bolted onto the game — it's the same `RenderEngine`/`RenderStage` pipeline, just pointed at an offscreen texture and given extra ImGui-driven input instead of a `PlayerControllerComponent`. This is why the Editor can render a fully accurate preview of what the Game will show: there is only one renderer, one scene model, and one component system underneath both.

## Key files

- `Engine\Src\EngineApp.h` / `EngineApp.cpp`
- `Game\Src\GameApp.h` / `GameApp.cpp`, `Game\Src\Main.cpp`
- `Editor\Src\EditorApp.h` / `EditorApp.cpp`, `Editor\Src\EditorMain.cpp`
