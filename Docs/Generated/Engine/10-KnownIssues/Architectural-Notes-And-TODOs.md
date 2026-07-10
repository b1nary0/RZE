# Known Architectural Issues & TODOs

This page collects rough edges the codebase itself is candid about (source comments, `#TODO`/`@NOTE` annotations) plus a couple of risks observed while reading the code. None of these are "bugs" in the sense of currently breaking normal usage — they're documented limitations worth knowing about before extending the affected systems.

## 1. Component-inheritance ID collision (confirmed, actively documented in source)

`Engine\Src\Game\World\GameObject\GameObjectComponent.h` carries this comment verbatim:

> "This stuff doesn't support class hierarchies, and will only ever use the first registered component in any hierarchy... things like `CameraComponent` are broken because I refactored `EditorCameraComponent` to inherit from `CameraComponent`; this means we can't have both a `CameraComponent` and `EditorCameraComponent` reflected because the ID will be the same, and the ID is the driving factor behind all recognition of unique classes."

**Impact:** you cannot create a component subclass of another `GameObjectComponent`-derived type and expect it to get its own distinct type ID, factory entry, or independent serialization identity — it silently shares the parent's ID. Full detail in [Component-System.md](../03-SceneAndGameObjects/Component-System.md).

**If you need this fixed:** the ID system needs to stop being driven by the first `GameObjectComponent<T>` instantiation in a hierarchy and instead assign IDs explicitly per concrete leaf type, independent of inheritance depth.

## 2. `ReflectDB` potentially hashes pointers, not string contents

`Utils\Src\Utils\Reflect\ReflectDB.h/.cpp` generates `ReflectTypeID` via `std::hash<const char*>` applied to a type-name string. In most standard library implementations, `std::hash<const char*>` hashes the **pointer value**, not the string's contents (there's no guaranteed content-based specialization for `const char*` the way there is for `std::string`). If two different `const char*` literals happen to contain the same text but live at different addresses, they could plausibly get different `ReflectTypeID`s — silently breaking the "same type name → same ID" assumption this registry depends on. This system currently only backs Editor inspector sort-ordering (`EditorComponentCache`), not the primary component ID/factory path, so the blast radius today is limited to inspector ordering glitches rather than serialization corruption — but it's worth fixing before this registry is used for anything more load-bearing. See [Utils-Library-Reference.md](../06-Platform-And-Infrastructure/Utils-Library-Reference.md).

## 3. Custom allocator (`BlockAllocator`) exists but isn't wired in

`Engine\Src\EngineCore\Platform\Memory\BlockAllocator.h/.cpp` is described in its own comment as a "Buddy Block allocation algorithm" but currently behaves like a simple bump/stack allocator over a fixed 64KB pool, with a member literally named/commented "tmp for testing." `ResourceHandler` and general engine code still use raw `new`/`delete`; `RZE_NEW`/`RZE_DEL` macros in `Memory.h` are no-op wrappers around plain `new`/`delete`. Treat this as scaffolding, not the engine's actual memory strategy. See [Memory-Management.md](../06-Platform-And-Infrastructure/Memory-Management.md).

## 4. Job system (`JobScheduler`) is underused relative to its design

A fixed 4-worker thread pool exists and is initialized/shut down with the engine, but the only confirmed consumer in the explored source is the Editor's "Build Game.../Launch Game..." background jobs. Asset loading (`ResourceHandler::LoadResource`) remains synchronous. If you're chasing a load-time stall or considering parallelizing resource loading, this system is the obvious place to route work through — but it isn't doing that yet.

**Concrete weaknesses in the current implementation**, found reading `JobScheduler.cpp`/`WorkerThread.cpp`:
- `JobScheduler::Wait()` is a true busy-spin loop (`while (bShouldWait) { ... }`, no sleep/yield at all) — it pins a full CPU core for the duration of any wait. Idle worker threads at least sleep 1ms between poll attempts (`WorkerThread::ThreadSetup()`), but the scheduler's own wait doesn't.
- All workers pull from a single shared `mJobQueue` guarded by one global `JobMutex` (`RequestJob` uses `try_lock()` and just returns `false` on contention, causing the calling worker to spin again). The header carries its own `#TODO` acknowledging this: "Have each thread have a queue and send it to whoever is free at the moment instead of one queue because we're losing time to a locked mutex."
- `PushJob(Job::Task)` is fire-and-forget — there's no future/promise or result channel, so nothing can await a job's output or chain dependent work off it. This is likely why `ResourceHandler::LoadResource` was never routed through it: an async load needs a way to hand the result back.
- `MAX_WORKER_THREADS = 4` is a hard-coded constant, not derived from `std::thread::hardware_concurrency()`.

**To actually use this for async asset loading**, the missing piece is a result-bearing job (e.g. a `Job<T>`/future wrapper) so `LoadResource` can return something awaitable instead of the resource itself; the busy-wait and single-queue-contention issues would also be worth fixing before adding load-bearing traffic. See [Threading-And-Jobs.md](../06-Platform-And-Infrastructure/Threading-And-Jobs.md).

## 5. Dead code: `old_RenderCommands.h`

`Modules\Rendering\Src\Rendering\old_RenderCommands.h` contains vestigial command structs from a prior design iteration (e.g. `CreateBufferRenderCommand`) that aren't referenced by the current `RenderCommand.h`/`RenderThread.cpp`. Safe to ignore when reading the renderer, and a candidate for deletion in a future cleanup pass. See [Low-Level-Renderer-Module.md](../04-Rendering/Low-Level-Renderer-Module.md).

## 6. No culling or bucketing yet, despite being part of the original design

`Docs\RenderSystemThoughts.txt` describes bucketing (grouping RenderObjects into passes, e.g. shadow casters) and culling (determining viewport-relevant objects) as core `RenderSystem` responsibilities. Neither is implemented today — `RenderEngine` submits its entire scene to every render stage each frame. A commented-out sketch for a bucket API (`Renderer::StartBucket()`/`SubmitBucket()`) exists directly in `RenderEngine.h` as a design placeholder. See [Rendering-Overview.md](../04-Rendering/Rendering-Overview.md).

## 7. Slow linear-scan component lookups on `GameObject`

`GameObject::AddComponent<T>`, `GetComponent<T>`, and `RemoveComponent<T>` are all `std::find_if` linear scans over `m_components`, each explicitly flagged `// #TODO Slow function` in the header. Not a problem at current object/component counts, but worth knowing if profiling ever points at per-object component lookups — the fix would be an index (e.g. a type-ID→pointer map) rather than a linear scan. See [Scene-And-GameObject-Model.md](../03-SceneAndGameObjects/Scene-And-GameObject-Model.md).

## 8. Texture "burning" is a stub

`AssimpSourceImporter::WriteTextureAsset()` in `SourceAssetBurner` currently just returns `false` — there is no offline texture compression/packing step. Textures are referenced by relative path back into `Assets\` rather than converted into a burned format the way meshes and materials are. See [Asset-Burning-Pipeline.md](../05-AssetPipeline/Asset-Burning-Pipeline.md).

## Summary table

| #   | Issue                                                | Severity                                              | Where documented in-source                                   |
| --- | ---------------------------------------------------- | ----------------------------------------------------- | ------------------------------------------------------------ |
| 1   | Component-inheritance ID collision                   | High (blocks a whole pattern of component reuse)      | `GameObjectComponent.h` comment                              |
| 2   | `ReflectDB` pointer-hash risk                        | Low today, higher if reused elsewhere                 | Not commented — inferred from `std::hash<const char*>` usage |
| 3   | `BlockAllocator` not wired in                        | Low (no current impact, misleading if assumed active) | "tmp for testing" comment                                    |
| 4   | Job system underused                                 | Low (missed opportunity, not a bug)                   | Inferred from usage search                                   |
| 5   | Dead `old_RenderCommands.h`                          | Cosmetic                                              | File name itself (`old_` prefix)                             |
| 6   | No culling/bucketing                                 | Medium (perf ceiling as scene complexity grows)       | `RenderSystemThoughts.txt`, commented-out bucket sketch      |
| 7   | Slow linear component lookups                        | Low today                                             | `// #TODO Slow function` comments                            |
| 8   | Texture burning stubbed out                          | Medium (textures aren't actually "conditioned")       | `WriteTextureAsset` returns `false`                          |
