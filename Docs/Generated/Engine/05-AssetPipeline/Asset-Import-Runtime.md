# Runtime Asset Import & Resource Loading

How burned `.meshasset` / `.materialasset` files (see [Asset-Burning-Pipeline.md](Asset-Burning-Pipeline.md)) and textures get loaded at runtime, and how `ResourceHandler` keeps track of them.

There are **two ways to load** a resource:

| | Synchronous | Asynchronous |
|---|---|---|
| Call | `ResourceHandler::LoadResource<T>(path)` | `AsyncResourceBatch::Request<T>(path)`, inside an `AsyncOperation` |
| Where the work runs | All on the main thread, right away | File reading and parsing on worker threads; GPU uploads on the main thread, a little each frame |
| Stalls the frame? | Yes, for the whole load | No; it's time-sliced by a per-frame budget |
| Used by | One-off loads: shaders, config | **Scene loading** (`SceneLoadOperation`), **preloading** (`ResourcePreloadOperation`), **one resource at a time** (`PendingResource`): the Content Browser's images, thumbnail meshes, assets dropped or picked in the editor |

Both end in the same place: a resource registered in `ResourceHandler`, reached through a `ResourceHandle`.

## `ResourceHandler` (`Engine\Src\EngineCore\Resources\`)

A reference-counted table of loaded resources, keyed by a string made from the file path. **Main thread only**: the table and the reference counts aren't synchronised.

```cpp
ResourceHandle LoadResource<ResourceT>(const Filepath& path, Args... args); // load, or reuse if already loaded
const ResourceT* GetResource<ResourceT>(const ResourceHandle& handle);      // no type check: ask for the right type
bool HasResource(const Filepath& path) const;
ResourceHandle GetHandle(const Filepath& path);                             // empty handle if it isn't loaded
ResourceHandle RegisterLoadedResource(const Filepath& path, IResource* r);  // adopt a resource loaded elsewhere (async)
void ReleaseResource(ResourceHandle& handle);
```

`ResourceHandle` is a move-only handle; when the last handle to a resource goes, the resource is released. Resources implement `IResource` (`Utils\Src\Utils\Interfaces\Resource.h`): `Load(path)` and `Release()`, plus the optional async hooks below.

Used for `StaticMeshResource`, `MaterialResource`, `Texture2D`, `VertexShader` / `PixelShader`, and engine config.

## Async loading, step by step

A resource opts in by returning `true` from `SupportsAsyncLoad()` and splitting its load in two. Today `StaticMeshResource`, `MaterialResource` and `Texture2D` do; anything else requested through a batch falls back to a normal synchronous load.

```mermaid
%%{init: {'flowchart': {'subGraphTitleMargin': {'top': 6, 'bottom': 10}}, 'themeCSS': '.labelBkg, .edgeLabel { background-color: var(--background-primary) !important; } .messageText, .labelText, .loopText { paint-order: stroke; stroke: var(--background-primary); stroke-width: 5px; stroke-linejoin: round; }'}}%%
flowchart TD
    Req["batch.Request&lt;T&gt;(path)"] --> Loaded{"already<br/>loaded?"}
    Loaded -->|yes| Ref["just reference it"]
    Loaded -->|no| Async{"SupportsAsyncLoad?"}
    Async -->|no| Sync["ResourceHandler::LoadResource<br/>(synchronous fallback)"]
    Async -->|yes| CPU["LoadCPU(path)<br/>worker thread:<br/>read + parse, no GPU"]
    CPU --> Deps["RequestDependencies(batch)<br/>main thread: e.g. a mesh<br/>requests its textures"]
    Deps --> Wait["wait until those<br/>are finalized"]
    Wait --> Fin["FinalizeStep() repeatedly<br/>main thread, within the frame budget:<br/>GPU buffers, one piece at a time"]
    Fin --> Reg["RegisterLoadedResource<br/>-> ResourceHandler"]
```

- **`LoadCPU`** may not touch the renderer, the `ResourceHandler` or any other engine state; it only reads and decodes.
- **`GetNextFinalizeStepCost`** tells the batch how big the next step is (bytes copied into the render command arena, bytes uploaded to the GPU), so it can be pushed to a later frame if it won't fit. `StaticMeshResource` finalizes one submesh (its GPU buffers) per step, and makes the mesh's materials with the last.
- **`GetFinalizeProgress`** (optional) says how far a resource that finalizes in several steps has got, 0 to 1, for progress displays; `StaticMeshResource` counts its submeshes. The batch's `GetSettledAmount()` adds it to the resources already settled (e.g. 2.5 of 4), and `GetCurrentPath()` is the first one it's still waiting on.
- Because a mesh's or material's textures are its dependencies, they're always finalized first, so when the materials are made, their texture lookups are cache hits.
- The batch holds every resolved resource until `ReleaseHeldResources()`, so nothing is unloaded before whoever asked for it picks it up.
- **Loads in flight are shared.** A batch asking for a resource another batch is already loading (`ResourceHandler::TryBeginAsyncLoad` returns false) waits for that load and takes a handle to the result, instead of reading and uploading it a second time. If the other load fails or is cancelled, the waiting batch loads it itself. This matters once two scenes load at once, e.g. the editor's scene and a thumbnail's.

### Preloading: `ResourcePreloadOperation`

`ResourcePreloadOperation` (`Engine\Src\EngineCore\Resources\ResourcePreloadOperation.h/.cpp`) loads a list of resources and their dependencies through a batch, then **keeps them loaded for as long as the operation is held**. Whoever started it can then use them without stalling the main thread:
- `GetHandle(path)` returns the handle to a resource it was asked for (or one that pulled in) as soon as that has loaded, even while it's still loading others. It returns an empty handle for one that failed, hasn't loaded yet, or wasn't asked for, and loads nothing. The handle is a reference of its own, so it keeps the resource loaded after the operation is gone. This is how a mesh is swapped at runtime: `StaticMeshResource::CreateInstance(handle)`, then `RenderComponent::SetMeshInstance` ([Materials-Meshes-Shaders.md](../04-Rendering/Materials-Meshes-Shaders.md#swapping-a-mesh-at-runtime)).
- Or load them synchronously, e.g. `RenderComponent::LoadMesh`, as a cache hit.

A resource that fails is logged and left out; the operation still succeeds.

- **Name:** "Preloading resources" unless one is given, e.g. "Loading lion.png". Its kind is `ResourceLoad`.
- **Progress** counts the files read (60%) and then the resources uploaded (40%), the uploads a part at a time for those that upload in several steps (`GetSettledAmount`), so it moves while a large file loads.
- **Status:** "Loading assets (n/m)" or "Uploading to GPU (n/m)", with the file it's waiting on on the next line, from the moment it starts.

### One resource: `PendingResource`

`PendingResource` (`Engine\Src\EngineCore\Resources\PendingResource.h/.cpp`) loads **one** resource in the background, for code that polls each frame (a panel, a widget, a batch). Move-only, main thread only.

```cpp
PendingResource mesh;
mesh.Load<StaticMeshResource>(path);          // takes it now if it's loaded, or starts "Loading Mountain.meshasset"
...
if (!mesh.IsLoading())                        // picks up the handle once it's in; call it each frame while waiting
{
    if (mesh.HasFailed()) { /* logged already; e.g. say so */ }
    else { /* mesh.GetHandle() keeps it loaded for as long as mesh holds it */ }
}
```

- `Load<T>(path, flags = Background)` returns the load it started (a `ResourcePreloadOperation` named "Loading <file name>"), e.g. to add to an [`AsyncOperationGroup`](#several-operations-as-one-asyncoperationgroup), or null if it took the resource straight away.
- `Reset()`, loading something else, moving another `PendingResource` in, or destroying it **cancels** a load that's still running and lets go of the resource.
- Used by the Content Browser's tiles, the thumbnail renderer's meshes, and the editor's background loads of assets dropped in the scene view or set on an asset field ([Object-Inspector-And-Properties.md](../07-Editor/Object-Inspector-And-Properties.md#asset-fields)). A property says what its asset loads as with `.Preload<T>()`.

## Releasing resources

A resource can't be destroyed the moment its last handle goes: the render thread runs a frame behind, so a command queued this frame (an upload, a draw) may still read its memory. So **every** resource the `ResourceHandler` destroys, whether released or discarded, goes through `DestroyWhenRendered`:

1. It's taken out of the table straight away, tagged with the frame being recorded (`Renderer::GetRecordingFrame()`).
2. `ResourceHandler::Update()`, after present on every frame that updated the UI, destroys the ones whose frame the render thread has finished (`Renderer::IsFrameProcessed`). Frames that skip the UI ([`UIFrameRate`](../02-Architecture/Engine-Startup-And-Frame-Loop.md#ui-frame-rate)) render the last UI again, which may still show a texture released since, so nothing is destroyed until a newer UI has replaced it.
3. At shutdown, `DestroyPendingNow()` destroys the rest once the render thread is idle, before the renderer shuts down ([Engine-Startup-And-Frame-Loop.md](../02-Architecture/Engine-Startup-And-Frame-Loop.md#shutdown)).

So a resource can be released in the same frame it was uploaded or drawn in. Code that releases something no longer has to wait frames itself.

## Async operations

Long-running work, like loading a scene or building the game, runs as an **`AsyncOperation`** (`Engine\Src\EngineCore\Async\`), driven by `AsyncOperationManager` (`RZE().GetAsyncOperationManager()`).

```mermaid
%%{init: {'flowchart': {'subGraphTitleMargin': {'top': 6, 'bottom': 10}}, 'themeCSS': '.labelBkg, .edgeLabel { background-color: var(--background-primary) !important; } .messageText, .labelText, .loopText { paint-order: stroke; stroke: var(--background-primary); stroke-width: 5px; stroke-linejoin: round; }'}}%%
stateDiagram-v2
    [*] --> Running: Start() / OnStart()
    Running --> Running: OnTick(budget) each frame,<br/>again with spare time (idle)<br/>(+ worker jobs via PushWorkerJob)
    Running --> Finishing: Complete() / Fail() / cancel
    Finishing --> Finished: no worker jobs left<br/>OnFinished(state) + completion callback
    Finished --> [*]: 2 frames later: OnCleanup(), released
```

- **Threading rules:** progress, status text and cancellation can be set from any thread. Every hook and the completion callback run on the main thread. The manager won't finish an operation while any of its worker jobs are still running, so jobs can safely capture `this`.
- **Frame budget** (`AsyncFrameBudget`): shared by the foreground operations each frame. It allows **8 ms** of main-thread time and **16 MB** of GPU uploads (soft limits; the first piece of work each frame always runs, so everything makes progress), and leaves **8 MB** of headroom in the render command arena (a hard limit). Background operations get a budget of their own on top (below).
- **Idle time:** when a paced frame finishes early, `AsyncOperationManager::TickIdle(ms)` gives the spare time before the next frame to the running operations, foreground before background, right after the frame is handed to the renderer, so it adds nothing to that frame's latency. Its budget says it's idle (`AsyncFrameBudget::IsIdle()`): extra time within the same frame, not a new one, so work that counts frames counts only ticks where it's false (`CaptureBatchOperation`'s settle frames do). Idle ticks only give operations more time; finishing them, their callbacks, retiring and starting new ones stay in `Tick()`. What an idle tick records goes out with the next frame, so the upload and arena bytes it uses come out of that frame's allowance. Frames only have idle time with vsync or a frame rate cap ([Engine-Startup-And-Frame-Loop.md](../02-Architecture/Engine-Startup-And-Frame-Loop.md#waiting-for-the-next-frame)).
- **Flags** (`EAsyncOperationFlags`, combinable):

  | Flag | Meaning |
  |---|---|
  | `Blocking` | Blocks the user. The Editor shows its progress modal for the oldest one (`GetBlockingOperation`). |
  | `SubStep` | Part of another operation that reports on it, e.g. a thumbnail run's scene load or PNG save, or one image of a folder's "Loading thumbnails". It describes the operation; whether it's shown is up to the [filter](#which-operations-are-shown). |
  | `Background` | Work nobody is waiting on. It's ticked **after** every foreground operation, with **2 ms** of main-thread time and a quarter of the upload allowance of its own, so it always progresses but never takes much of a frame. Its worker jobs go to the [background lane](../06-Platform-And-Infrastructure/Threading-And-Jobs.md#background-work). It keeps going while a `Blocking` operation runs. |

- **Kind** (`EAsyncOperationKind`, a constructor argument, `General` unless given): what the operation does, so displays can pick which kinds to show. Each is a bit of its own.

  | Kind | Operations |
  |---|---|
  | `ResourceLoad` | `ResourcePreloadOperation`, so every `PendingResource`; the Content Browser's "Loading thumbnails" |
  | `SceneLoad` | `SceneLoadOperation` |
  | `Rendering` | `CaptureBatchOperation`, so thumbnail runs |
  | `Saving` | Writing a captured image (`TextureFile::SaveToPNG`) |
  | `General` | Anything else, e.g. building the game (`JobAsyncOperation` takes a kind too) |

- **History:** `LogStep(text)` and `AddWarning(text)` add timestamped entries (`GetLoggedSteps`). A warning doesn't stop the operation, which can still succeed with warnings. The Editor's status bar shows them ([Status-Bar-And-Settings.md](../07-Editor/Status-Bar-And-Settings.md)).
- **Retiring:** an operation is only released two frames after it finishes, because the render thread may still be using data it submitted (see [Low-Level-Renderer-Module.md](../04-Rendering/Low-Level-Renderer-Module.md)).
- **Progress:** `WeightedProgress` blends several stages of different cost into one 0–1 value that never goes backwards; a stage is set as a count (`SetStage(done, total)`) or a fraction (`SetStageFraction`). Operations flagged `Blocking` get the Editor's progress modal automatically.
- **Timing:** the manager stamps an operation when it starts it, and `Finish` when it ends. `GetSecondsRunning()` is 0 before it starts, counts while it runs and stays fixed once it's finished. `EstimateSecondsLeft()` is how long it has left at the rate it's gone so far, once it's at least 5% done (`k_minProgressToEstimate`) and has run a second; the static `EstimateSecondsLeft(seconds, progress)` does the sum for any numbers.
- **`JobAsyncOperation`** wraps a single worker-thread function (the Editor's Build/Launch uses it).

### Which operations are shown

Operations only describe themselves (their kind, and whether they're a `SubStep`). What a progress display shows is an **`AsyncOperationFilter`** (`Engine\Src\EngineCore\Async\AsyncOperationFilter.h/.cpp`): whether sub-steps and blocking operations show, and a mask of the kinds that do (`ShownKinds`, `ShowsKind`, `SetShowsKind`); `Matches(operation)` is a flag test. Its defaults show everything.

- `AsyncOperationManager::GetVisibleOperations()` gives the running operations the manager's filter matches, oldest first; `GetRunningOperations()` gives them all.
- The engine sets the filter from Engine.ini's `[BackgroundTasks]` at start-up ([Config-System.md](../06-Platform-And-Infrastructure/Config-System.md)); `SetVisibilityFilter` changes it while running (the Editor's Settings > Background Tasks).

### Several operations as one: `AsyncOperationGroup`

`AsyncOperationGroup` (`Engine\Src\EngineCore\Async\AsyncOperationGroup.h/.cpp`) reports on several operations as one, e.g. a folder's images loading one at a time. Whoever adds an operation (`Add(operation, label)`) starts it too, flagged `SubStep`, so displays show the group in its place.

- **Progress** is how far they've got between them; **status** is "39 of 65 done" with the one it's waiting on. Operations can be added while it runs; it finishes once they've all finished.
- **History:** each is logged as it ends, by its label: a step when it succeeds ("lion.png"), "lion.png: cancelled" when it's cancelled on its own, a warning when it fails (the group still succeeds). At the end its status counts them, e.g. "17 done, 48 cancelled".
- Cancelling the group cancels those still running.

## Loading a scene: `SceneLoadOperation`

`Engine\Src\Game\SceneLoadOperation.h/.cpp` loads a `.scene` file into a `GameScene` in three phases. The engine loads into the active scene, `Blocking`. A [`HeadlessScene`](../03-SceneAndGameObjects/Scene-And-GameObject-Model.md#headlessscene-a-scene-rendered-off-screen) loads into its own, `Background | SubStep`; the operation holds that scene by `shared_ptr`, so it stays alive until the load finishes even if its owner lets go.

```mermaid
%%{init: {'flowchart': {'subGraphTitleMargin': {'top': 6, 'bottom': 10}}, 'themeCSS': '.labelBkg, .edgeLabel { background-color: var(--background-primary) !important; } .messageText, .labelText, .loopText { paint-order: stroke; stroke: var(--background-primary); stroke-width: 5px; stroke-linejoin: round; }'}}%%
sequenceDiagram
    participant Main as Main thread
    participant W as Worker threads
    Main->>W: 1. Parsing: read, parse and upgrade the .scene file,<br/>ask each component what resources it needs
    W-->>Main: dependency list
    Main->>W: 2. LoadingResources: AsyncResourceBatch<br/>LoadCPU for every mesh and texture
    W-->>Main: parsed data
    Main->>Main: finalize resources, a slice per frame
    Main->>Main: 3. Instantiating: GameObjectLoader creates objects in file order,<br/>a slice per frame (resource loads are now cache hits)
    Main-->>Main: completed (or, on failure, unload the partial scene)
```

Parsing also runs `SceneFormat::Upgrade`, so an older scene file is converted to the current format before anything reads it, and instantiating links each object to its parent by GUID; see [Scene-And-GameObject-Model.md](../03-SceneAndGameObjects/Scene-And-GameObject-Model.md#scene-files).

Components say what they need with a static `GatherResourceDependencies(componentData, outDependencies)` registered with `GameObjectComponentRegistry`; `RenderComponent` lists its mesh, and the materials and textures its material overrides swap in. Gathering everything up front is what lets the heavy loading happen before any objects exist.

## Importers (`Engine\Src\Asset\AssetImport\`)

- `MeshAssetImporter` reads the `.meshasset` format back into submesh data, placements and each submesh's material (name, path and `MaterialData`).
- `MaterialAssetImporter` reads `.materialasset` (versions 0 to 2) into `MaterialData`, which `MaterialInstance::Create` turns into a material. Older versions read as they drew: version 0 has no blend mode (cutout with an opacity mask, otherwise opaque), and versions 0 and 1 have no tint (white with a diffuse texture, grey without).
- `StaticMeshResource` (`Engine\Src\Game\StaticMeshResource.h/.cpp`) is the `IResource` for a mesh asset. It makes one shared, immutable `StaticMesh`, and `CreateInstance(handle)` gives each object its own `StaticMeshInstance` of it, holding the handle so the asset stays loaded ([Materials-Meshes-Shaders.md](../04-Rendering/Materials-Meshes-Shaders.md)).
- `MaterialResource` (`Engine\Src\Graphics\MaterialResource.h/.cpp`) is the `IResource` for one material asset loaded on its own, which a material slot's overrides can swap in.

## Key files

- `Engine\Src\EngineCore\Resources\ResourceHandler.h/.cpp`, `AsyncResourceBatch.h/.cpp`, `ResourceDependencyList.h`, `ResourcePreloadOperation.h/.cpp`, `PendingResource.h/.cpp`
- `Engine\Src\EngineCore\Async\` — `AsyncOperation`, `AsyncOperationManager`, `AsyncOperationFilter`, `AsyncOperationGroup`, `AsyncFrameBudget`, `JobAsyncOperation`, `WeightedProgress`
- `Engine\Src\Game\CaptureBatchOperation.h/.cpp` — capturing a batch of items off-screen (thumbnails)
- `Engine\Src\Game\SceneLoadOperation.h/.cpp`, `StaticMeshResource.h/.cpp`
- `Engine\Src\Graphics\MaterialResource.h/.cpp`
- `Engine\Src\Asset\AssetImport\MeshAssetImporter.h/.cpp`, `MaterialAssetImporter.h/.cpp`
- `Utils\Src\Utils\Interfaces\Resource.h` — `IResource`
