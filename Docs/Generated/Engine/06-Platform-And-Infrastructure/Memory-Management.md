# Memory Management

RZE has two independent, unrelated pieces of custom memory management: a mostly-unused general allocator in `Engine`, and a heavily-used bump allocator in `Modules\Rendering`. They don't share code and are worth telling apart.

## `Engine\Src\EngineCore\Platform\Memory\BlockAllocator` — early-stage, not wired in

- `IAllocator` — interface: `Init`, `Allocate`, `Free`.
- `BlockAllocator : public IAllocator` — a source comment describes it as a "Buddy Block allocation algorithm," but the current implementation is closer to a simple bump/stack allocator (`mStackPtr`, tagged "tmp for testing" in a comment) over a fixed pool (`mPool`, default `ALLOCATOR_DEFAULT_FIXED_SIZE_KB` = 64KB, 32-byte alignment).
- **Not currently used anywhere else in the engine** — `ResourceHandler` and general engine allocations still use raw `new`/`delete`. Treat this as scaffolding for a future allocation strategy, not the engine's actual memory model today.
- `Memory.h` defines `RZE_NEW`/`RZE_DEL` macros, currently just plain `new;`/`delete;` — placeholders for future allocation tracking, not yet doing anything beyond what a bare `new`/`delete` would do.

## `Modules\Rendering\MemArena` — the one that actually matters

A frame-scoped bump allocator with a fixed 64MB size, backing every `RenderCommand` allocation in the low-level renderer. This is fully described in [Low-Level-Renderer-Module.md](../04-Rendering/Low-Level-Renderer-Module.md) — see that page for `AllocType<T>()`, `Cycle()`, and the pressure-tracking (`PressureValue::LOW`/`MED`/`HIGH`) surfaced live in the Editor's menu bar.

## `Utils\Src\Utils\Memory\` — shared low-level helpers

- `ByteStream.h/.cpp` — `ByteStream(name[, streamLength])`, `ReadFromFile(Filepath)`, cursor-based `PeekBytes()`/`PeekBytesAdvance(sizeBytes)`, `ReadBytes`/`WriteBytes`, `GetNumBytesWritten()`. Backs the binary mesh/material asset formats ([Asset-Burning-Pipeline.md](../05-AssetPipeline/Asset-Burning-Pipeline.md)).
- `MemoryUtils.h/.cpp` — `Kilobytes`/`Megabytes`/`Gigabytes(size_t)` helpers, and `AlignSize(U32 size, U32 boundary)` — used both to size `MemArena` (64MB) and to align constant-buffer uploads in `Rendering::Renderer::InternalUploadDataToBuffer`.

## Summary

| System | Status | Used for |
|---|---|---|
| `BlockAllocator` (Engine) | Scaffolding, not wired in | Nothing yet — general engine code still uses raw `new`/`delete` |
| `MemArena` (Rendering) | Actively used, load-bearing | Every `RenderCommand` allocated per frame |
| `ByteStream`/`MemoryUtils` (Utils) | Actively used | Binary asset serialization, GPU buffer alignment |

## Key files

- `Engine\Src\EngineCore\Platform\Memory\BlockAllocator.h/.cpp`, `Memory.h`
- `Modules\Rendering\Src\Rendering\MemArena.h/.cpp`
- `Utils\Src\Utils\Memory\ByteStream.h/.cpp`, `MemoryUtils.h/.cpp`
