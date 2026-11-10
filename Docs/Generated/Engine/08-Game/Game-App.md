# Game App

`GameApp` (`Game\Src\GameApp.h/.cpp`) is the shippable game's `RZE_Application` subclass — much thinner than the Editor since it carries no tooling UI (ImGui is present in Debug and Development but not Retail, via `IMGUI_ENABLED`; see [Debug-Services-And-Logging.md](../06-Platform-And-Infrastructure/Debug-Services-And-Logging.md)).

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

- **`ParseArguments`** — reads a `-scene` CLI argument into `m_startScene`; the Editor's Launch Game passes the scene that's open in it ([Editor-Camera-And-Build-Launch.md](../07-Editor/Editor-Camera-And-Build-Launch.md)). A source `#TODO` comment notes this dev-only field should probably be hidden behind a debug `#ifdef`.
- **`OnInitialize()`** — runs once the base `Initialize(window)` has kept the window and registered the engine's components. Sets the window's icon from `Config\Game.ini`'s `Icon` if it has one (`GameConfig`, `Window::SetIcon`; otherwise the window keeps RZE's own icon, see [Windowing.md](../06-Platform-And-Infrastructure/Windowing.md#icons)), creates the render target from the window's client size and sets it on `RenderEngine`, then deserializes either `m_startScene` (from `Assets\Scenes\<name>`) or the default `Assets\Scenes\RenderTest.scene` into `RZE().GetActiveScene()`. Conditionally adds `ImGuiRenderStage` (`#ifdef IMGUI_ENABLED`: Debug and Development, not Retail), and **unconditionally** adds `FinalRenderTargetStage` — the stage that blits the offscreen render target to the actual back buffer. This is present in `GameApp` but never added by the Editor, since the Editor renders into an ImGui panel instead of presenting directly (see [Editor-Overview.md](../07-Editor/Editor-Overview.md)).
- **`Start()`** — takes the loaded scene's main camera (`GameScene::GetMainCamera()`, asserting there is one) and attaches a `PlayerControllerComponent` to it at runtime. The playable camera is therefore scene-authored (whichever camera is saved as the main one, see [Scene-And-GameObject-Model.md](../03-SceneAndGameObjects/Scene-And-GameObject-Model.md#the-main-camera)), but the controller *behavior* is attached by game code, not saved in the scene file — see [Built-In-Components.md](../03-SceneAndGameObjects/Built-In-Components.md).
- **`UpdateUI()`** — the game's UI step, on UI frames inside ImGui's frame ([Engine-Startup-And-Frame-Loop.md](../02-Architecture/Engine-Startup-And-Frame-Loop.md#update)); in `IMGUI_ENABLED` builds it shows the ImGui demo window (leftover dev debug, not shipped functionality). The game has no per-frame work of its own, so it doesn't override `Update()`.
- **`RegisterInputEvents`** — Escape quits (`RZE().PostExit()`); F1/F2/F3 resize the window to 720p/900p/1080p presets (`RZE().SetWindowSize`).
- **`ProcessInput`** — in `IMGUI_ENABLED` builds, forwards mouse and key state to ImGui (`ImGuiInput::SubmitInput`), then returns an `InputGate` that gives each device to the game unless ImGui wants it: the keyboard while a field is being typed in (`WantCaptureKeyboard`), the mouse while it's over an ImGui window (`WantCaptureMouse`). The engine raises the events of a device the gate lets through and resets the other, so the game's key bindings (Escape, F1–F3, the player controller's) fire as they do in Retail ([Known Issues](../10-KnownIssues/Architectural-Notes-And-TODOs.md#30-the-games-key-bindings-never-fired-with-imgui-on-fixed)). In Retail it delegates straight to the base `RZE_Application::ProcessInput`.
- **`OnWindowResize`** — recreates the render target, updates `RenderEngine`'s viewport size, and re-derives the main camera's aspect ratio.
- **`CreateRenderTarget`** — asserts `m_renderTarget` is currently null (`AssertIsNull`) before constructing a new `Rendering::RenderTargetTexture`.

## Engine coupling

`RZE.h`, `GameObject.h`, `CameraComponent.h`, `PlayerControllerComponent.h`, `RenderEngine.h` (Engine); `Rendering\Graphics\RenderTarget.h` (Modules\Rendering); `CmdLine.h`, `Debug.h`, `Filepath.h` (Utils); conditionally `imgui.h` + `ImGuiRenderStage.h`, and unconditionally `FinalRenderTargetStage.h`.

## Contrast with the Editor

See [App-Shell-Pattern.md](../02-Architecture/App-Shell-Pattern.md) for the full comparison table. In short: both `GameApp` and `EditorApp` are thin `RZE_Application` subclasses that mostly configure render targets/stages and scene loading — almost all "engine" logic lives in `Engine`/`Modules\Rendering`, not in either app shell.

## Key files

- `Game\Src\Main.cpp`
- `Game\Src\GameApp.h/.cpp`
