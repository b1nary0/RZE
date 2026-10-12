# RZE Engine Knowledgebase

RZE is a custom C++ game engine built with a hand-rolled entity/component scene model, a command-buffer DirectX 11 renderer running on its own thread, and an offline "asset burning" content pipeline. This knowledgebase documents how it works, organized by system.

This documentation is generated from a point-in-time read of the codebase. File paths are given relative to `C:\dev\RZE\RZE\` unless stated otherwise. When code and docs disagree, trust the code — grep the cited path to confirm before relying on a claim here.

## How the repo is laid out

```
RZE/                    (repo root, C:\dev\RZE)
  RZE/                  (all source + build config)
    Engine/              — core engine library (this is the "Engine" module, not this knowledgebase)
    Modules/Rendering/   — low-level DX11 renderer, independent of Engine
    Editor/              — level/scene editor application (exe)
    Game/                — shippable game application (exe)
    Utils/               — foundational library (math, containers, platform, no engine deps)
    Externals/           — vendored source (ImGui + extensions)
    SourceAssetBurner/   — offline asset conditioning tool (exe)
    ThirdParty/          — vendored prebuilt libs (Assimp, DirectXTK, GLM, RapidJSON, ...)
    Config/, ProjectData/, Assets/, Make/, _project/, _build/
  Docs/                  — hand-written design notes + this generated knowledgebase
  DrawIO/                — original architecture diagrams (source for several diagrams reproduced here)
  Tools/                 — RZEHub.exe, Sharpmake, Assimp, Optick binaries
```

## Module dependency graph

Everything is wired together by Sharpmake (`RZE.sharpmake.cs`), and dependencies flow strictly one way — nothing later in this list is depended on by anything earlier:

```mermaid
%%{init: {'flowchart': {'subGraphTitleMargin': {'top': 6, 'bottom': 10}}, 'themeCSS': '.labelBkg, .edgeLabel { background-color: var(--background-primary) !important; } .messageText, .labelText, .loopText { paint-order: stroke; stroke: var(--background-primary); stroke-width: 5px; stroke-linejoin: round; }'}}%%
graph LR
    Utils["Utils<br/>(math, containers, platform — zero deps)"]
    Externals["Externals<br/>(vendored ImGui + extensions)"]
    Rendering["Modules/Rendering<br/>(DX11 command-buffer renderer)"]
    Engine["Engine<br/>(scene, components, resources, input)"]
    Editor["Editor<br/>(exe)"]
    Game["Game<br/>(exe)"]
    SAB["SourceAssetBurner<br/>(exe)"]

    Utils --> Rendering
    Externals --> Rendering
    Utils --> Engine
    Externals --> Engine
    Rendering --> Engine
    Engine --> SAB
    Engine --> Editor
    Engine --> Game
```

Only the solid edges shown above are declared directly in Sharpmake (`AddPublicDependency<T>`). `Editor`, `Game`, and `SourceAssetBurner` each declare a *single* explicit dependency — on `Engine` — and receive `Utils`/`Externals`/`Rendering` transitively, because Sharpmake propagates a project's *public* dependencies to everything that depends on it. See [02-Architecture/Module-Dependency-Graph.md](02-Architecture/Module-Dependency-Graph.md) for the exact declarations and this transitivity mechanic.

## Navigating this knowledgebase

| Folder | Covers |
|---|---|
| [01-GettingStarted](01-GettingStarted/Building-And-Running.md) | How to build and run RZE end-to-end |
| [02-Architecture](02-Architecture/Module-Dependency-Graph.md) | Module graph, engine startup/frame loop, app-shell pattern, [world conventions](02-Architecture/World-Conventions.md) (metres, axes, angles) |
| [03-SceneAndGameObjects](03-SceneAndGameObjects/Scene-And-GameObject-Model.md) | GameScene, GameObject, the component system, built-in components |
| [04-Rendering](04-Rendering/Rendering-Overview.md) | RenderEngine, render stages, materials/meshes/shaders, the low-level renderer module |
| [05-AssetPipeline](05-AssetPipeline/Asset-Burning-Pipeline.md) | Offline asset burning (SourceAssetBurner) and runtime asset loading |
| [06-Platform-And-Infrastructure](06-Platform-And-Infrastructure/Input-System.md) | Input, events, windowing, config, threading, memory, debug services, Utils reference |
| [07-Editor](07-Editor/Editor-Overview.md) | The ImGui-based scene editor: its [framework](07-Editor/Editor-Framework.md) (context, panels, commands, events), panels, [Object Inspector and property descriptors](07-Editor/Object-Inspector-And-Properties.md), gizmo snapping and grid, and [every shortcut](07-Editor/Editor-Shortcuts.md) |
| [08-Game](08-Game/Game-App.md) | The playable game application shell |
| [09-BuildSystem](09-BuildSystem/Sharpmake-Project-Graph.md) | Sharpmake project definitions, third-party dependencies |
| [10-KnownIssues](10-KnownIssues/Architectural-Notes-And-TODOs.md) | Documented rough edges and TODOs found in the source |

## Where to start

```mermaid
%%{init: {'flowchart': {'subGraphTitleMargin': {'top': 6, 'bottom': 10}}, 'themeCSS': '.labelBkg, .edgeLabel { background-color: var(--background-primary) !important; } .messageText, .labelText, .loopText { paint-order: stroke; stroke: var(--background-primary); stroke-width: 5px; stroke-linejoin: round; }'}}%%
flowchart LR
    A["Building-And-Running"] --> B["World-Conventions<br/>(metres, +Y up, degrees)"]
    B --> C["Engine-Startup-And-Frame-Loop"]
    C --> D{"What are you<br/>working on?"}
    D -->|rendering| R["Rendering-Overview<br/>-> Render-Stages"]
    D -->|editor tools| E["Editor-Overview<br/>-> Editor-Panels"]
    D -->|content| P["Asset-Burning-Pipeline<br/>-> Asset-Import-Runtime"]
    D -->|gameplay| G["Scene-And-GameObject-Model<br/>-> Built-In-Components"]
```

## What's changed recently

The knowledgebase was first generated on 2026-07-09. These are the larger changes since, and where they're documented:

```mermaid
timeline
    title Engine changes since the docs were generated
    2026-10-09 : Declarative render pipeline (Render-Stages)
               : Frustum culling, cached shadow map
               : Async scene loading + job system rework (Asset-Import-Runtime, Threading-And-Jobs)
               : RZE_LOG in Release and in the Editor's Log panel
    2026-10-10 : Camera icons and frustums, mesh bounds on selection
               : Assimp 6, metres and +Y-up conventions, no more mirrored models (World-Conventions)
               : 0..1 depth projections
               : Editor camera orbit, pan and dolly
               : Multi-selection, click picking (GPU) and marquee selection (Editor-Panels)
    2026-10-11 : Gizmo snapping and the editor grid (Gizmo-Snapping-And-Grid)
               : Semi-transparent, distance-faded debug lines
               : Editor shortcuts reference (Editor-Shortcuts)
               : Editor UI rework - context, self-registering panels, commands (Editor-Framework)
               : Property descriptors and the Object Inspector
               : Lifetime-checked GameObject handles
    2026-10-12 : Object GUIDs, scene format v2, names no longer unique
               : Copy, paste and duplicate (Editor-Overview)
               : Editor event bus, unsaved-changes marker (Editor-Framework)
```

| Area | What changed | Read |
|---|---|---|
| Rendering | Stages declare the data they read and write; the pipeline orders them. Main and secondary views. Culling, shadow caching, eye adaptation + ACES. GPU picking stage. | [Render-Stages](04-Rendering/Render-Stages.md), [Engine-Graphics-Layer](04-Rendering/Engine-Graphics-Layer.md) |
| Loading | Scenes load asynchronously: parsing on workers, GPU uploads time-sliced on the main thread. | [Asset-Import-Runtime](05-AssetPipeline/Asset-Import-Runtime.md), [Threading-And-Jobs](06-Platform-And-Infrastructure/Threading-And-Jobs.md) |
| Content | Assimp 6.0.5, units converted to metres, models no longer mirrored, deterministic burns. **Re-burn after pulling.** | [Asset-Burning-Pipeline](05-AssetPipeline/Asset-Burning-Pipeline.md), [World-Conventions](02-Architecture/World-Conventions.md) |
| Editor | Orbit/pan/dolly camera, glide-to-frame, light and camera icons, multi-select, click and marquee picking. Gizmo snapping (X, Ctrl to invert) and a fading ground grid. All shortcuts in one table. | [Editor-Panels](07-Editor/Editor-Panels.md), [Editor-Camera-And-Build-Launch](07-Editor/Editor-Camera-And-Build-Launch.md), [Gizmo-Snapping-And-Grid](07-Editor/Gizmo-Snapping-And-Grid.md), [Editor-Shortcuts](07-Editor/Editor-Shortcuts.md) |
| Diagnostics | `RZE_LOG` in Release and in the Log panel, clearer assert dialogs, PIX markers only when a profiler is attached. `-perfcapture` frame-time recording for A/B comparisons. | [Debug-Services-And-Logging](06-Platform-And-Infrastructure/Debug-Services-And-Logging.md) |
| Editor framework | Panels share an `EditorContext` and register themselves; each has a data step (`Update`, before ImGui's frame) and a draw step. Scene changes are commands applied once per frame; an event bus reports loads, saves, edits and selection changes. Window title shows unsaved changes. | [Editor-Framework](07-Editor/Editor-Framework.md) |
| Inspector | Components describe their properties in the Engine with no ImGui; the "Object Inspector" (was "Component View") draws them, nests materials, meshes and textures, and edits several selected objects at once. `OnEditorInspect` is gone. | [Object-Inspector-And-Properties](07-Editor/Object-Inspector-And-Properties.md), [Component-System](03-SceneAndGameObjects/Component-System.md) |
| Scene model | `GameObjectPtr` / `GameObjectComponentPtr` know when their target is destroyed. Objects have a saved GUID; names can repeat. Scene files are version 2 (an array keyed by GUID); version 1 files convert on load. **Resave scenes to update them.** | [Scene-And-GameObject-Model](03-SceneAndGameObjects/Scene-And-GameObject-Model.md) |
| Editing | Copy, paste and duplicate (Ctrl+C/V/D), pasted beside the originals. The gizmo and inspector edit every selected object. | [Editor-Overview](07-Editor/Editor-Overview.md#edit-copy-paste-and-duplicate), [Editor-Panels](07-Editor/Editor-Panels.md) |

## Key architectural themes (read this before diving into a section)

1. **Singleton/service-locator core.** `RZE()` (a free function returning a global `RZE_Engine&`) is the hub nearly everything reaches through — `RZE().GetResourceHandler()`, `RZE().GetRenderEngine()`, etc. There is no dependency injection; see [Engine-Startup-And-Frame-Loop.md](02-Architecture/Engine-Startup-And-Frame-Loop.md).
2. **Entity + component composition, not a data-oriented ECS.** `GameObject` owns a `std::vector<GameObjectComponentBase*>` of polymorphic components — classic OOP composition. Component type IDs are assigned via a CRTP static counter and a manual registration macro, not real reflection. See [Component-System.md](03-SceneAndGameObjects/Component-System.md).
3. **Engine and low-level Renderer are separate libraries.** `Modules/Rendering` has zero dependency on `Engine` — it's a standalone DX11 abstraction with its own render thread and command queue. `Engine/Graphics` sits on top of it. See [Low-Level-Renderer-Module.md](04-Rendering/Low-Level-Renderer-Module.md).
4. **Assets are "burned" offline, not loaded from source formats at runtime.** A separate tool, `SourceAssetBurner`, converts `.obj`/`.fbx` (via Assimp) into RZE's own binary `.meshasset`/`.materialasset` formats ahead of time; the runtime only ever reads the burned format, and loads scenes asynchronously. See [Asset-Burning-Pipeline.md](05-AssetPipeline/Asset-Burning-Pipeline.md).
5. **Render passes are declarative.** Each render stage says what data it writes, reads or modifies, and the pipeline works out the order and checks it. Stages never call each other. See [Render-Stages.md](04-Rendering/Render-Stages.md).
6. **The Editor and Game are thin shells over the same Engine.** Both subclass `RZE_Application` and mostly just configure render targets/stages and scene loading; nearly all logic lives in `Engine`. See [App-Shell-Pattern.md](02-Architecture/App-Shell-Pattern.md).
7. **The codebase is candid about its own rough edges** — component-inheritance IDs, an unwired custom allocator, and more are called out directly in source comments. These are collected in [10-KnownIssues](10-KnownIssues/Architectural-Notes-And-TODOs.md) rather than glossed over.
