# Config System

`Engine\Src\EngineCore\Config\` provides a thin INI-backed configuration layer, loaded once at startup as a first-class `IResource`.

## `Config` (`Config.h/.cpp`)

`class Config : public IResource` — wraps `CSimpleIniA` (the vendored SimpleINI third-party library) plus a `File`. `Read(filePath)` parses an INI file into an internal `mINIParser`. `Write(filePath)` saves the parser's values back, keeping the comments `Read` found, as `Key=Value` with no spaces. It writes beside the file first and then moves it over (`File::MoveReplacing`), so a reader never gets half a file. Only `GameConfig` writes today.

## `EngineConfig` (`EngineConfig.h/.cpp`)

`class EngineConfig final : public Config` — loads `Config\Engine.ini` and exposes four structs and the rendering settings:
- `EngineSettings` — `MSAASampleCount`. Nothing reads it today: the renderer's stages pick their own sample counts.
- `WindowSettings` — `WindowTitle`, `WindowWidth`, `WindowHeight`, `LaunchMonitor`
- `FrameSettings` (`FrameSettings.h`) — `[FrameSettings]`: `EnableVsync`, `MaxFrameRate`, `UnfocusedFrameRate`, `UIFrameRate`. A missing key keeps the struct's default. See [Engine-Startup-And-Frame-Loop.md](../02-Architecture/Engine-Startup-And-Frame-Loop.md#frame-pacing).
- `AsyncOperationFilter` (`EngineCore\Async\AsyncOperationFilter.h`, `GetBackgroundTaskFilter()`) — `[BackgroundTasks]`: which running operations progress displays show (the Editor's status bar). `ShowSubSteps`, `ShowBlocking`, and a key per kind: `ShowResourceLoads`, `ShowSceneLoads`, `ShowRendering`, `ShowSaving`, `ShowOther`. A missing key keeps the default, which shows it. `RZE_Engine::Init` gives it to the `AsyncOperationManager` right after loading the config ([Asset-Import-Runtime.md](../05-AssetPipeline/Asset-Import-Runtime.md#which-operations-are-shown)).
- `[Rendering]` — the main view's rendering, which `RZE_Engine::CreateAndInitializeRenderer()` sets once the `RenderEngine` is made:
  - `AntiAliasing` (`Off`, `FXAA` or `SMAA`) and `AntiAliasingQuality` (`Low`, `Medium`, `High` or `Ultra`; FXAA uses High for Ultra), together `GetAntiAliasing()`, an `AntiAliasingMode`. SMAA High by default ([Render-Stages.md](../04-Rendering/Render-Stages.md#antialiasingstage)).
  - `AmbientOcclusion` (`Off`, `Low`, `Medium` or `High`; `GetAmbientOcclusion()`), `GlobalIllumination` and `AutoExposure` (`0` or `1`).
  - `ProbeTargetCount`, `ProbeCaptureDrawBudget` and `ProbeRelightBudget` (`GetGlobalIlluminationSettings()`; at least 1 each), which go to `RenderEngine::SetGlobalIlluminationSettings` ([Render-Stages.md](../04-Rendering/Render-Stages.md#irradianceprobestage)).
  - Names are read in any case (`ReadEnumSetting`, from each enum's `EnumNames` table). A name that isn't one logs a warning listing the allowed ones and keeps the default; a missing key keeps it quietly.

Loaded once during `RZE_Engine::Init()` via `LoadEngineConfig()` → `ResourceHandler::LoadResource<EngineConfig>()` — it goes through the exact same resource-loading path as meshes/textures ([Asset-Import-Runtime.md](../05-AssetPipeline/Asset-Import-Runtime.md)), just with a config-file `IResource` implementation instead of a binary asset format.

## `Config\Engine.ini` contents (checked into the repo)

```ini
[EngineSettings]
MSAASampleCount=8

[FrameSettings]
; Present on the display's refresh
EnableVsync=1
; The most frames per second the loop runs, 0 for no cap beyond vsync
MaxFrameRate=0
; Frames per second while the window is unfocused or minimised, 0 to not throttle
UnfocusedFrameRate=30
; How many times a second the UI updates while you're not interacting, 0 for every frame. The scene still updates and renders every frame.
UIFrameRate=15

[Rendering]
; How the main view smooths jagged edges: Off, FXAA (the cheapest; softens some texture detail) or SMAA (keeps
; textures sharp; three passes)
AntiAliasing=SMAA
; How hard it works at it: Low, Medium, High or Ultra (SMAA only; FXAA uses High)
AntiAliasingQuality=High
; How much creases and corners darken the indirect light (GTAO): Off, Low, Medium or High
AmbientOcclusion=Medium
; Indirect light from the scene's irradiance probes, 0 for the flat sky and ground ambient
GlobalIllumination=1
; About how many probes a scene gets, whatever its size; draws a frame for capturing them; probes relit a frame
ProbeTargetCount=4096
ProbeCaptureDrawBudget=4000
ProbeRelightBudget=512
; Expose the main view to the scene's brightness, adapting over time; 0 for a fixed exposure of 1
AutoExposure=1

[BackgroundTasks]
; Which tasks progress displays (the editor's status bar) show. Every task is reported; these only pick which to show.
; Steps of other tasks, e.g. each image of a folder's "Loading thumbnails", or the scene a thumbnail run loads
ShowSubSteps=0
; Tasks that block the editor, which show a progress dialog too, e.g. opening a scene
ShowBlocking=1
ShowResourceLoads=1
ShowSceneLoads=1
ShowRendering=1
ShowSaving=1
; Anything else, e.g. building the game
ShowOther=1

[WindowSettings]
WindowWidth=1600
WindowHeight=900
WindowTitle=Game Title
LaunchMonitor=0
```

Read during startup by the splash screen and `CreateAndInitializeWindow()` (`[WindowSettings]`; `-launchMonitor` overrides `LaunchMonitor`) and `InitializeFramePacing()` (`[FrameSettings]`; `-vsync`, `-maxfps` and `-uifps` override it), see [Engine-Startup-And-Frame-Loop.md](../02-Architecture/Engine-Startup-And-Frame-Loop.md). The frame settings can change while running (`RZE_Engine::SetFrameSettings`, the Editor's Frame Rate page), and so can the background task filter (`AsyncOperationManager::SetVisibilityFilter`, the Background Tasks page) and the main view's rendering settings (the Graphics page), but nothing writes them back to the file.

## `GameConfig` (`GameConfig.h/.cpp`) and `Config\Game.ini`

`class GameConfig final : public Config` — what the game itself is, as opposed to how the engine runs it. The Editor's [Project Settings](../07-Editor/Editor-Panels.md#project-settings-projectsettingspanel) change it and save it as they go; the Game reads it in `GameApp::Initialize` ([Game-App.md](../08-Game/Game-App.md#lifecycle)).

- `[Game] Icon` (`GetIcon`/`SetIcon`): the image the game's window shows, a path relative to the project root, saved with `\` like every stored path (`Filepath::GetSerializedPath`). Empty (an invalid `Filepath`) keeps the executable's icon, the RZE mark ([Windowing.md](Windowing.md#icons)).
- `Load` on a missing file leaves everything at its default, and `Save` creates it. `Save` writes over the file it was loaded from.

Unlike `EngineConfig` it isn't loaded through the `ResourceHandler`, whose resources are read-only: the Editor's panel and `GameApp` each own one.

```ini
[Game]

; The game window's icon: a square PNG under Assets, 256 px or larger. Empty for the RZE icon. Set in the editor's Edit > Project Settings.
Icon=
```

The checked-in file is in the layout SimpleINI writes (the blank line before a commented key), so saving it only changes the values.

## `Config\imgui.ini`

A standard Dear ImGui persisted window-layout file, auto-generated/updated by ImGui itself — not hand-authored, and not part of the `EngineConfig`/`Config` class hierarchy. The Editor points ImGui's `io.IniFilename` at this path during `EditorApp::OnInitialize()`. ImGui saves it a few seconds after a change, and again on exit when the render thread destroys its context ([Engine-Startup-And-Frame-Loop.md](../02-Architecture/Engine-Startup-And-Frame-Loop.md#shutdown)). Besides the windows and dock layout, it holds the Editor's own `[BottomDockArea][Area]` section ([Editor-Framework.md](../07-Editor/Editor-Framework.md#the-bottom-dock-area)).

It's local: every Editor run rewrites it, so it isn't tracked (`.gitignore`), and a fresh clone has none. Without one, the Editor builds its default layout (`PanelManager::BuildDefaultDockLayout` and `BuildBottomDockLayout`, [Editor-Framework.md](../07-Editor/Editor-Framework.md)). The run-rze drivers and the CI smoke test put it back as it was after a session, or remove the one a run wrote if there was none ([EditorHarness-And-Drivers.md](../07-Editor/EditorHarness-And-Drivers.md#imguiini-is-put-back)).

## Where config lives in the build/asset flow

Nowhere else: the Editor and the Game both read `Config\` from the source tree (`EDirectoryContext::Source`), so an edit to `Engine.ini` or `Game.ini` applies on their next launch with no copy step. See [Building-And-Running.md](../01-GettingStarted/Building-And-Running.md).

## Key files

- `Engine\Src\EngineCore\Config\Config.h/.cpp`
- `Engine\Src\EngineCore\Config\EngineConfig.h/.cpp`, `FrameSettings.h`; `Engine\Src\EngineCore\Async\AsyncOperationFilter.h/.cpp`
- `Engine\Src\EngineCore\Config\GameConfig.h/.cpp`
- `Config\Engine.ini`, `Config\Game.ini`, `Config\imgui.ini` (local, not tracked)
- `UnitTests\Src\Engine\GameConfigTests.cpp`
