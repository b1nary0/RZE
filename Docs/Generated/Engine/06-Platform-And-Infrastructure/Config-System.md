# Config System

`Engine\Src\EngineCore\Config\` provides a thin INI-backed configuration layer, loaded once at startup as a first-class `IResource`.

## `Config` (`Config.h/.cpp`)

`class Config : public IResource` — wraps `CSimpleIniA` (the vendored SimpleINI third-party library) plus a `File`. `Read(filePath)` parses an INI file into an internal `mINIParser`.

## `EngineConfig` (`EngineConfig.h/.cpp`)

`class EngineConfig final : public Config` — loads `Config\Engine.ini` and exposes two structs:
- `EngineSettings` — `EnableVsync`, `MSAASampleCount`
- `WindowSettings` — `WindowTitle`, `WindowWidth`, `WindowHeight`

Loaded once during `RZE_Engine::Init()` via `LoadEngineConfig()` → `ResourceHandler::LoadResource<EngineConfig>()` — it goes through the exact same resource-loading path as meshes/textures ([Asset-Import-Runtime.md](../05-AssetPipeline/Asset-Import-Runtime.md)), just with a config-file `IResource` implementation instead of a binary asset format.

## `Config\Engine.ini` contents (checked into the repo)

```ini
[EngineSettings]
EnableVsync=1
MSAASampleCount=8

[WindowSettings]
WindowWidth=1600
WindowHeight=900
WindowTitle=Game Title
```

Consumed directly by `CreateAndInitializeWindow()` and `CreateAndInitializeRenderer()` during startup (see [Engine-Startup-And-Frame-Loop.md](../02-Architecture/Engine-Startup-And-Frame-Loop.md)).

## `Config\imgui.ini`

A standard Dear ImGui persisted window-layout file, auto-generated/updated by ImGui itself — not hand-authored, and not part of the `EngineConfig`/`Config` class hierarchy. The Editor points ImGui's `io.IniFilename` at this path during `EditorApp::Initialize()`.

## Where config lives in the build/asset flow

`AssetCpy.bat` copies the entire `Config\` folder into the build output directory alongside `Assets\` and `ProjectData\` — see [Building-And-Running.md](../01-GettingStarted/Building-And-Running.md).

## Key files

- `Engine\Src\EngineCore\Config\Config.h/.cpp`
- `Engine\Src\EngineCore\Config\EngineConfig.h/.cpp`
- `Config\Engine.ini`, `Config\imgui.ini`
