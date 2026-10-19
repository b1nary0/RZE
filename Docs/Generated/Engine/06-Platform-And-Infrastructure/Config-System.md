# Config System

`Engine\Src\EngineCore\Config\` provides a thin INI-backed configuration layer, loaded once at startup as a first-class `IResource`.

## `Config` (`Config.h/.cpp`)

`class Config : public IResource` — wraps `CSimpleIniA` (the vendored SimpleINI third-party library) plus a `File`. `Read(filePath)` parses an INI file into an internal `mINIParser`.

## `EngineConfig` (`EngineConfig.h/.cpp`)

`class EngineConfig final : public Config` — loads `Config\Engine.ini` and exposes four structs:
- `EngineSettings` — `MSAASampleCount`. Nothing reads it today: the renderer's stages pick their own sample counts.
- `WindowSettings` — `WindowTitle`, `WindowWidth`, `WindowHeight`, `LaunchMonitor`
- `FrameSettings` (`FrameSettings.h`) — `[FrameSettings]`: `EnableVsync`, `MaxFrameRate`, `UnfocusedFrameRate`, `UIFrameRate`. A missing key keeps the struct's default. See [Engine-Startup-And-Frame-Loop.md](../02-Architecture/Engine-Startup-And-Frame-Loop.md#frame-pacing).
- `AsyncOperationFilter` (`EngineCore\Async\AsyncOperationFilter.h`, `GetBackgroundTaskFilter()`) — `[BackgroundTasks]`: which running operations progress displays show (the Editor's status bar). `ShowSubSteps`, `ShowBlocking`, and a key per kind: `ShowResourceLoads`, `ShowSceneLoads`, `ShowRendering`, `ShowSaving`, `ShowOther`. A missing key keeps the default, which shows it. `RZE_Engine::Init` gives it to the `AsyncOperationManager` right after loading the config ([Asset-Import-Runtime.md](../05-AssetPipeline/Asset-Import-Runtime.md#which-operations-are-shown)).

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

Read during startup by `CreateAndInitializeWindow()` (`[WindowSettings]`; `-launchMonitor` overrides `LaunchMonitor`) and `InitializeFramePacing()` (`[FrameSettings]`; `-vsync`, `-maxfps` and `-uifps` override it), see [Engine-Startup-And-Frame-Loop.md](../02-Architecture/Engine-Startup-And-Frame-Loop.md). The frame settings can change while running (`RZE_Engine::SetFrameSettings`, the Editor's Frame Rate page), and so can the background task filter (`AsyncOperationManager::SetVisibilityFilter`, the Background Tasks page), but nothing writes them back to the file.

## `Config\imgui.ini`

A standard Dear ImGui persisted window-layout file, auto-generated/updated by ImGui itself — not hand-authored, and not part of the `EngineConfig`/`Config` class hierarchy. The Editor points ImGui's `io.IniFilename` at this path during `EditorApp::Initialize()`.

## Where config lives in the build/asset flow

Nowhere else: the Editor and the Game both read `Config\` from the source tree (`EDirectoryContext::Source`), so an edit to `Engine.ini` applies on their next launch with no copy step. See [Building-And-Running.md](../01-GettingStarted/Building-And-Running.md).

## Key files

- `Engine\Src\EngineCore\Config\Config.h/.cpp`
- `Engine\Src\EngineCore\Config\EngineConfig.h/.cpp`, `FrameSettings.h`; `Engine\Src\EngineCore\Async\AsyncOperationFilter.h/.cpp`
- `Config\Engine.ini`, `Config\imgui.ini`
