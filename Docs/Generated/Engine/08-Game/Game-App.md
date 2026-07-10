# Game App

`GameApp` (`Game\Src\GameApp.h/.cpp`) is the shippable game's `RZE_Application` subclass — much thinner than the Editor since it carries no tooling UI (ImGui is present only in Debug builds, via `IMGUI_ENABLED`; see [Debug-Services-And-Logging.md](../06-Platform-And-Infrastructure/Debug-Services-And-Logging.md)).

## Entry point

`Game\Src\Main.cpp`:
```cpp
int main(int argc, char* argv[]) {
    GameApp gGameApp;
    if (argc > 1) gGameApp.ParseArguments(argv, argc);
    gGameApp.Run();
}
```

## Lifecycle

- **`ParseArguments`** — reads a `-scene` CLI argument into `m_startScene` (a source `#TODO` comment notes this dev-only field should probably be hidden behind a debug `#ifdef`).
- **`Initialize()`** — calls base init, creates the render target from the window's client size and sets it on `RenderEngine`, then deserializes either `m_startScene` (from `Assets\Scenes\<name>`) or the default `Assets\Scenes\RenderTest.scene` into `RZE().GetActiveScene()`. Conditionally adds `ImGuiRenderStage` (`#ifdef IMGUI_ENABLED`, Debug builds only), and **unconditionally** adds `FinalRenderTargetStage` — the stage that blits the offscreen render target to the actual back buffer. This is present in `GameApp` but never added by the Editor, since the Editor renders into an ImGui panel instead of presenting directly (see [Editor-Overview.md](../07-Editor/Editor-Overview.md)).
- **`Start()`** — finds the `GameObject` named `"Camera"` in the loaded scene (asserts it exists) and attaches a `PlayerControllerComponent` to it at runtime. The playable camera/character controller is therefore scene-authored (a `GameObject` named `"Camera"` with a `CameraComponent`), but the controller *behavior* is attached by game code, not saved in the scene file — see [Built-In-Components.md](../03-SceneAndGameObjects/Built-In-Components.md).
- **`Update()`** — base update; in `IMGUI_ENABLED` builds also shows the ImGui demo window (leftover dev debug, not shipped functionality).
- **`RegisterInputEvents`** — Escape quits (`RZE().PostExit()`); F1/F2/F3 resize the window to 720p/900p/1080p presets (`RZE().SetWindowSize`).
- **`ProcessInput`** — in `IMGUI_ENABLED` builds, manually forwards mouse/key state to ImGui IO and returns `false` (input is *not* considered consumed, so gameplay still receives it); otherwise delegates straight to the base `RZE_Application::ProcessInput`.
- **`OnWindowResize`** — recreates the render target, updates `RenderEngine`'s viewport size, and re-derives the scene camera's aspect ratio from the `"Camera"` GameObject's `CameraComponent`.
- **`CreateRenderTarget`** — asserts `m_renderTarget` is currently null (`AssertIsNull`) before constructing a new `Rendering::RenderTargetTexture`.

## Engine coupling

`RZE.h`, `GameObject.h`, `CameraComponent.h`, `PlayerControllerComponent.h`, `RenderEngine.h` (Engine); `Rendering\Graphics\RenderTarget.h` (Modules\Rendering); `CmdLine.h`, `Debug.h`, `Filepath.h` (Utils); conditionally `imgui.h` + `ImGuiRenderStage.h`, and unconditionally `FinalRenderTargetStage.h`.

## Contrast with the Editor

See [App-Shell-Pattern.md](../02-Architecture/App-Shell-Pattern.md) for the full comparison table. In short: both `GameApp` and `EditorApp` are thin `RZE_Application` subclasses that mostly configure render targets/stages and scene loading — almost all "engine" logic lives in `Engine`/`Modules\Rendering`, not in either app shell.

## Key files

- `Game\Src\Main.cpp`
- `Game\Src\GameApp.h/.cpp`
